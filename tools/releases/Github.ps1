function Start-Gh([string[]]$Arguments, [scriptblock]$Success, [string]$Json='') {
    if (!$ghPath) { throw 'Install GitHub CLI, then run gh auth login.' }
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName=$ghPath; $info.Arguments=ConvertTo-ToolArguments $Arguments; $info.WorkingDirectory=$root
    $info.UseShellExecute=$false; $info.CreateNoWindow=$true; $info.RedirectStandardOutput=$true; $info.RedirectStandardError=$true
    $info.RedirectStandardInput=!!$Json
    $process=New-Object Diagnostics.Process; $process.StartInfo=$info; $null=$process.Start()
    if ($Json) { $process.StandardInput.WriteLine($Json); $process.StandardInput.Close() }
    $state.Pending=@{Process=$process; Output=$process.StandardOutput.ReadToEndAsync(); Error=$process.StandardError.ReadToEndAsync(); Success=$Success; Started=[DateTime]::UtcNow}
}
function Show-Failure([string]$Message) {
    $status.Text=$Message; $status.Foreground=[Windows.Media.Brushes]::Firebrick
    Set-Busy $false
}
function Validate-RemoteCatalog {
    $status.Text='Checking the selected branch and release catalog...'
    $endpoint="repos/$($repoBox.Text)/contents/tools/releases/releases.json?ref=$([Uri]::EscapeDataString($refBox.Text))"
    Start-Gh @('api',$endpoint) {
        param($output)
        $remoteCatalog=$output | ConvertFrom-Json
        $bytes=[Convert]::FromBase64String($remoteCatalog.content -replace '\s','')
        if ((Get-ReleaseCatalogContentHash $bytes) -ne $catalogHash) { throw 'The selected branch has a different release catalog. Update your checkout and review the selection again.' }
        Start-Gh @('api',"repos/$($repoBox.Text)/actions/workflows/all.yml") {
            param($output)
            if (($output | ConvertFrom-Json).state -ne 'active') { throw 'The release workflow is not active in this repository.' }
            $state.RequestId=[Guid]::NewGuid().ToString('N').Substring(0,16)
            $request=New-ReleaseDispatch $catalog @($state.Selected.Keys) $repoBox.Text $refBox.Text $catalogHash $state.RequestId
            $state.Dispatching=$true; $status.Text='Submitting one release workflow...'
            Start-Gh $request.Arguments {
                param($output)
                $state.Dispatching=$false; $state.SearchRun=$true; $state.PollCount=0
                $state.RunUrl="https://github.com/$($repoBox.Text)/actions/workflows/all.yml"
                $openRun.IsEnabled=$true; $status.Foreground=[Windows.Media.Brushes]::SeaGreen
                $status.Text="Release workflow submitted. Request $($state.RequestId). Finding its run..."
            } $request.Json
        }
    }
}
$timer=New-Object Windows.Threading.DispatcherTimer; $timer.Interval=[TimeSpan]::FromMilliseconds(250)
$timer.Add_Tick({
    try {
        if ($state.Pending) {
            $pending=$state.Pending; $process=$pending.Process
            if (!$process.HasExited) {
                if (([DateTime]::UtcNow-$pending.Started).TotalSeconds -lt 60) { return }
                $process.Kill(); $process.WaitForExit(); $state.Pending=$null; $process.Dispose()
                if ($state.Dispatching) { $state.Dispatching=$false; throw 'Dispatch outcome unknown. Check GitHub Actions before retrying.' }
                throw 'GitHub request timed out. Check your connection and CLI login.'
            }
            $output=$pending.Output.GetAwaiter().GetResult(); $errorText=$pending.Error.GetAwaiter().GetResult(); $code=$process.ExitCode
            $state.Pending=$null; $process.Dispose()
            if ($code -ne 0) {
                $wasDispatching=$state.Dispatching; $state.Dispatching=$false
                if ($wasDispatching) { throw "Dispatch failed or its outcome is unknown. Check Actions before retrying.`r`n$errorText" }
                throw "$errorText`r`nIf these changes are new, commit and push the picker/workflow files first."
            }
            & $pending.Success $output
        } elseif ($state.SearchRun) {
            $state.PollCount++
            if ($state.PollCount % 8 -ne 0) { return }
            if ($state.PollCount -gt 120) {
                $state.SearchRun=$false; $status.Text='Submitted successfully. Use Open workflow run to find the run in Actions.'; Set-Busy $false; return
            }
            Start-Gh @('run','list','--repo',$repoBox.Text,'--workflow','all.yml','--event','workflow_dispatch','--limit','30','--json','displayTitle,url,status') {
                param($output)
                $run=@(($output | ConvertFrom-Json) | Where-Object { $_.displayTitle -eq "Release picker $($state.RequestId)" })
                if ($run.Count -eq 1) { $state.RunUrl=$run[0].url; $state.SearchRun=$false; $status.Text="Release workflow submitted ($($run[0].status)). Use Open workflow run to follow progress."; Set-Busy $false }
            }
        }
    } catch { $state.SearchRun=$false; Show-Failure $_.Exception.Message }
})
