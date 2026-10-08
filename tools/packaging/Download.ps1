param([string]$RepositoryRoot)
$ErrorActionPreference = 'Stop'
if (-not $RepositoryRoot) { $RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..')) }
Import-Module (Join-Path $PSScriptRoot 'Packaging.psm1') -Force -DisableNameChecking
$manifest = Read-PackageManifest $RepositoryRoot
Invoke-DependencyDownloads $manifest $manifest.packages $RepositoryRoot (Join-Path $RepositoryRoot ('build/packaging-downloads/' + [Guid]::NewGuid()))
