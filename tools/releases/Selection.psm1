$ErrorActionPreference = 'Stop'
function Read-ReleaseCatalog([string]$Root) {
    $catalog = Get-Content -LiteralPath (Join-Path $Root 'tools/releases/releases.json') -Raw | ConvertFrom-Json
    $manifest = Get-Content -LiteralPath (Join-Path $Root 'tools/packaging/packages.json') -Raw | ConvertFrom-Json
    if ($catalog.schemaVersion -ne 1 -or $catalog.workflow -ne 'all.yml') { throw 'Unsupported release catalog.' }
    $seen = @{}
    foreach ($release in $catalog.releases) {
        if (($release.tag -and $release.tag -notmatch '^[a-z0-9][a-z0-9_-]*$') -or $seen.ContainsKey($release.tag) -or !$release.name -or ($release.available -and !$release.tag)) { throw 'Invalid or duplicate release tag.' }
        $seen[$release.tag] = $true
        foreach ($id in $release.packages) { if ($id -notin $manifest.packages.id) { throw "Release $($release.tag) references unknown package $id." } }
        $platforms = @($manifest.packages | Where-Object { $_.id -in $release.packages } | ForEach-Object { $_.producers } | ForEach-Object { $_.solution } | Sort-Object -Unique)
        if (($platforms -join ',') -ne (@($release.platforms | Sort-Object -Unique) -join ',') -or $release.available -ne ($platforms.Count -gt 0 -and !!$release.tag)) { throw "Stale platform/availability metadata: $($release.tag)" }
    }
    foreach ($package in $manifest.packages) {
        if ($package.producers.Count -and $package.id -notin @($catalog.releases | ForEach-Object { $_.packages })) { throw "No release entry for active package $($package.id)." }
    }
    return $catalog
}
function Resolve-ReleaseSelection($Catalog, [string[]]$Tags) {
    $ids = @($Tags | ForEach-Object { $_ -split ',' } | ForEach-Object { $_.Trim() } | Where-Object { $_ } | Sort-Object -Unique)
    if (!$ids.Count) { throw 'Select at least one release.' }
    if ('all' -in $ids) {
        if ($ids.Count -ne 1) { throw 'Use all by itself, or an explicit release selection.' }
        $ids = @($Catalog.releases | Where-Object available | ForEach-Object { $_.tag })
    }
    foreach ($id in $ids) {
        $entry = @($Catalog.releases | Where-Object { $_.tag -eq $id })
        if ($entry.Count -ne 1) { throw "Unknown release tag: $id" }
        if (!$entry[0].available) { throw "Release $id has no active build/packaging rule." }
    }
    $releases = @($Catalog.releases | Where-Object { $_.tag -in $ids })
    return [pscustomobject]@{ Tags=@($releases.tag | Sort-Object); Packages=@($releases | ForEach-Object { $_.packages } | Sort-Object -Unique); Releases=$releases }
}
function Get-ReleaseCatalogContentHash([byte[]]$Bytes) {
    # Git normalizes text line endings; Windows checkouts may use CRLF.
    $text = [Text.Encoding]::UTF8.GetString($Bytes).TrimStart([char]0xfeff).Replace("`r`n", "`n")
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return ([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($text)))).Replace('-', '').ToLowerInvariant() }
    finally { $sha.Dispose() }
}
function Get-ReleaseCatalogHash([string]$Root) { return Get-ReleaseCatalogContentHash ([IO.File]::ReadAllBytes((Join-Path $Root 'tools/releases/releases.json'))) }
function New-ReleaseDispatch($Catalog, [string[]]$Tags, [string]$Repository, [string]$Ref, [string]$CatalogHash, [string]$RequestId) {
    if ($Repository -notmatch '^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$') { throw 'Repository must be owner/name.' }
    if (!$Ref -or $Ref.StartsWith('-') -or $Ref -match '[\x00-\x20\x7f]') { throw 'Enter a valid source branch or tag.' }
    if ($CatalogHash -notmatch '^[a-f0-9]{64}$' -or $RequestId -notmatch '^[a-f0-9]{16}$') { throw 'Invalid release request metadata.' }
    $selection = Resolve-ReleaseSelection $Catalog $Tags
    return [pscustomobject]@{
        Selection=$selection
        Arguments=@('workflow', 'run', $Catalog.workflow, '--repo', $Repository, '--ref', $Ref, '--json')
        Json=(@{tag_list=($selection.Tags -join ','); catalog_hash=$CatalogHash; request_id=$RequestId} | ConvertTo-Json -Compress)
    }
}
Export-ModuleMember -Function Read-ReleaseCatalog, Resolve-ReleaseSelection, Get-ReleaseCatalogHash, Get-ReleaseCatalogContentHash, New-ReleaseDispatch
