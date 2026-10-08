[CmdletBinding()]
param([string]$Password = $env:SCCOMPILE_PASSWORD)
$ErrorActionPreference = 'Stop'
if (-not $Password) { throw 'Set SCCOMPILE_PASSWORD or supply -Password before invoking this manual compiler.' }
$repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$source = Join-Path $repository 'source/SplinterCell.WidescreenFix'
$work = Join-Path $repository ('build/sccompile/' + [Guid]::NewGuid())
$null = New-Item -ItemType Directory -Force -Path $work
$archive = Join-Path $source 'sccompile.bin'
& (Join-Path $repository 'data/7za.exe') x "-p$Password" $archive "-o$work" -y *> (Join-Path $work 'extract.log')
if ($LASTEXITCODE -ne 0) { throw 'Failed to unpack the sccompile archive. No packaged scripts were updated.' }
Copy-Item -Path (Join-Path $source 'UnrealScript/*') -Destination $work -Recurse -Force
foreach ($name in @('Core', 'Echelon', 'EchelonCharacter', 'EchelonEffect', 'EchelonGameObject', 'EchelonHUD', 'EchelonIngredient', 'EchelonMenus', 'EchelonPattern', 'Editor', 'Engine', 'UDebugMenu', 'UWindow')) {
    $null = New-Item -ItemType Directory -Force -Path (Join-Path $work "$name/Inc")
}
$outputs = @('EchelonHUD.u', 'EchelonIngredient.u', 'UWindow.u')
# Require freshly generated files; an archive's pre-existing .u files cannot
# accidentally make a failed compilation look successful.
foreach ($name in $outputs) { Remove-Item -LiteralPath (Join-Path $work "System/$name") -Force -ErrorAction SilentlyContinue }
Push-Location (Join-Path $work 'System')
try {
    & './UCC.exe' make -nobind *> (Join-Path $work 'compile.log')
    if ($LASTEXITCODE -ne 0) { throw "UnrealScript compilation failed. See $work/compile.log" }
} finally { Pop-Location }
foreach ($name in $outputs) {
    $file = Get-Item -LiteralPath (Join-Path $work "System/$name") -ErrorAction Stop
    if ($file.Length -eq 0) { throw "Compiler output is empty: $name" }
}
$destination = Join-Path $repository 'data/SplinterCell.WidescreenFix/system/update'
$null = New-Item -ItemType Directory -Force -Path $destination
foreach ($name in $outputs) { Copy-Item -LiteralPath (Join-Path $work "System/$name") -Destination (Join-Path $destination $name) -Force }
Write-Host "Updated $($outputs -join ', '). Review the changed .u files before committing."
