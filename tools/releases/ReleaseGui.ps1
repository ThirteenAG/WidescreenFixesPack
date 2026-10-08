[CmdletBinding()]
param([string]$Repository = '', [switch]$NoNetwork, [string]$RenderPath = '', [switch]$TestMode)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
Import-Module (Join-Path $PSScriptRoot 'Selection.psm1') -Force
Import-Module (Join-Path $root 'tools/packaging/Packaging.psm1') -Force -DisableNameChecking
Add-Type -AssemblyName PresentationFramework, PresentationCore, WindowsBase
$reader=[Xml.XmlReader]::Create((Join-Path $PSScriptRoot 'ReleaseGui.xaml'))
try { $form=[Windows.Markup.XamlReader]::Load($reader) } finally { $reader.Close() }
$catalog=Read-ReleaseCatalog $root
$catalogHash=Get-ReleaseCatalogHash $root
if (!$Repository) {
    $remote = & git -C $root remote get-url origin 2>$null
    if ($remote -match 'github\.com[:/]([^/]+/[^/]+?)(?:\.git)?$') { $Repository=$Matches[1] }
}
$gh=Get-Command gh.exe -ErrorAction SilentlyContinue
if (!$gh -and (Test-Path 'C:\Program Files\GitHub CLI\gh.exe')) { $gh=Get-Item 'C:\Program Files\GitHub CLI\gh.exe' }
$ghPath=if ($gh) { if ($gh.Source) { $gh.Source } else { $gh.FullName } } else { '' }
$state=@{ Selected=@{}; Refreshing=$false; Pending=$null; Busy=$false; Dispatching=$false; RunUrl=''; SearchRun=$false; RequestId=''; PollCount=0 }
$platformNames=@{ Win32='Windows x86'; Win64='Windows x64'; PPSSPP='PSP / PPSSPP'; PCSX2F='PS2 / PCSX2'; Dolphin='GameCube / Dolphin'; CXBXR='Xbox / Cxbx' }
$platformBadges=@{ Win32='Windows x86'; Win64='Windows x64'; PPSSPP='PSP'; PCSX2F='PS2'; Dolphin='GameCube'; CXBXR='Xbox' }
foreach ($release in $catalog.releases) {
    $release | Add-Member -NotePropertyName Chosen -NotePropertyValue $false
    $release | Add-Member -NotePropertyName PlatformLabel -NotePropertyValue (@($release.platforms | ForEach-Object { $platformBadges[$_] }) -join ' + ')
    $release | Add-Member -NotePropertyName PlatformDetail -NotePropertyValue (@($release.platforms | ForEach-Object { $platformNames[$_] }) -join ' + ')
}
$grid=$form.FindName('Releases'); $categories=$form.FindName('Categories'); $search=$form.FindName('Search'); $searchHint=$form.FindName('SearchHint')
$selectedOnly=$form.FindName('SelectedOnly'); $repoBox=$form.FindName('Repo'); $repoBox.Text=$Repository; $refBox=$form.FindName('Ref')
$connect=$form.FindName('Connect'); $signIn=$form.FindName('SignIn'); $connection=$form.FindName('Connection')
$selectAll=$form.FindName('SelectAll'); $clearAll=$form.FindName('ClearAll'); $selectShown=$form.FindName('SelectShown'); $clearShown=$form.FindName('ClearShown')
$publish=$form.FindName('Publish'); $openRun=$form.FindName('OpenRun'); $review=$form.FindName('Review'); $detail=$form.FindName('Detail'); $countLabel=$form.FindName('Count'); $status=$form.FindName('Status')
$categoryItems=@([pscustomobject]@{Key='all'; Name="All releases     $($catalog.releases.Count)"})
foreach ($platform in @('Win32','Win64','PPSSPP','PCSX2F','Dolphin','CXBXR')) {
    $total=@($catalog.releases | Where-Object { $platform -in $_.platforms }).Count
    $categoryItems += [pscustomobject]@{Key=$platform; Name="$($platformNames[$platform])     $total"}
}
if (@($catalog.releases | Where-Object { !$_.available }).Count) { $categoryItems += [pscustomobject]@{Key='unavailable';Name='Unavailable / archived'} }
$categories.ItemsSource=$categoryItems; $categories.SelectedIndex=0
function Get-VisibleReleases {
    $key=$categories.SelectedItem.Key; $query=$search.Text.Trim()
    return @($catalog.releases | Where-Object {
        ($key -eq 'all' -or ($key -eq 'unavailable' -and !$_.available) -or $key -in $_.platforms) -and
        (!$selectedOnly.IsChecked -or $state.Selected.ContainsKey($_.tag)) -and
        (!$query -or ($_.name+' '+$_.tag+' '+($_.packages -join ' ')).IndexOf($query,[StringComparison]::OrdinalIgnoreCase) -ge 0)
    })
}
function Update-Review {
    $selection=@($catalog.releases | Where-Object { $state.Selected.ContainsKey($_.tag) })
    $packages=@($selection | ForEach-Object { $_.packages } | Sort-Object -Unique)
    $countLabel.Text="$($selection.Count) releases selected  /  $($packages.Count) packages"
    $review.Text=if ($selection.Count) { "Repository: $($repoBox.Text)`r`nSource: $($refBox.Text)`r`n`r`n" + (($selection | ForEach-Object { "$($_.name)`r`n$($_.packages.Count) package(s)" }) -join "`r`n`r`n") } else { "Nothing selected yet.`r`n`r`nChoose a platform and select individual releases, or use Select shown.`r`n`r`nYour selection stays checked when you change filters." }
    $publish.IsEnabled=(!$state.Busy -and !!$ghPath -and $selection.Count -gt 0 -and !$NoNetwork)
    $publish.Content=if ($selection.Count) { "Publish $($selection.Count) releases  " + [char]0x2192 } else { 'Publish releases  ' + [char]0x2192 }
}
function Update-Highlight {
    $detail.Text=''
    $release=$grid.SelectedItem
    if (!$release) { return }
    $detail.Text="$($release.name)`r`nTag: $($release.tag)`r`n`r`nPackages:`r`n$($release.packages -join "`r`n")"
    if (!$release.available) { $detail.Text+="`r`n`r`nUnavailable: $($release.reason)" }
    if ($release.unmanagedAssets.Count) { $detail.Text+="`r`n`r`nExisting assets retained, not rebuilt:`r`n$($release.unmanagedAssets -join "`r`n")" }
}
function Refresh-View {
    $highlight=if ($grid.SelectedItem) { $grid.SelectedItem.tag } else { '' }
    $state.Refreshing=$true
    try {
        foreach ($release in $catalog.releases) { $release.Chosen=$state.Selected.ContainsKey($release.tag) }
        $grid.ItemsSource=@(Get-VisibleReleases)
        $same=@($grid.ItemsSource | Where-Object tag -eq $highlight)
        if ($same.Count) { $grid.SelectedItem=$same[0] } elseif ($grid.Items.Count) { $grid.SelectedIndex=0 }
        $searchHint.Visibility=if ($search.Text) { 'Collapsed' } else { 'Visible' }
    } finally { $state.Refreshing=$false }
    Update-Review; Update-Highlight
}
function Set-ShownSelection([bool]$Value) {
    foreach ($release in (Get-VisibleReleases)) {
        if (!$release.available) { continue }
        if ($Value) { $state.Selected[$release.tag]=$true } else { $state.Selected.Remove($release.tag) }
    }
    Refresh-View
}
function Set-Busy([bool]$Value) {
    $state.Busy=$Value
    foreach ($control in @($connect,$repoBox,$refBox,$signIn,$grid,$selectAll,$clearAll,$selectShown,$clearShown)) { $control.IsEnabled=!$Value }
    Update-Review
}
# Nonblocking GitHub CLI requests and workflow-run tracking.
. (Join-Path $PSScriptRoot 'Github.ps1')
$grid.AddHandler([Windows.Controls.Primitives.ButtonBase]::ClickEvent, [Windows.RoutedEventHandler]{
    param($sender,$event)
    $check=$event.OriginalSource
    if ($check -isnot [Windows.Controls.CheckBox] -or $state.Refreshing) { return }
    $release=$check.DataContext
    if (!$release -or !$release.available) { return }
    if ($check.IsChecked) { $state.Selected[$release.tag]=$true } else { $state.Selected.Remove($release.tag) }
    Update-Review
    if ($selectedOnly.IsChecked) { $null=$form.Dispatcher.BeginInvoke([Action]{ Refresh-View }) }
}, $true)
$grid.Add_SelectionChanged({ if (!$state.Refreshing) { Update-Highlight } })
$categories.Add_SelectionChanged({ Refresh-View }); $search.Add_TextChanged({ Refresh-View })
$selectedOnly.Add_Checked({ Refresh-View }); $selectedOnly.Add_Unchecked({ Refresh-View })
$repoBox.Add_TextChanged({ Update-Review })
$refBox.AddHandler([Windows.Controls.Primitives.TextBoxBase]::TextChangedEvent, [Windows.RoutedEventHandler]{ Update-Review })
$selectShown.Add_Click({ Set-ShownSelection $true }); $clearShown.Add_Click({ Set-ShownSelection $false })
$selectAll.Add_Click({ foreach ($release in $catalog.releases) { if ($release.available) { $state.Selected[$release.tag]=$true } }; Refresh-View })
$clearAll.Add_Click({ $state.Selected.Clear(); Refresh-View })
$connect.Add_Click({
    try {
        $null=New-ReleaseDispatch $catalog @('bully') $repoBox.Text $refBox.Text $catalogHash '0000000000000000'
        Set-Busy $true; $status.Text='Connecting to GitHub...'; $status.Foreground=[Windows.Media.Brushes]::Gray
        Start-Gh @('api',"repos/$($repoBox.Text)") {
            param($output)
            $repo=$output | ConvertFrom-Json
            if (!$repo.permissions.push -and !$repo.permissions.admin) { throw 'Your GitHub login does not have write access to this repository.' }
            $connection.Text="Connected to $($repo.full_name). Default branch: $($repo.default_branch)."
            if (!$refBox.Text -or $refBox.Text -eq 'master') { $refBox.Text=$repo.default_branch }
            Start-Gh @('api',"repos/$($repoBox.Text)/branches?per_page=100",'--paginate','--jq','.[].name') {
                param($output)
                $refBox.Items.Clear(); foreach ($branch in ($output -split '\r?\n' | Where-Object { $_ })) { $null=$refBox.Items.Add($branch) }
                $status.Text='Connected. Choose releases and the source branch.'; Set-Busy $false
            }
        }
    } catch { Show-Failure $_.Exception.Message }
})
$signIn.Add_Click({
    if (!$ghPath) { Start-Process 'https://cli.github.com/'; return }
    Start-Process -FilePath $ghPath -ArgumentList @('auth','login','--hostname','github.com','--git-protocol','https','--web')
    $status.Text='Finish signing in, then click Connect.'
})
$publish.Add_Click({
    try {
        $null=New-ReleaseDispatch $catalog @($state.Selected.Keys) $repoBox.Text $refBox.Text $catalogHash '0000000000000000'
        Set-Busy $true; $status.Foreground=[Windows.Media.Brushes]::Gray; Validate-RemoteCatalog
    } catch { Show-Failure $_.Exception.Message }
})
$openRun.Add_Click({ if ($state.RunUrl) { Start-Process $state.RunUrl } })
$form.Add_Closing({ param($sender,$event)
    if ($state.Dispatching) { $event.Cancel=$true; $status.Text='Wait for the dispatch result before closing.'; return }
    $timer.Stop()
    if ($state.Pending) {
        if (!$state.Pending.Process.HasExited) { $state.Pending.Process.Kill() }
        $state.Pending.Process.Dispose(); $state.Pending=$null
    }
})
$form.Add_PreviewKeyDown({ param($sender,$event)
    if ([Windows.Input.Keyboard]::Modifiers -eq 'Control' -and $event.Key -eq 'F') { $null=$search.Focus(); $event.Handled=$true }
})
Refresh-View
if (!$ghPath) { $connection.Text='GitHub CLI is missing. Install it using Sign in, then reopen this picker.' }
if ($TestMode) { return @{Form=$form; Grid=$grid; State=$state; Categories=$categories; Search=$search; SelectShown=$selectShown; ClearShown=$clearShown; SelectedOnly=$selectedOnly; Publish=$publish; Ref=$refBox; Repo=$repoBox; Review=$review; Detail=$detail; Catalog=$catalog; Timer=$timer} }
if ($RenderPath) {
    $form.WindowStartupLocation='Manual'; $form.Left=-32000; $form.Top=-32000; $form.ShowInTaskbar=$false
    $form.Show(); $form.UpdateLayout()
    $visual=$form.Content
    $bitmap=New-Object Windows.Media.Imaging.RenderTargetBitmap([int]$visual.ActualWidth,[int]$visual.ActualHeight,96,96,[Windows.Media.PixelFormats]::Pbgra32)
    $bitmap.Render($visual)
    $encoder=New-Object Windows.Media.Imaging.PngBitmapEncoder
    $encoder.Frames.Add([Windows.Media.Imaging.BitmapFrame]::Create($bitmap))
    $stream=[IO.File]::Create([IO.Path]::GetFullPath($RenderPath))
    try { $encoder.Save($stream) } finally { $stream.Dispose(); $form.Close() }
    return
}
$form.Add_ContentRendered({ $timer.Start(); if (!$NoNetwork -and $ghPath) { $connect.RaiseEvent((New-Object Windows.RoutedEventArgs([Windows.Controls.Primitives.ButtonBase]::ClickEvent))) } })
$null=$form.ShowDialog()
