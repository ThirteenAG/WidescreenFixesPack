[CmdletBinding()]
param(
    [ValidateSet('after-build', 'as-ready')][string]$Mode = 'after-build',
    [string]$Project = '',
    [string]$Packages = '',
    [ValidateSet('required', 'optional', 'off')][string]$Signing = 'optional',
    [switch]$Existing,
    [switch]$Available,
    [switch]$NoDownload,
    [string]$RepositoryRoot,
    [string]$WorkDirectory
)
$ErrorActionPreference = 'Stop'
if (-not $RepositoryRoot) { $RepositoryRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..')) }
Import-Module (Join-Path $PSScriptRoot 'Packaging.psm1') -Force -DisableNameChecking
$RepositoryRoot = [IO.Path]::GetFullPath($RepositoryRoot)
if (-not $WorkDirectory) { $WorkDirectory = Join-Path $RepositoryRoot ('build/packaging/' + [Guid]::NewGuid()) }
$completion = Join-Path $WorkDirectory 'completed'; $readyDirectory = Join-Path $WorkDirectory 'archives'
$null = New-Item -ItemType Directory -Force -Path $completion, $readyDirectory
$manifest = Read-PackageManifest $RepositoryRoot
$commands = @()
foreach ($line in ($Project -split '&&|\r?\n' | Where-Object { $_.Trim() })) {
    $tokens = @([regex]::Matches($line, '"[^"]*"|[^\s"]+') | ForEach-Object { $_.Value.Trim('"') })
    if ($tokens[0] -notmatch '^msbuild(?:\.exe)?$' -or @($tokens | Where-Object { $_ -match '[&|<>]' }).Count -or $tokens -notcontains '/p:Configuration=Release') { throw 'Project input must contain only MSBuild Release commands separated by && or newlines.' }
    $commands += ,@($tokens | Select-Object -Skip 1)
}
$selected = @($manifest.packages)
if ($Packages) {
    $requested = @($Packages -split ',')
    foreach ($id in $requested) { if ($id -notin $manifest.packages.id) { throw "Unknown package: $id" } }
    $selected = @($selected | Where-Object { $_.id -in $requested -or @($_.associatedPackages | Where-Object { $_ -in $requested }).Count })
} elseif ($commands.Count) {
    $wanted = @{}
    foreach ($arguments in $commands) {
        $inputFile = @($arguments | Where-Object { $_ -match '\.(slnx|sln|vcxproj)$' } | Select-Object -First 1)
        if (-not $inputFile.Count) { throw 'No solution/project in MSBuild command.' }
        $solution = [IO.Path]::GetFileName($inputFile[0]).Split('.')[0]
        $targets = @($arguments | Where-Object { $_ -match '^/t:' } | ForEach-Object { $_.Substring(3) -split ';' })
        foreach ($target in $targets) {
            $leaf = ($target -split '[\\/]')[-1]
            $producer = @($manifest.packages.producers | Where-Object { $_.solution -eq $solution -and $_.project.Replace('.', '_') -eq $leaf })
            if ($producer.Count -ne 1) { throw "Unknown release target: $target" }
            $wanted[$producer[0].project] = $true
        }
        if (-not $targets.Count) {
            foreach ($producer in $manifest.packages.producers) {
                if (($inputFile[0] -match '\.vcxproj$' -and [IO.Path]::GetFileNameWithoutExtension($inputFile[0]) -eq $producer.project) -or ($inputFile[0] -notmatch '\.vcxproj$' -and $producer.solution -eq $solution)) { $wanted[$producer.project] = $true }
            }
        }
    }
    $chosen = @($selected | Where-Object { @($_.producers | Where-Object { $wanted.ContainsKey($_.project) }).Count }).id
    $selected = @($selected | Where-Object { $_.id -in $chosen -or @($_.associatedPackages | Where-Object { $_ -in $chosen }).Count })
}
if ($Available) {
    if (-not $Existing -or $Packages -or $Project) { throw '-Available is only valid with -Existing and without another selection.' }
    $present = @($selected | Where-Object { $_.producers.Count -and -not @($_.producers | Where-Object { -not (Test-Path -LiteralPath (Resolve-PackagePath $RepositoryRoot $_.source)) }).Count }).id
    $selected = @($selected | Where-Object { $_.id -in $present -or @($_.associatedPackages | Where-Object { $_ -in $present }).Count })
}
if (-not $selected.Count) { throw 'No packages selected.' }
if ($Existing -and $Mode -ne 'after-build') { throw 'as-ready requires coordinated builds.' }
if ($Signing -eq 'required' -and (-not $env:CODE_SIGNING_PFX -or -not $env:CODE_SIGNING_PASSWORD)) { throw 'Signing is required but its credentials are missing.' }
$producers = @{}; foreach ($producer in $selected.producers) { $producers[$producer.project] = $producer }
foreach ($package in $selected) {
    foreach ($id in $package.associatedPackages) {
        foreach ($producer in ($manifest.packages | Where-Object { $_.id -eq $id }).producers) { $producers[$producer.project] = $producer }
    }
}
$state = @{ Completed = @{}; Scheduled = @{}; Workers = New-Object Collections.ArrayList; Results = New-Object Collections.ArrayList }
$pool = [System.Management.Automation.Runspaces.RunspaceFactory]::CreateRunspacePool(1, 2)
$pool.Open()
$building = $null; $fatal = $null
function Test-Ready($Package) {
    foreach ($producer in $Package.producers) { if (-not $state.Completed.ContainsKey($producer.project)) { return $false } }
    foreach ($id in $Package.associatedPackages) {
        foreach ($producer in ($manifest.packages | Where-Object { $_.id -eq $id }).producers) { if (-not $state.Completed.ContainsKey($producer.project)) { return $false } }
    }
    return $true
}
function Read-Completions {
    foreach ($file in Get-ChildItem -LiteralPath $completion -Filter '*.ready') {
        $record = [IO.File]::ReadAllText($file.FullName).Trim([char]0xfeff, [char]13, [char]10).Split('|')
        if (-not $producers.ContainsKey($record[0])) { continue }
        if ($record.Count -ne 5 -or $record[1] -ne 'Release') { throw "Invalid build completion: $($file.Name)" }
        $output = if ($record[4]) {$record[4]} else {$record[3]}
        if ([IO.Path]::GetFullPath($output) -ne (Resolve-PackagePath $RepositoryRoot $producers[$record[0]].source)) { throw "Build output differs from manifest: $($record[0])" }
        $state.Completed[$record[0]] = $true
    }
}
function Complete-Workers {
    foreach ($worker in @($state.Workers)) {
        if (-not $worker.Invocation.IsCompleted) { continue }
        try {
            $values = @($worker.Shell.EndInvoke($worker.Invocation))
            if ($worker.Shell.Streams.Error.Count) { throw $worker.Shell.Streams.Error[0] }
            if ($values.Count -ne 1) { throw "Invalid packaging result: $($worker.Id)" }
            $result = $values[0]
            if (-not $result.skipped) {
                $null = $state.Results.Add($result)
                $pending = Join-Path $readyDirectory "$($worker.Id).pending"
                [IO.File]::WriteAllText($pending, ($result | ConvertTo-Json -Depth 10))
                [IO.File]::Move($pending, (Join-Path $readyDirectory "$($worker.Id).ready.json"))
                Write-Host "Ready: $($worker.Id).zip"
            }
        } finally { $worker.Shell.Dispose(); $state.Workers.Remove($worker) }
    }
}
function Start-ReadyPackages {
    if (Test-Path -LiteralPath (Join-Path $WorkDirectory 'upload-failed.json')) { throw 'An artifact upload failed.' }
    Complete-Workers
    foreach ($package in $selected) {
        if ($state.Workers.Count -ge 2) { break }
        if ($state.Scheduled.ContainsKey($package.id) -or -not (Test-Ready $package)) { continue }
        $state.Scheduled[$package.id] = $true
        Write-Host "Packaging $($package.id)"
        $shell = [PowerShell]::Create(); $shell.RunspacePool = $pool
        $null = $shell.AddScript('param($module, $manifest, $package, $root, $work, $signing); $ErrorActionPreference="Stop"; Import-Module $module -DisableNameChecking; Invoke-ManifestPackage $manifest $package $root $work $signing')
        foreach ($argument in @((Join-Path $PSScriptRoot 'Packaging.psm1'), $manifest, $package, $RepositoryRoot, $WorkDirectory, $Signing)) { $null = $shell.AddArgument($argument) }
        $null = $state.Workers.Add(@{ Shell = $shell; Invocation = $shell.BeginInvoke(); Id = $package.id })
    }
}
function Get-Platform($Producer) {
    switch ($Producer.solution) { 'Win64' {'x64'} 'Dolphin' {'x64'} 'PCSX2F' {'PS2'} 'PPSSPP' {'PSP'} default {'Win32'} }
}
function Build-Plan([string[]]$Arguments) {
    Write-Host "Building $($Arguments -join ' ')"
    $script:building = Start-PackageTool 'msbuild' ($Arguments + @('/p:Configuration=Release', '/p:PostBuildEventUseInBuild=false', "/p:ForceImportAfterCppTargets=$PSScriptRoot/completion.targets", "/p:WFPCompletionDirectory=$completion")) $RepositoryRoot
    while (-not $script:building.Process.HasExited) {
        if (Test-Path -LiteralPath (Join-Path $WorkDirectory 'upload-failed.json')) { throw 'An artifact upload failed.' }
        Read-Completions
        if ($Mode -eq 'as-ready') { Start-ReadyPackages }
        Start-Sleep -Milliseconds 200
    }
    $tool = $script:building; $script:building = $null
    $bytes = Complete-PackageTool $tool
    [IO.File]::WriteAllBytes((Join-Path $WorkDirectory ('build-' + [Guid]::NewGuid() + '.log')), $bytes)
    Read-Completions
    if ($Mode -eq 'as-ready') { Start-ReadyPackages }
}
try {
    if (-not $Existing -and -not $NoDownload) { Invoke-DependencyDownloads $manifest $selected $RepositoryRoot (Join-Path $WorkDirectory 'downloads') }
    foreach ($package in $selected) { Remove-Item -LiteralPath (Join-Path $RepositoryRoot "data/Archives/$($package.id).zip") -Force -ErrorAction SilentlyContinue }
    if ($Existing) { foreach ($name in $producers.Keys) { $state.Completed[$name] = $true } }
    else {
        $known = @{}; foreach ($producer in $manifest.packages.producers) { $known[$producer.project] = $true }
        foreach ($solution in $manifest.solutions) {
            [xml]$xml = Get-Content -LiteralPath (Join-Path $RepositoryRoot "build/$solution.WidescreenFixesPack.slnx") -Raw
            foreach ($item in $xml.SelectNodes('//Project')) {
                $name = [IO.Path]::GetFileNameWithoutExtension($item.Path)
                if ($name -notlike 'ReleaseDependencies.*' -and -not $known.ContainsKey($name)) { throw "Project has no packaging rule: $name" }
            }
        }
        if ($commands.Count) { foreach ($arguments in $commands) { Build-Plan $arguments } }
        elseif ($Packages) {
            foreach ($producer in @($producers.Values | Sort-Object project)) { Build-Plan @('-m', "build/$($producer.project).vcxproj", "/p:Platform=$(Get-Platform $producer)") }
        } else { foreach ($solution in $manifest.solutions) { Build-Plan @('-m', "build/$solution.WidescreenFixesPack.slnx") } }
        foreach ($producer in $producers.Values) {
            if (-not $state.Completed.ContainsKey($producer.project)) { Build-Plan @('-m', "build/$($producer.project).vcxproj", "/p:Platform=$(Get-Platform $producer)") }
        }
        foreach ($name in $producers.Keys) { if (-not $state.Completed.ContainsKey($name)) { throw "Build completion record is missing: $name" } }
    }
    while ($state.Scheduled.Count -lt $selected.Count -or $state.Workers.Count) { Start-ReadyPackages; Start-Sleep -Milliseconds 100 }
} catch { $fatal = $_ }
finally {
    if ($script:building) {
        try { Stop-PackageTool $script:building; $null = Complete-PackageTool $script:building } catch { if (-not $fatal) { $fatal = $_ } }
    }
    # Completed packages remain uploadable on failure; wait for all started work.
    while ($state.Workers.Count) {
        try { Complete-Workers } catch { if (-not $fatal) { $fatal = $_ } }
        Start-Sleep -Milliseconds 100
    }
    $pool.Close(); $pool.Dispose()
}
if ($fatal) { throw $fatal }
if (-not $state.Results.Count) { throw 'No package archives were produced.' }
[IO.File]::WriteAllText((Join-Path $WorkDirectory 'summary.json'), (@{ mode = $Mode; results = @($state.Results) } | ConvertTo-Json -Depth 20))
Write-Host "Packaged $($state.Results.Count) archives."
