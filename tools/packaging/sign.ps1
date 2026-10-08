param(
    [Parameter(Mandatory=$true)][string]$FilePath,
    [Parameter(Mandatory=$true)][string]$CertificatePath,
    [string]$SigningTool
)
$ErrorActionPreference = 'Stop'
if (-not $env:CODE_SIGNING_PASSWORD) { throw 'Signing password is missing.' }
if (-not $SigningTool) {
    $SigningTool = (Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter signtool.exe |
        Where-Object { $_.Directory.Name -eq 'x64' } |
        Sort-Object { [version]$_.Directory.Parent.Name } | Select-Object -Last 1).FullName
}
if (-not $SigningTool -or -not (Test-Path -LiteralPath $SigningTool)) { throw 'signtool.exe is missing.' }
# Capture tool output; do not print the password or certificate arguments.
$output = & $SigningTool sign /fd SHA256 /f $CertificatePath /p $env:CODE_SIGNING_PASSWORD /tr http://timestamp.digicert.com /td SHA256 $FilePath 2>&1
if ($LASTEXITCODE -ne 0) { throw "Signing failed for $FilePath" }
$output = & $SigningTool verify /pa $FilePath 2>&1
if ($LASTEXITCODE -ne 0) { throw "Signature verification failed for $FilePath" }
