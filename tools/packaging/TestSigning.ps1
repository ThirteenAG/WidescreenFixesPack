# Windows integration test: uses a disposable certificate and the real SDK
# signtool, including the production timestamp server. No CI secrets are needed.
$ErrorActionPreference = 'Stop'
Import-Module (Join-Path $PSScriptRoot 'Packaging.psm1') -Force -DisableNameChecking
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work = Join-Path $root ('build/packaging-signing-tests/' + [Guid]::NewGuid())
$null = New-Item -ItemType Directory -Path $work
$certificates = @(); $previousPassword = $env:CODE_SIGNING_PASSWORD
function Assert($Condition, [string]$Message) { if (!$Condition) { throw $Message } }
function Assert-Fails([scriptblock]$Action, [string]$Pattern) {
    $failure = $null
    try { & $Action } catch { $failure = $_ }
    if (!$failure -or "$failure" -notmatch $Pattern) { throw "Expected '$Pattern', got '$failure'" }
}
try {
    $tool = (Get-ChildItem 'C:\Program Files (x86)\Windows Kits\10\bin' -Recurse -Filter signtool.exe |
        Where-Object { $_.Directory.Name -eq 'x64' } |
        Sort-Object { [version]$_.Directory.Parent.Name } | Select-Object -Last 1).FullName
    if (!$tool) { throw 'Windows SDK signtool is required.' }
    $env:CODE_SIGNING_PASSWORD = [Guid]::NewGuid().ToString()
    $password = ConvertTo-SecureString $env:CODE_SIGNING_PASSWORD -AsPlainText -Force
    foreach ($name in @('publisher', 'other-publisher')) {
        $certificate = New-SelfSignedCertificate -Type CodeSigningCert -Subject ('CN=WFP packaging test ' + [Guid]::NewGuid()) -CertStoreLocation 'Cert:\CurrentUser\My' -NotAfter (Get-Date).AddDays(1) -KeyAlgorithm RSA -KeyLength 2048 -HashAlgorithm SHA256 -Provider 'Microsoft Enhanced RSA and AES Cryptographic Provider' -KeySpec Signature
        $certificates += $certificate
        $null = Export-PfxCertificate -Cert $certificate -FilePath (Join-Path $work "$name.pfx") -Password $password
        Assert (!(Test-Path "Cert:\CurrentUser\Root\$($certificate.Thumbprint)")) 'Test certificate was unexpectedly trusted.'
    }
    $file = Join-Path $work 'fixture.exe'
    Copy-Item -LiteralPath (Join-Path $root 'data/7za.exe') -Destination $file
    $arguments = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'sign.ps1'), '-FilePath', $file, '-CertificatePath', (Join-Path $work 'publisher.pfx'), '-SigningTool', $tool)
    $null = Invoke-PackageTool (Get-PackagingShell) $arguments
    $signature = Get-AuthenticodeSignature -LiteralPath $file
    Assert ($signature.SignerCertificate.Thumbprint -eq $certificates[0].Thumbprint) 'Wrong signer.'
    Assert ($null -ne $signature.TimeStamperCertificate) 'Timestamp missing.'
    Assert (!(Test-Path "Cert:\CurrentUser\Root\$($certificates[0].Thumbprint)")) 'Signing modified root trust.'
    if (!('WfpPackaging.SignatureVerification' -as [type])) { Add-Type -Path (Join-Path $PSScriptRoot 'SignatureVerification.cs') }
    Assert ([WfpPackaging.SignatureVerification]::Verify($file) -eq -2146762487) 'Expected the precise untrusted-root result.'
    # Keep the original signature for negative tests; only verification uses the
    # real signtool. This ensures the helper cannot silently accept another signer
    # or a file modified after signing.
    $verifyOnly = Join-Path $work 'verify-only.cmd'
    [IO.File]::WriteAllText($verifyOnly, "@echo off`r`nif `"%1`"==`"sign`" exit /b 0`r`n`"$tool`" %*`r`nexit /b %errorlevel%`r`n")
    $arguments[$arguments.IndexOf('-SigningTool') + 1] = $verifyOnly
    $arguments[$arguments.IndexOf('-CertificatePath') + 1] = Join-Path $work 'other-publisher.pfx'
    Assert-Fails { Invoke-PackageTool (Get-PackagingShell) $arguments } 'signer does not match'
    $arguments[$arguments.IndexOf('-CertificatePath') + 1] = Join-Path $work 'publisher.pfx'
    $bytes = [IO.File]::ReadAllBytes($file); $bytes[512] = $bytes[512] -bxor 1
    [IO.File]::WriteAllBytes($file, $bytes)
    Assert ([WfpPackaging.SignatureVerification]::Verify($file) -eq -2146869232) 'Expected the precise bad-digest result for the modified binary.'
    Assert-Fails { Invoke-PackageTool (Get-PackagingShell) $arguments } 'Signature verification failed'
    Assert (!(Test-Path "Cert:\CurrentUser\Root\$($certificates[0].Thumbprint)")) 'Failed verification modified root trust.'
    Write-Host 'PASS: real self-signed Authenticode signing, timestamp, unchanged root trust, unexpected signer rejection, and tampered binary rejection.'
} finally {
    $env:CODE_SIGNING_PASSWORD = $previousPassword
    foreach ($certificate in $certificates) {
        $thumbprint = $certificate.Thumbprint
        if (Test-Path "Cert:\CurrentUser\My\$thumbprint") { Remove-Item -LiteralPath "Cert:\CurrentUser\My\$thumbprint" -DeleteKey }
        $certificate.Dispose()
    }
    $prefix = [IO.Path]::GetFullPath((Join-Path $root 'build/packaging-signing-tests')).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (!([IO.Path]::GetFullPath($work)).StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw 'Test cleanup target is outside its workspace.' }
    Remove-Item -LiteralPath $work -Recurse -Force
}
