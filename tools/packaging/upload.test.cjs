// Tests only the GitHub adapter; the portable packager has PowerShell tests.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs/promises');
const path = require('node:path');
const os = require('node:os');
const {EventEmitter} = require('node:events');
const {github} = require('./upload.cjs');
const delay = ms => new Promise(resolve => setTimeout(resolve, ms));
test('the default uploader loads the installed ESM client and passes completed ZIPs to it', async t => {
  const {DefaultArtifactClient} = await import('@actions/artifact');
  const originalUpload = DefaultArtifactClient.prototype.uploadArtifact;
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'wfp-upload-client-'));
  t.after(async () => {
    DefaultArtifactClient.prototype.uploadArtifact = originalUpload;
    await fs.rm(root, {recursive: true, force: true});
  });
  let calls = 0;
  DefaultArtifactClient.prototype.uploadArtifact = async (name, files, directory, options) => {
    calls++;
    assert.equal(name, 'fixture.zip');
    assert.equal(files.length, 1);
    assert.equal(path.dirname(files[0]), directory);
    assert.equal(await fs.readFile(files[0], 'utf8'), 'complete');
    assert.deepEqual(options, {retentionDays: 90, skipArchive: true});
  };
  await github({root, spawnProcess: (command, args) => {
    const child = new EventEmitter();
    const work = args[args.indexOf('-WorkDirectory') + 1];
    setTimeout(async () => {
      try {
        const file = path.join(work, 'fixture.zip');
        await fs.writeFile(file, 'complete');
        await fs.writeFile(path.join(work, 'archives', 'fixture.ready.json'), JSON.stringify({id: 'fixture', file}));
        child.emit('close', 0);
      } catch (error) { child.emit('error', error); }
    }, 0);
    return child;
  }});
  assert.equal(calls, 1);
});
for (const fail of [false, true]) {
  test(`GitHub adapter ${fail ? 'propagates upload failure and waits active uploads' : 'uploads complete records exactly once with four workers'}`, async t => {
    const root = await fs.mkdtemp(path.join(os.tmpdir(), 'wfp-upload-test-'));
    t.after(() => fs.rm(root, {recursive: true, force: true}));
    let active = 0, peak = 0, observedFailure = false;
    const uploaded = [];
    const spawnProcess = (command, args) => {
      assert.equal(command, 'powershell.exe');
      const child = new EventEmitter();
      const work = args[args.indexOf('-WorkDirectory') + 1];
      setTimeout(async () => {
        try {
          const directory = path.join(work, 'archives');
          await fs.writeFile(path.join(directory, 'unfinished.pending'), 'not JSON yet');
          for (let id = 0; id < 12; id++) {
            const file = path.join(work, `${id}.zip`);
            await fs.writeFile(file, 'complete');
            await fs.writeFile(path.join(directory, `${id}.ready.json`), JSON.stringify({id: String(id), file}));
          }
          if (fail) {
            while (true) {
              try { await fs.access(path.join(work, 'upload-failed.json')); observedFailure = true; break; }
              catch { await delay(10); }
            }
          }
          child.emit('close', fail ? 1 : 0);
        } catch (error) { child.emit('error', error); }
      }, 0);
      return child;
    };
    const run = github({root, spawnProcess, uploader: async result => {
      assert.equal(await fs.readFile(result.file, 'utf8'), 'complete');
      active++; peak = Math.max(peak, active);
      await delay(result.id === '0' ? 20 : 50);
      active--;
      if (fail && result.id === '0') throw new Error('Upload failed');
      uploaded.push(result.id);
    }});
    if (fail) { await assert.rejects(run, /Upload failed/); assert(observedFailure); }
    else { await run; assert.equal(uploaded.length, 12); assert.equal(new Set(uploaded).size, 12); }
    assert.equal(active, 0); assert.equal(peak, 4);
  });
}
test('a successful coordinator without ready records fails instead of reporting an empty upload', async t => {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), 'wfp-upload-empty-'));
  t.after(() => fs.rm(root, {recursive: true, force: true}));
  await assert.rejects(github({root, uploader: async () => assert.fail('No upload expected'), spawnProcess: () => {
    const child = new EventEmitter(); setTimeout(() => child.emit('close', 0), 10); return child;
  }}), /No completed/);
});
