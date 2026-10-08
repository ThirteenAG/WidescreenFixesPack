// GitHub-only adapter. Building, downloading and packaging use PowerShell;
// this Action supplies the runtime credentials required by GitHub's client.
const fs = require('node:fs/promises');
const path = require('node:path');
const crypto = require('node:crypto');
const {spawn} = require('node:child_process');

async function github({project = '', packages = '', mode = 'after-build', signing = 'optional', root = process.cwd(), uploader, spawnProcess = spawn} = {}) {
  const work = path.join(root, 'build/packaging', crypto.randomUUID());
  const directory = path.join(work, 'archives');
  await fs.mkdir(directory, {recursive: true});
  const client = uploader ? null : new (await import('@actions/artifact')).DefaultArtifactClient();
  const upload = uploader || (async result => client.uploadArtifact(`${result.id}.zip`, [result.file], path.dirname(result.file), {retentionDays: 90, skipArchive: true}));
  const args = ['-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', path.join(root, 'tools/packaging/Release.ps1'), '-Mode', mode, '-Signing', signing, '-WorkDirectory', work];
  if (project) args.push('-Project', project);
  if (packages) args.push('-Packages', packages);
  const child = spawnProcess('powershell.exe', args, {cwd: root, windowsHide: true, stdio: 'inherit'});
  let finished = false, buildFailure, uploadFailure;
  child.on('error', error => {buildFailure = error; finished = true;});
  child.on('close', code => {if (code !== 0) buildFailure ||= new Error(`Release coordinator failed (${code})`); finished = true;});
  const seen = new Set(), active = new Set();
  while (true) {
    // Only atomically published JSON records are consumed. Their ZIPs have
    // already passed integrity, file-list and content-hash checks.
    const records = (await fs.readdir(directory)).filter(file => file.endsWith('.ready.json'));
    if (!project && !packages && !uploadFailure) {
      for (const name of records) {
        if (seen.has(name) || active.size >= 4) continue;
        const result = JSON.parse(await fs.readFile(path.join(directory, name), 'utf8'));
        seen.add(name);
        console.log(`Uploading ${result.id}.zip`);
        const task = Promise.resolve().then(() => upload(result)).catch(async error => {
          uploadFailure ||= error;
          await fs.writeFile(path.join(work, 'upload-failed.json'), JSON.stringify({message: String(error.message)}));
        }).finally(() => active.delete(task));
        active.add(task);
      }
    }
    if (finished && !active.size && (project || packages || uploadFailure || records.every(name => seen.has(name)))) break;
    await Promise.race([...active, new Promise(resolve => setTimeout(resolve, 200))]);
  }
  await Promise.allSettled(active);
  if (uploadFailure || buildFailure) throw uploadFailure || buildFailure;
  if (!project && !packages && !seen.size) throw new Error('No completed package artifacts were produced');
}
module.exports = {github};
