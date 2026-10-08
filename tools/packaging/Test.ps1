$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Packaging.psm1') -Force -DisableNameChecking
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$manifest = Read-PackageManifest $root
function Assert($Condition, [string]$Message) { if (-not $Condition) { throw $Message } }
function Assert-Fails([scriptblock]$Action, [string]$Pattern) {
    $failure = $null
    try { & $Action } catch { $failure = $_ }
    if (-not $failure -or "$failure" -notmatch $Pattern) { throw "Expected failure matching '$Pattern', got '$failure'" }
}
$work = Join-Path $root ('build/packaging-tests/' + [Guid]::NewGuid())
$null = New-Item -ItemType Directory -Force -Path (Join-Path $work 'data/A'), (Join-Path $work 'tools/packaging')
try {
    Assert-Fails { Resolve-PackagePath $work '../escape' } 'escapes'
    Assert-Fails { Resolve-PackagePath $work 'C:/escape' } 'Invalid relative'
    Assert-Fails { Select-ArchiveMember @('a.dll', 'b.dll') '*.dll' } 'found 2'
    Assert ((Select-ArchiveMember @('Xidi-v1/Win32/dinput8.dll', 'Xidi-v1/x64/dinput8.dll') 'Xidi-*/Win32/dinput8.dll') -eq 'Xidi-v1/Win32/dinput8.dll') 'Wrong extraction member.'
    $producer = [pscustomobject]@{project='A'; solution='Win32'; source='data/A/A.asi'; destination='A.asi'; sign=$false}
    $package = [pscustomobject]@{id='A'; producers=@($producer); files=@([pscustomobject]@{source='data/A/config.ini'; destination='config.ini'}); dependencies=@()}
    $fixture = [pscustomobject]@{schemaVersion=1; solutions=@('Win32'); dependencies=[pscustomobject]@{}; packages=@($package)}
    Copy-Item -LiteralPath (Join-Path $root 'data/7za.exe') -Destination (Join-Path $work 'data/7za.exe')
    $bytes = New-Object byte[] 512
    [BitConverter]::GetBytes([uint16]0x5a4d).CopyTo($bytes, 0)
    [BitConverter]::GetBytes([uint32]64).CopyTo($bytes, 0x3c)
    [BitConverter]::GetBytes([uint32]0x4550).CopyTo($bytes, 64)
    [BitConverter]::GetBytes([uint16]332).CopyTo($bytes, 68)
    [BitConverter]::GetBytes([uint16]224).CopyTo($bytes, 84)
    [BitConverter]::GetBytes([uint16]0x10b).CopyTo($bytes, 88)
    [IO.File]::WriteAllBytes((Join-Path $work 'data/A/A.asi'), $bytes)
    [IO.File]::WriteAllText((Join-Path $work 'data/A/config.ini'), 'configuration')
    [IO.File]::WriteAllText((Join-Path $work 'data/A/stale.asi'), 'not an input')
    [IO.File]::WriteAllText((Join-Path $work 'tools/packaging/packages.json'), ($fixture | ConvertTo-Json -Depth 10))
    $null = Read-PackageManifest $work
    $originalHash = Get-FileHashValue (Join-Path $work 'data/A/A.asi')
    $result = Invoke-ManifestPackage $fixture $package $work (Join-Path $work 'invocation') 'off'
    Assert ($result.files.Count -eq 2 -and 'stale.asi' -notin $result.files) 'Unexpected archive inputs.'
    Assert ((Get-FileHashValue (Join-Path $work 'data/A/A.asi')) -eq $originalHash) 'Source binary was changed.'
    Remove-Item -LiteralPath $result.file, (Join-Path $work 'data/A/config.ini')
    Assert-Fails { Invoke-ManifestPackage $fixture $package $work (Join-Path $work 'second') 'off' } 'does not exist|Cannot find'
    Assert (-not (Test-Path -LiteralPath $result.file)) 'A failed package created a ZIP.'
    $package.files += [pscustomobject]@{source='data/A/another.ini'; destination='CONFIG.INI'}
    [IO.File]::WriteAllText((Join-Path $work 'tools/packaging/packages.json'), ($fixture | ConvertTo-Json -Depth 10))
    Assert-Fails { Read-PackageManifest $work } 'collision'
    Assert-Fails { Assert-EmbeddedPdb $bytes ([Text.Encoding]::ASCII.GetBytes('dummy PDB')) } 'footer'
    $tracked = @(& git -C $root ls-files data)
    foreach ($file in $tracked) {
        $parts = $file.Split('/'); if ($parts.Count -lt 3) { continue }
        $entry = @($manifest.packages | Where-Object { $_.id -eq $parts[1] })
        if (-not $entry.Count -or -not $entry[0].producers.Count) { continue }
        $relative = ($parts | Select-Object -Skip 2) -join '/'
        if ($file -match '\.(ual|x64ual)$') {
            $destination = $relative -replace '\.(ual|x64ual)$', '.dll'
            Assert (@($entry[0].dependencies | Where-Object { $_.destination -eq $destination }).Count -eq 1) "Missing loader rule: $file"
        } elseif ($file -notmatch '\.(wrapper|pdb|db|ipdb|iobj|tmp|lib|exp|ilk|map)$|(?:^|/)\.gitkeep$') {
            Assert (@(@($entry[0].files) + @($entry[0].producers) | Where-Object { $_.source -eq $file -or $_.destination -eq $relative }).Count -gt 0) "Missing asset: $file"
        }
    }
    $farcry = $manifest.packages | Where-Object { $_.id -eq 'FarCry.WidescreenFix' }
    Assert ($farcry.producers.Count -eq 2) 'FarCry must require both architectures.'
    Assert-Fails { & (Join-Path $root 'tools/compile-splintercell-scripts.ps1') -Password '' } 'SCCOMPILE_PASSWORD'
    $fakeSign = Join-Path $work 'fake-sign.cmd'
    [IO.File]::WriteAllText($fakeSign, "@echo off`r`nexit /b 3`r`n")
    $previousPassword = $env:CODE_SIGNING_PASSWORD
    try {
        $env:CODE_SIGNING_PASSWORD = 'test-not-a-secret'
        Assert-Fails { Invoke-PackageTool (Get-PackagingShell) @('-NoProfile', '-File', (Join-Path $PSScriptRoot 'sign.ps1'), '-FilePath', 'dummy.asi', '-CertificatePath', 'dummy.pfx', '-SigningTool', $fakeSign) } 'Signing failed'
        [IO.File]::WriteAllText($fakeSign, "@echo off`r`nif `"%1`"==`"sign`" exit /b 0`r`nexit /b 4`r`n")
        Assert-Fails { Invoke-PackageTool (Get-PackagingShell) @('-NoProfile', '-File', (Join-Path $PSScriptRoot 'sign.ps1'), '-FilePath', 'dummy.asi', '-CertificatePath', 'dummy.pfx', '-SigningTool', $fakeSign) } 'Signature verification failed'
    } finally { $env:CODE_SIGNING_PASSWORD = $previousPassword }
    Write-Host 'PASS: manifest migration, ZIP integrity, unchanged sources, missing inputs, path/collision checks, symbols, signing failures, and manual compilation guard.'
} finally {
    $prefix = [IO.Path]::GetFullPath((Join-Path $root 'build/packaging-tests')).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not ([IO.Path]::GetFullPath($work)).StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test cleanup target is outside its workspace.' }
    Remove-Item -LiteralPath $work -Recurse -Force
}
