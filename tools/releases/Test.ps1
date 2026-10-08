$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $PSScriptRoot 'Selection.psm1') -Force
Import-Module (Join-Path $root 'tools/packaging/Packaging.psm1') -Force -DisableNameChecking
function Assert($Condition, [string]$Message) { if (!$Condition) { throw $Message } }
function Assert-Fails([scriptblock]$Action, [string]$Pattern) {
    $failure=$null
    try { & $Action } catch { $failure=$_ }
    Assert ($failure -and "$failure" -match $Pattern) "Expected failure '$Pattern', got '$failure'."
}
$catalog=Read-ReleaseCatalog $root
$all=Resolve-ReleaseSelection $catalog 'all'
Assert ($all.Tags.Count -eq $catalog.releases.Count) 'All supported releases should be selectable.'
Assert ('dmc3se' -notin $all.Tags -and 'mclarpsp' -in $all.Tags) 'DMC3 removal/Midnight Club addition failed.'
$psp=Resolve-ReleaseSelection $catalog @($catalog.releases | Where-Object { 'PPSSPP' -in $_.platforms } | ForEach-Object tag)
Assert ('MidnightClubLARemix.PPSSPP.FusionFix' -in $psp.Packages) 'PSP selection excludes Midnight Club.'
$selection=Resolve-ReleaseSelection $catalog @('gtalcspsp,gtavcspsp', 'gtalcspsp')
Assert ($selection.Tags.Count -eq 2 -and $selection.Packages.Count -eq 5) 'Bundle selection or deduplication failed.'
$frontends=Resolve-ReleaseSelection $catalog @('gta3','gtavc')
Assert ('GTA3.WidescreenFrontend' -notin $frontends.Packages -and 'GTAVC.WidescreenFrontend' -notin $frontends.Packages) 'Static frontend archives must be retained, not rebuilt.'
Assert-Fails { Resolve-ReleaseSelection $catalog 'dmc3se' } 'Unknown'
Assert-Fails { Resolve-ReleaseSelection $catalog @() } 'at least one'
Assert-Fails { Resolve-ReleaseSelection $catalog 'all,bully' } 'by itself'
$hash=Get-ReleaseCatalogHash $root
$request=New-ReleaseDispatch $catalog $selection.Tags 'ThirteenAG/WidescreenFixesPack' 'master' $hash '0123456789abcdef'
Assert (($request.Json | ConvertFrom-Json).tag_list -eq 'gtalcspsp,gtavcspsp') 'Wrong dispatch inputs.'
Assert-Fails { New-ReleaseDispatch $catalog 'bully' 'bad/repo/extra' 'master' $hash '0123456789abcdef' } 'owner/name'
Assert-Fails { New-ReleaseDispatch $catalog 'bully' 'owner/repo' '--unsafe' $hash '0123456789abcdef' } 'branch or tag'
$utf8=New-Object Text.UTF8Encoding($false)
Assert ((Get-ReleaseCatalogContentHash $utf8.GetBytes("a`r`nb`r`n")) -eq (Get-ReleaseCatalogContentHash $utf8.GetBytes("a`nb`n"))) 'Git line-ending normalization changes catalog hash.'

# Every selectable catalog entry must publish exactly its declared packages.
$workflow=Get-Content (Join-Path $root '.github/workflows/all.yml') -Raw
$blocks=[regex]::Matches($workflow, '(?m)^    - name:[\s\S]*?(?=^    - name:|\z)')
$uploads=@{}
foreach ($block in $blocks) {
    if ($block.Value -notmatch 'uses: \./\.github/workflows/release_tag') { continue }
    $tag=[regex]::Match($block.Value,'(?m)^        tag: (\S+)').Groups[1].Value
    $packages=@([regex]::Match($block.Value,'(?m)^        artifacts: (.+)').Groups[1].Value.Trim() -split ',\s*' | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_) } | Sort-Object)
    Assert (!$uploads.ContainsKey($tag)) "Duplicate release upload $tag."
    $uploads[$tag]=$packages
}
Assert ($uploads.Count -eq $catalog.releases.Count) 'Catalog and workflow release counts differ.'
foreach ($entry in $catalog.releases) {
    Assert (($uploads[$entry.tag] -join ',') -eq (@($entry.packages | Sort-Object) -join ',')) "Upload/catalog mismatch: $($entry.tag)."
    Assert (Test-Path (Join-Path $root ".github/docs/$($entry.tag).md")) "Missing release description: $($entry.tag)."
}
$work=Join-Path $root ('build/release-picker-tests/' + [Guid]::NewGuid())
$null=New-Item -ItemType Directory -Path $work -Force
$manifest=Read-PackageManifest $root
$chosen=@($manifest.packages | Where-Object { $_.id -in $selection.Packages } | ForEach-Object producers)
$originalHash=(Get-FileHash (Join-Path $root 'build/PPSSPP.WidescreenFixesPack.slnx')).Hash
$plans=@(New-SelectedSolutionPlans $manifest $chosen $root $work)
Assert ($plans.Count -eq 1 -and $plans[0].Count -eq 2 -and $plans[0][0] -eq '-m') 'Single-solution build arguments were flattened.'
[xml]$xml=Get-Content $plans[0][1] -Raw
$projects=@($xml.SelectNodes('//Project') | ForEach-Object { [IO.Path]::GetFileNameWithoutExtension($_.Path) } | Sort-Object)
Assert (($projects -join ',') -eq (@($chosen.project | Sort-Object) -join ',')) 'Filtered solution has wrong producers.'
Assert ($originalHash -eq (Get-FileHash (Join-Path $root 'build/PPSSPP.WidescreenFixesPack.slnx')).Hash) 'Original solution was modified.'
$native=@($manifest.packages | Where-Object id -in @('Bully.WidescreenFix','GTAVCDE.FusionFix') | ForEach-Object producers)
$plans=@(New-SelectedSolutionPlans $manifest $native $root $work)
Assert ($plans.Count -eq 2) 'Cross-architecture solution selection failed.'
foreach ($plan in $plans) {
    [xml]$xml=Get-Content $plan[1] -Raw
    Assert (@($xml.SelectNodes('//Project') | Where-Object { $_.Path -like '*ReleaseDependencies.*' }).Count -gt 0) 'Shared compilation dependency omitted.'
    foreach ($dependency in $xml.SelectNodes('//BuildDependency')) {
        Assert ($dependency.Project -in @($xml.SelectNodes('//Project') | ForEach-Object Path)) 'Solution dependency path does not match a retained project.'
    }
}

# Show only our own test window offscreen; never invoke the network controls.
$ui = . (Join-Path $PSScriptRoot 'ReleaseGui.ps1') -TestMode -NoNetwork
try {
    $ui.Form.WindowStartupLocation='Manual'; $ui.Form.Left=-32000; $ui.Form.Top=-32000; $ui.Form.ShowInTaskbar=$false
    $ui.Form.Show(); $ui.Form.UpdateLayout()
    $ui.Categories.SelectedIndex=3
    $ui.SelectShown.RaiseEvent((New-Object Windows.RoutedEventArgs([Windows.Controls.Primitives.ButtonBase]::ClickEvent)))
    Assert ($ui.State.Selected.Count -eq $psp.Tags.Count) 'Platform select-all failed.'
    $ui.Form.UpdateLayout()
    Assert ($ui.Grid.Items[0].Chosen) 'Bulk selection did not update the bound release model.'
    $ui.Categories.SelectedIndex=1
    Assert ($ui.State.Selected.Count -eq $psp.Tags.Count) 'Selections did not persist across filters.'
    $ui.Categories.SelectedIndex=0; $ui.Search.Text='Midnight'
    Assert ($ui.Grid.Items.Count -eq 1 -and $ui.Grid.Items[0].tag -eq 'mclarpsp') 'Search failed.'
    Assert ($ui.Detail.Text -match 'MidnightClubLARemix.PPSSPP.FusionFix') 'Highlighted package details missing.'
    $ui.ClearShown.RaiseEvent((New-Object Windows.RoutedEventArgs([Windows.Controls.Primitives.ButtonBase]::ClickEvent)))
    Assert (!$ui.State.Selected.ContainsKey('mclarpsp')) 'Clear shown failed.'
    function Find-Checkbox([Windows.DependencyObject]$Parent) {
        if ($Parent -is [Windows.Controls.CheckBox]) { return $Parent }
        for ($index=0; $index -lt [Windows.Media.VisualTreeHelper]::GetChildrenCount($Parent); $index++) {
            $found=Find-Checkbox ([Windows.Media.VisualTreeHelper]::GetChild($Parent,$index))
            if ($found) { return $found }
        }
    }
    $ui.Form.UpdateLayout()
    $check=Find-Checkbox $ui.Grid
    Assert ($check -and $check.DataContext.tag -eq 'mclarpsp') 'Release checkbox is not rendered/bound.'
    $check.IsChecked=$true
    $check.RaiseEvent((New-Object Windows.RoutedEventArgs([Windows.Controls.Primitives.ButtonBase]::ClickEvent)))
    Assert ($ui.State.Selected.ContainsKey('mclarpsp')) 'Checkbox selection failed.'
    $ui.SelectedOnly.IsChecked=$true
    $ui.Form.UpdateLayout(); $check=Find-Checkbox $ui.Grid
    $check.IsChecked=$false
    $check.RaiseEvent((New-Object Windows.RoutedEventArgs([Windows.Controls.Primitives.ButtonBase]::ClickEvent)))
    $ui.Form.Dispatcher.Invoke([Action]{}, [Windows.Threading.DispatcherPriority]::Background)
    Assert ($ui.Grid.Items.Count -eq 0) 'Only-selected view kept an unchecked release.'
    Assert (!$ui.Detail.Text) 'Empty filtered view kept stale release details.'
    Assert (!$ui.Publish.IsEnabled) 'Offline test unexpectedly enables dispatch.'
} finally { $ui.Form.Close(); $ui.Timer.Stop() }
Write-Host "Release picker checks passed: $($all.Tags.Count) releases, all platforms, workflow mappings, filtered builds and GUI selection."
