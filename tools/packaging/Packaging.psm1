$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$script:RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))

function Resolve-PackagePath([string]$Root, [string]$Relative) {
    if (-not $Relative -or [IO.Path]::IsPathRooted($Relative) -or $Relative.Contains(':')) { throw "Invalid relative path: $Relative" }
    $result = [IO.Path]::GetFullPath((Join-Path $Root $Relative))
    $prefix = [IO.Path]::GetFullPath($Root).TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $result.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) { throw "Path escapes its root: $Relative" }
    return $result
}
function Assert-PackageFile([string]$File) {
    $item = Get-Item -LiteralPath $File -ErrorAction Stop
    if ($item.PSIsContainer -or $item.Length -eq 0 -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)) { throw "Missing, empty or invalid file: $File" }
}
function Get-ByteHash([byte[]]$Bytes) {
    $sha = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($sha.ComputeHash($Bytes)).Replace('-', '').ToLowerInvariant() } finally { $sha.Dispose() }
}
function Get-FileHashValue([string]$File) { return (Get-FileHash -LiteralPath $File -Algorithm SHA256).Hash.ToLowerInvariant() }
function ConvertTo-ToolArguments([string[]]$Values) {
    return (($Values | ForEach-Object {
        '"' + [regex]::Replace([regex]::Replace($_, '(\\*)"', '$1$1\"'), '(\\+)$', '$1$1') + '"'
    }) -join ' ')
}
function Start-PackageTool([string]$Command, [string[]]$Arguments, [string]$Directory = $script:RepositoryRoot) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $Command
    $info.Arguments = ConvertTo-ToolArguments $Arguments
    $info.WorkingDirectory = $Directory
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    $null = $process.Start()
    $output = New-Object IO.MemoryStream
    return @{ Process = $process; Output = $output; Copy = $process.StandardOutput.BaseStream.CopyToAsync($output); Error = $process.StandardError.ReadToEndAsync(); Command = $Command }
}
function Complete-PackageTool($Tool) {
    try {
        $Tool.Process.WaitForExit()
        $null = $Tool.Copy.GetAwaiter().GetResult()
        $errorText = $Tool.Error.GetAwaiter().GetResult()
        if ($Tool.Process.ExitCode -ne 0) {
            $text = [Text.Encoding]::Default.GetString($Tool.Output.ToArray())
            throw "$([IO.Path]::GetFileName($Tool.Command)) failed ($($Tool.Process.ExitCode)): $errorText`n$text"
        }
        return ,$Tool.Output.ToArray()
    } finally { $Tool.Output.Dispose(); $Tool.Process.Dispose() }
}
function Invoke-PackageTool([string]$Command, [string[]]$Arguments, [string]$Directory = $script:RepositoryRoot) {
    return ,(Complete-PackageTool (Start-PackageTool $Command $Arguments $Directory))
}
function Stop-PackageTool($Tool) {
    if (-not $Tool.Process.HasExited) {
        if ([Environment]::OSVersion.Platform -eq 'Win32NT') { $null = Invoke-PackageTool 'taskkill.exe' @('/PID', [string]$Tool.Process.Id, '/T', '/F') }
        else { $Tool.Process.Kill() }
    }
}
function Get-PackagingShell {
    if ([Environment]::OSVersion.Platform -eq 'Win32NT') { return (Get-Command powershell.exe).Source }
    return (Get-Command pwsh).Source
}
function Get-PackageArchiver([string]$Root) {
    if ([Environment]::OSVersion.Platform -eq 'Win32NT') { return Join-Path $Root 'data/7za.exe' }
    return (Get-Command 7z).Source
}
function Assert-Fields($Value, [string[]]$Allowed, [string]$Label) {
    foreach ($name in $Value.PSObject.Properties.Name) { if ($name -notin $Allowed) { throw "Unknown $Label field: $name" } }
}
function Read-PackageManifest([string]$Root = $script:RepositoryRoot) {
    $manifest = Get-Content -LiteralPath (Join-Path $Root 'tools/packaging/packages.json') -Raw | ConvertFrom-Json
    if ($manifest.schemaVersion -ne 1 -or $null -eq $manifest.packages -or $null -eq $manifest.solutions) { throw 'Invalid manifest version/structure.' }
    Assert-Fields $manifest @('$schema', 'schemaVersion', 'solutions', 'dependencies', 'packages') 'manifest'
    $ids = @{}; $projects = @{}
    foreach ($property in $manifest.dependencies.PSObject.Properties) {
        $dependency = $property.Value
        Assert-Fields $dependency @('archive', 'architecture', 'repository', 'tag', 'assetPattern', 'selection', 'nestedMember') 'dependency'
        $null = Resolve-PackagePath $Root $dependency.archive
        if ($dependency.architecture -notin @('x86', 'x64')) { throw 'Invalid dependency architecture.' }
    }
    foreach ($package in $manifest.packages) {
        Assert-Fields $package @('id', 'producers', 'files', 'dependencies', 'optionalTree', 'associatedPackages') 'package'
        if ($package.id -notmatch '^[A-Za-z0-9_.-]+$' -or $ids.ContainsKey($package.id)) { throw "Invalid/duplicate package: $($package.id)" }
        $ids[$package.id] = $true
        $destinations = @{}
        foreach ($producer in $package.producers) {
            Assert-Fields $producer @('project', 'solution', 'source', 'destination', 'embedPdb', 'sign') 'producer'
            if ($producer.project -notmatch '^[A-Za-z0-9_.-]+$' -or $projects.ContainsKey($producer.project) -or $producer.solution -notin $manifest.solutions) { throw "Invalid/duplicate producer: $($producer.project)" }
            $projects[$producer.project] = $true
            if ($producer.embedPdb -and $producer.embedPdb -notin @('if-present', 'required')) { throw 'Invalid PDB policy.' }
            if ($null -ne $producer.sign -and $producer.sign -isnot [bool]) { throw 'sign must be a boolean.' }
        }
        foreach ($file in $package.files) { Assert-Fields $file @('source', 'destination') 'file' }
        foreach ($rule in $package.dependencies) {
            Assert-Fields $rule @('id', 'member', 'destination') 'extraction'
            if (-not $manifest.dependencies.($rule.id) -or -not $rule.member) { throw "Invalid dependency: $($rule.id)" }
        }
        foreach ($item in @($package.producers) + @($package.files) + @($package.dependencies)) {
            if ($item.source) { $null = Resolve-PackagePath $Root $item.source }
            $null = Resolve-PackagePath $Root $item.destination
            if ($destinations.ContainsKey($item.destination)) { throw "Destination collision: $($package.id)/$($item.destination)" }
            $destinations[$item.destination] = $true
        }
        if ($package.optionalTree) {
            Assert-Fields $package.optionalTree @('source', 'destination') 'tree'
            $null = Resolve-PackagePath $Root $package.optionalTree.source
            $null = Resolve-PackagePath $Root $package.optionalTree.destination
        }
    }
    foreach ($package in $manifest.packages) { foreach ($id in $package.associatedPackages) { if (-not $ids.ContainsKey($id)) { throw "Unknown associated package: $id" } } }
    return $manifest
}
function Get-PeInfo([byte[]]$Bytes) {
    if ($Bytes.Length -lt 64 -or [BitConverter]::ToUInt16($Bytes, 0) -ne 0x5a4d) { throw 'Not a PE binary.' }
    $offset = [BitConverter]::ToUInt32($Bytes, 0x3c)
    if ($offset + 24 -gt $Bytes.Length -or [BitConverter]::ToUInt32($Bytes, $offset) -ne 0x4550) { throw 'Invalid PE header.' }
    $machine = [BitConverter]::ToUInt16($Bytes, $offset + 4)
    $architecture = switch ($machine) { 332 {'x86'} 34404 {'x64'} default {throw 'Unsupported PE architecture.'} }
    $optional = $offset + 24
    $directories = $optional + $(if ($architecture -eq 'x64') {112} else {96})
    $sections = $optional + [BitConverter]::ToUInt16($Bytes, $offset + 20)
    $debugRva = [BitConverter]::ToUInt32($Bytes, $directories + 48)
    $debugLength = [BitConverter]::ToUInt32($Bytes, $directories + 52)
    $debug = 0; $pdb = $null
    for ($i = 0; $i -lt [BitConverter]::ToUInt16($Bytes, $offset + 6); $i++) {
        $s = $sections + $i * 40
        $start = [BitConverter]::ToUInt32($Bytes, $s + 12)
        $size = [Math]::Max([BitConverter]::ToUInt32($Bytes, $s + 8), [BitConverter]::ToUInt32($Bytes, $s + 16))
        if ($debugRva -ge $start -and $debugRva -lt $start + $size) { $debug = [BitConverter]::ToUInt32($Bytes, $s + 20) + $debugRva - $start; break }
    }
    if ($debug) {
        for ($entry = $debug; $entry + 28 -le $debug + $debugLength -and $entry + 28 -le $Bytes.Length; $entry += 28) {
            if ([BitConverter]::ToUInt32($Bytes, $entry + 12) -ne 2) { continue }
            $pointer = [BitConverter]::ToUInt32($Bytes, $entry + 24)
            if ($pointer + 24 -le $Bytes.Length -and [Text.Encoding]::ASCII.GetString($Bytes, $pointer, 4) -eq 'RSDS') {
                $pdb = @{ Guid = [BitConverter]::ToString($Bytes, $pointer + 4, 16); Age = [BitConverter]::ToUInt32($Bytes, $pointer + 20) }
            }
        }
    }
    return @{ Architecture = $architecture; Pdb = $pdb; SecurityOffset = [BitConverter]::ToUInt32($Bytes, $directories + 32) }
}
function Get-PdbIdentity([byte[]]$Bytes) {
    if (-not [Text.Encoding]::ASCII.GetString($Bytes, 0, 24).StartsWith('Microsoft C/C++ MSF 7.00')) { throw 'Unsupported PDB format.' }
    $blockSize = [BitConverter]::ToUInt32($Bytes, 32); $directorySize = [BitConverter]::ToUInt32($Bytes, 44); $mapBlock = [BitConverter]::ToUInt32($Bytes, 52)
    if ($blockSize -lt 512 -or $blockSize -gt 65536 -or $directorySize -gt $Bytes.Length) { throw 'Invalid PDB directory.' }
    $directory = New-Object byte[] $directorySize
    for ($i = 0; $i -lt [Math]::Ceiling($directorySize / $blockSize); $i++) {
        $start = [BitConverter]::ToUInt32($Bytes, $mapBlock * $blockSize + $i * 4) * $blockSize
        [Array]::Copy($Bytes, $start, $directory, $i * $blockSize, [Math]::Min($blockSize, $directorySize - $i * $blockSize))
    }
    $count = [BitConverter]::ToUInt32($directory, 0)
    if ($count -lt 2 -or 4 + $count * 4 -gt $directory.Length) { throw 'Invalid PDB streams.' }
    $blockOffset = 4 + $count * 4; $firstSize = [BitConverter]::ToUInt32($directory, 4)
    if ($firstSize -ne [uint32]::MaxValue) { $blockOffset += [Math]::Ceiling($firstSize / $blockSize) * 4 }
    $infoSize = [BitConverter]::ToUInt32($directory, 8)
    if ($infoSize -lt 28 -or $infoSize -gt $Bytes.Length) { throw 'Invalid PDB information.' }
    $infoBlock = [BitConverter]::ToUInt32($directory, $blockOffset) * $blockSize
    return @{ Guid = [BitConverter]::ToString($Bytes, $infoBlock + 12, 16); Age = [BitConverter]::ToUInt32($Bytes, $infoBlock + 8) }
}
function Assert-EmbeddedPdb([byte[]]$Binary, [byte[]]$Pdb) {
    $offsets = @($Binary.Length - 32)
    $security = (Get-PeInfo $Binary).SecurityOffset
    if ($security) { for ($padding = 0; $padding -lt 8; $padding++) { $offsets += $security - 32 - $padding } }
    $footer = -1
    foreach ($offset in $offsets) {
        if ($offset -ge 0 -and $offset + 32 -le $Binary.Length -and [Text.Encoding]::ASCII.GetString($Binary, $offset + 16, 16) -eq "PDB_EMBED_V1`0`0`0`0") { $footer = $offset; break }
    }
    if ($footer -lt 0) { throw 'PDB embedding produced no valid footer.' }
    $original = [BitConverter]::ToUInt64($Binary, $footer); $compressed = [BitConverter]::ToUInt64($Binary, $footer + 8)
    if ($original -ne $Pdb.Length -or $compressed -lt 6 -or $compressed -gt $footer) { throw 'Invalid embedded PDB size.' }
    # miniz writes zlib. DeflateStream on .NET Framework reads the raw payload,
    # between the two-byte zlib header and four-byte Adler checksum.
    $inputStream = New-Object IO.MemoryStream(,$Binary)
    $inputStream.Position = $footer - $compressed + 2
    $deflate = New-Object IO.Compression.DeflateStream($inputStream, [IO.Compression.CompressionMode]::Decompress)
    $outputStream = New-Object IO.MemoryStream
    try {
        $deflate.CopyTo($outputStream)
        if ($outputStream.Length -ne $Pdb.Length -or (Get-ByteHash $outputStream.ToArray()) -ne (Get-ByteHash $Pdb)) { throw 'Embedded PDB does not match its input.' }
    } finally { $deflate.Dispose(); $inputStream.Dispose(); $outputStream.Dispose() }
}
function Get-ArchiveMembers([string]$Archiver, [string]$Archive) {
    $listing = [Text.Encoding]::UTF8.GetString((Invoke-PackageTool $Archiver @('l', '-slt', $Archive))).Replace("`r", '')
    foreach ($block in $listing -split "`n`n") {
        if ($block -match '(?m)^Path = (.+)$' -and $block -match '(?m)^Size = \d+$' -and $block -notmatch '(?m)^Folder = \+$|^Attributes = D') {
            $null = $block -match '(?m)^Path = (.+)$'; $Matches[1].Replace('\', '/')
        }
    }
}
function Select-ArchiveMember([string[]]$Members, [string]$Pattern) {
    $expression = '^' + $(if (-not $Pattern.Contains('/')) {'(?:.*/)?'} else {''}) + [regex]::Escape($Pattern).Replace('\*', '[^/]*') + '$'
    $found = @($Members | Where-Object { $_ -match $expression })
    if ($found.Count -ne 1) { throw "Expected one archive member matching $Pattern, found $($found.Count)." }
    return $found[0]
}
function Invoke-PackagingRequest([string]$Uri, $Headers, [string]$OutputFile) {
    for ($attempt = 1; $attempt -le 3; $attempt++) {
        try {
            if ($OutputFile) { Invoke-WebRequest -UseBasicParsing -Uri $Uri -OutFile $OutputFile -TimeoutSec 120; return }
            return Invoke-RestMethod -Uri $Uri -Headers $Headers -TimeoutSec 120
        } catch {
            if ($attempt -eq 3) { throw }
            Start-Sleep -Milliseconds (500 * $attempt)
        }
    }
}
function Invoke-DependencyDownloads($Manifest, $Packages, [string]$Root, [string]$Work) {
    $null = New-Item -ItemType Directory -Force -Path $Work
    [Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
    $archiver = Get-PackageArchiver $Root
    $releases = @{}; $resolved = @{}
    $headers = @{ Accept = 'application/vnd.github+json'; 'User-Agent' = 'WidescreenFixesPack' }
    if ($env:GH_TOKEN) { $headers.Authorization = "Bearer $env:GH_TOKEN" }
    foreach ($id in @($Packages.dependencies.id | Sort-Object -Unique)) {
        $definition = $Manifest.dependencies.$id
        $key = "$($definition.repository)/$($definition.tag)"
        if (-not $releases.ContainsKey($key)) {
            $endpoint = if ($definition.tag -eq 'latest') {'latest'} else {'tags/' + [Uri]::EscapeDataString($definition.tag)}
            $releases[$key] = Invoke-PackagingRequest "https://api.github.com/repos/$($definition.repository)/releases/$endpoint" $headers
        }
        $release = $releases[$key]
        $assets = @($release.assets | Where-Object { $_.name -match $definition.assetPattern })
        if ($definition.selection -eq 'highest-revision') { $assets = @($assets | Sort-Object { $null = $_.name -match '_r(\d+)'; [int]$Matches[1] } -Descending | Select-Object -First 1) }
        if ($assets.Count -ne 1) { throw "Expected one release asset for $id, found $($assets.Count)." }
        $asset = $assets[0]; $temporary = Join-Path $Work "$id.download.zip"
        Invoke-PackagingRequest $asset.browser_download_url $null $temporary
        if ((Get-Item -LiteralPath $temporary).Length -ne $asset.size) { throw "Incomplete download: $id" }
        $digest = Get-FileHashValue $temporary
        if ($asset.digest -and $asset.digest.StartsWith('sha256:') -and $asset.digest -ne "sha256:$digest") { throw "Dependency checksum mismatch: $id" }
        $null = Invoke-PackageTool $archiver @('t', $temporary)
        $output = $temporary
        if ($definition.nestedMember) {
            $member = Select-ArchiveMember @(Get-ArchiveMembers $archiver $temporary) $definition.nestedMember
            $output = Join-Path $Work "$id.inner.zip"
            [IO.File]::WriteAllBytes($output, (Invoke-PackageTool $archiver @('e', '-so', $temporary, $member)))
            $null = Invoke-PackageTool $archiver @('t', $output)
        }
        $sha256 = Get-FileHashValue $output
        Move-Item -LiteralPath $output -Destination (Resolve-PackagePath $Root $definition.archive) -Force
        $resolved[$id] = @{ repository = $definition.repository; tag = $release.tag_name; assetId = $asset.id; url = $asset.browser_download_url; downloadedSha256 = $digest; sha256 = $sha256 }
        Write-Host "Downloaded ${id}: $($release.tag_name)/$($asset.name)"
    }
    [IO.File]::WriteAllText((Join-Path $Work 'dependencies.json'), ($resolved | ConvertTo-Json -Depth 10))
}
function Invoke-ManifestPackage($Manifest, $Package, [string]$Root, [string]$Work, [string]$Signing = 'optional') {
    if ($Signing -notin @('required', 'optional', 'off')) { throw 'Invalid signing profile.' }
    $packageWork = Join-Path $Work $Package.id
    $stage = Join-Path $packageWork 'stage'; $check = Join-Path $packageWork 'verify'
    $archiver = Get-PackageArchiver $Root
    $null = New-Item -ItemType Directory -Force -Path $stage
    $expected = @{}; $pfx = $null
    if ($Signing -eq 'required' -and (-not $env:CODE_SIGNING_PFX -or -not $env:CODE_SIGNING_PASSWORD)) { throw 'Signing is required but its credentials are missing.' }
    try {
        if ($Signing -ne 'off' -and $env:CODE_SIGNING_PFX -and $env:CODE_SIGNING_PASSWORD) {
            $pfx = Join-Path $packageWork 'certificate.pfx'
            [IO.File]::WriteAllBytes($pfx, [Convert]::FromBase64String(($env:CODE_SIGNING_PFX -replace '\s', '')))
        }
        foreach ($file in $Package.files) {
            $source = Resolve-PackagePath $Root $file.source; Assert-PackageFile $source
            $target = Resolve-PackagePath $stage $file.destination
            $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target)
            Copy-Item -LiteralPath $source -Destination $target
            $expected[$file.destination] = Get-FileHashValue $target
        }
        foreach ($producer in $Package.producers) {
            $source = Resolve-PackagePath $Root $producer.source; Assert-PackageFile $source
            $target = Resolve-PackagePath $stage $producer.destination
            $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target)
            Copy-Item -LiteralPath $source -Destination $target
            $bytes = [IO.File]::ReadAllBytes($target); $info = $null; $embeddedPdb = $null
            if ($source -match '\.(asi|dll)$') {
                $info = Get-PeInfo $bytes
                $architecture = if ($producer.solution -in @('Win64', 'Dolphin')) {'x64'} else {'x86'}
                if ($info.Architecture -ne $architecture) { throw "Wrong build architecture: $($producer.project)" }
            } elseif ($source -match '\.(elf|prx)$') {
                if ($bytes.Length -lt 52 -or [BitConverter]::ToUInt32($bytes, 0) -ne 0x464c457f -or $bytes[4] -ne 1 -or $bytes[5] -ne 1 -or [BitConverter]::ToUInt16($bytes, 18) -ne 8) { throw "Not a MIPS ELF module: $($producer.project)" }
            }
            if ($producer.embedPdb -and ($info.Pdb -or $producer.embedPdb -eq 'required')) {
                if (-not $info.Pdb) { throw "No CodeView PDB reference: $($producer.project)" }
                $pdbPath = [IO.Path]::ChangeExtension($source, '.pdb'); Assert-PackageFile $pdbPath
                $embeddedPdb = [IO.File]::ReadAllBytes($pdbPath); $identity = Get-PdbIdentity $embeddedPdb
                if ($identity.Guid -ne $info.Pdb.Guid -or $identity.Age -ne $info.Pdb.Age) { throw "PDB does not match binary: $($producer.project)" }
                $stagedPdb = [IO.Path]::ChangeExtension($target, '.pdb')
                [IO.File]::WriteAllBytes($stagedPdb, $embeddedPdb)
                $null = Invoke-PackageTool (Join-Path $Root 'data/EmbedPDB.exe') @($target)
                Assert-EmbeddedPdb ([IO.File]::ReadAllBytes($target)) $embeddedPdb
                Remove-Item -LiteralPath $stagedPdb
            }
            if ($producer.sign -and $pfx) {
                $null = Invoke-PackageTool (Get-PackagingShell) @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $PSScriptRoot 'sign.ps1'), '-FilePath', $target, '-CertificatePath', $pfx)
            }
            if ($embeddedPdb) { Assert-EmbeddedPdb ([IO.File]::ReadAllBytes($target)) $embeddedPdb }
            $expected[$producer.destination] = Get-FileHashValue $target
        }
        $dependencyHashes = @{}
        foreach ($rule in $Package.dependencies) {
            $definition = $Manifest.dependencies.($rule.id)
            $archive = Resolve-PackagePath $Root $definition.archive; Assert-PackageFile $archive
            $member = Select-ArchiveMember @(Get-ArchiveMembers $archiver $archive) $rule.member
            $bytes = Invoke-PackageTool $archiver @('e', '-so', $archive, $member)
            if ((Get-PeInfo $bytes).Architecture -ne $definition.architecture) { throw "Wrong dependency architecture: $($rule.id)/$member" }
            $target = Resolve-PackagePath $stage $rule.destination
            $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target)
            [IO.File]::WriteAllBytes($target, $bytes)
            $expected[$rule.destination] = Get-FileHashValue $target
            $dependencyHashes[$rule.id] = Get-FileHashValue $archive
        }
        if ($Package.optionalTree) {
            $source = Resolve-PackagePath $Root $Package.optionalTree.source
            if (-not (Test-Path -LiteralPath $source)) { return @{ id = $Package.id; skipped = $true } }
            foreach ($file in Get-ChildItem -LiteralPath $source -Recurse -File) {
                Assert-PackageFile $file.FullName
                $destination = $Package.optionalTree.destination + '/' + $file.FullName.Substring($source.TrimEnd('\', '/').Length + 1).Replace('\', '/')
                if ($expected.ContainsKey($destination)) { throw "Destination collision: $destination" }
                $target = Resolve-PackagePath $stage $destination
                $null = New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target)
                Copy-Item -LiteralPath $file.FullName -Destination $target
                $expected[$destination] = Get-FileHashValue $target
            }
        }
        if (-not $expected.Count) { throw "Empty package: $($Package.id)" }
        $temporary = Join-Path $packageWork "$($Package.id).partial.zip"
        $null = Invoke-PackageTool $archiver @('a', '-tzip', $temporary, './*', '-r') $stage
        $null = Invoke-PackageTool $archiver @('t', $temporary)
        $null = Invoke-PackageTool $archiver @('x', $temporary, "-o$check", '-y')
        $extracted = @(Get-ChildItem -LiteralPath $check -Recurse -File)
        if ($extracted.Count -ne $expected.Count) { throw "Archive contents differ: $($Package.id)" }
        foreach ($file in $extracted) {
            $relative = $file.FullName.Substring($check.TrimEnd('\', '/').Length + 1).Replace('\', '/')
            if ($expected[$relative] -ne (Get-FileHashValue $file.FullName)) { throw "Archive file differs: $($Package.id)/$relative" }
        }
        $archives = Join-Path $Root 'data/Archives'; $null = New-Item -ItemType Directory -Force -Path $archives
        $output = Join-Path $archives "$($Package.id).zip"
        Move-Item -LiteralPath $temporary -Destination $output -Force
        return @{ id = $Package.id; file = $output; sha256 = Get-FileHashValue $output; files = @($expected.Keys | Sort-Object); dependencies = $dependencyHashes }
    } finally { if ($pfx) { Remove-Item -LiteralPath $pfx -Force -ErrorAction SilentlyContinue } }
}
Export-ModuleMember -Function *
