[CmdletBinding()]
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
# Windows PowerShell turns native stderr into error records. Capture all of it
# before checking the native exit code, including multi-line certificate errors.
function Invoke-SigningTool([string[]]$Arguments) {
    $savedPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = @(& $SigningTool @Arguments 2>&1)
        return @{ ExitCode = $LASTEXITCODE; Output = ($output | Out-String).Trim() }
    } finally { $ErrorActionPreference = $savedPreference }
}
$result = Invoke-SigningTool @('sign', '/fd', 'SHA256', '/f', $CertificatePath, '/p', $env:CODE_SIGNING_PASSWORD, '/tr', 'http://timestamp.digicert.com', '/td', 'SHA256', $FilePath)
if ($result.ExitCode -ne 0) { throw "Signing failed for $FilePath`n$($result.Output)" }
$result = Invoke-SigningTool @('verify', '/pa', $FilePath)
$signature = Get-AuthenticodeSignature -LiteralPath $FilePath -ErrorAction SilentlyContinue
# Windows PowerShell reports CERT_E_UNTRUSTEDROOT as UnknownError; newer
# versions may report NotTrusted. Neither status is accepted without rechecking.
if ($result.ExitCode -ne 0 -and (!$signature -or $signature.Status -notin @('NotTrusted', 'UnknownError'))) {
    throw "Signature verification failed for $FilePath ($($signature.Status): $($signature.StatusMessage))`n$($result.Output)"
}
$certificates = New-Object System.Security.Cryptography.X509Certificates.X509Certificate2Collection
$chain = New-Object System.Security.Cryptography.X509Certificates.X509Chain
try {
    $certificates.Import($CertificatePath, $env:CODE_SIGNING_PASSWORD, [System.Security.Cryptography.X509Certificates.X509KeyStorageFlags]::EphemeralKeySet)
    $expectedSigner = @($certificates | Where-Object { $_.HasPrivateKey })
    if ($expectedSigner.Count -ne 1 -or !$signature.SignerCertificate -or
        [Convert]::ToBase64String($signature.SignerCertificate.RawData) -ne [Convert]::ToBase64String($expectedSigner[0].RawData)) {
        throw "Signature verification failed for ${FilePath}: signer does not match the configured PFX."
    }
    if ($result.ExitCode -eq 0) { return }
    if (!('WfpPackaging.SignatureVerification' -as [type])) {
        Add-Type -Path (Join-Path $PSScriptRoot 'SignatureVerification.cs')
    }
    # Pin the configured publisher instead of requiring public CA trust. Only
    # CERT_E_UNTRUSTEDROOT is eligible; invalid digests, missing signatures,
    # revoked certificates and every other Windows verification error fail.
    $trustResult = [WfpPackaging.SignatureVerification]::Verify([IO.Path]::GetFullPath($FilePath))
    if ($trustResult -ne -2146762487) {
        throw "Signature verification failed for $FilePath (Windows trust result $trustResult).`n$($result.Output)"
    }
    $chain.ChainPolicy.ExtraStore.AddRange($certificates)
    $chain.ChainPolicy.RevocationMode = 'NoCheck'
    $null = $chain.ChainPolicy.ApplicationPolicy.Add((New-Object System.Security.Cryptography.Oid('1.3.6.1.5.5.7.3.3')))
    $null = $chain.Build($signature.SignerCertificate)
    $invalidStatus = @($chain.ChainStatus | Where-Object { $_.Status -notin @('NoError', 'UntrustedRoot') })
    $rootCertificate = $chain.ChainElements[$chain.ChainElements.Count - 1].Certificate
    if ($invalidStatus.Count -or $rootCertificate.Subject -ne $rootCertificate.Issuer -or
        !@($certificates | Where-Object { [Convert]::ToBase64String($_.RawData) -eq [Convert]::ToBase64String($rootCertificate.RawData) }).Count) {
        throw "Signature verification failed for ${FilePath}: invalid publisher chain or its root is not included in the configured PFX.`n$($result.Output)"
    }
    Write-Verbose 'Signature verified against the configured publisher; its root is not publicly trusted.'
} finally {
    $chain.Dispose()
    foreach ($certificate in $certificates) { $certificate.Dispose() }
}
