param([Parameter(Mandatory)][string]$Tags, [string]$CatalogHash = '', [string]$GithubOutput = $env:GITHUB_OUTPUT)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $PSScriptRoot 'Selection.psm1') -Force
$catalog = Read-ReleaseCatalog $root
if ($CatalogHash -and $CatalogHash -ne (Get-ReleaseCatalogHash $root)) { throw 'The release catalog on this branch differs from the picker. Update your checkout and review the selection again.' }
$selection = Resolve-ReleaseSelection $catalog @($Tags)
Write-Host "Selected releases: $($selection.Tags -join ', ')"
Write-Host "Selected packages: $($selection.Packages -join ', ')"
if ($GithubOutput) {
    [IO.File]::AppendAllText($GithubOutput, "tags=$($selection.Tags -join ',')`npackages=$($selection.Packages -join ',')`n", (New-Object Text.UTF8Encoding($false)))
}
if ($env:GITHUB_STEP_SUMMARY) {
    [IO.File]::AppendAllText($env:GITHUB_STEP_SUMMARY, "## Release selection`n`n" + (($selection.Releases | ForEach-Object { "- $($_.name): $($_.packages -join ', ')" }) -join "`n") + "`n")
}
