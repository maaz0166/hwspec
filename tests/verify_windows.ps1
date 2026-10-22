# Run from Windows PowerShell 5.1 or PowerShell 7; no Python/CMake required.
# Captures contain local hardware identifiers and addresses; summary.json is compact.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string]$OutputDirectory = "$PSScriptRoot/../build/windows-acceptance"
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Windows x64 is required.' }
$exePath = (Resolve-Path -LiteralPath $Executable).Path
$runDirectory = Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$null = New-Item -ItemType Directory -Path $runDirectory -Force
$checks = New-Object 'System.Collections.Generic.List[object]'
$captures = @{}
$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = ([Security.Principal.WindowsPrincipal]$identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)

function Record-Check([string]$Name, [bool]$Passed) {
    $checks.Add([pscustomobject]@{ name = $Name; passed = $Passed })
}

function Invoke-Capture([string]$Name, [string[]]$Flags, [int]$ExpectedExitCode = 0) {
    $startInfo = New-Object Diagnostics.ProcessStartInfo
    $startInfo.FileName = $exePath
    # Only fixed ASCII flags below are passed; no shell is involved.
    $startInfo.Arguments = $Flags -join ' '
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = [Text.Encoding]::UTF8
    $startInfo.StandardErrorEncoding = [Text.Encoding]::UTF8
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $startInfo
    try {
        if (-not $process.Start()) { throw "Could not start $Name" }
        $process.StandardInput.Close()
        $stdoutTask = $process.StandardOutput.ReadToEndAsync()
        $stderrTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(35000)) {
            $process.Kill()
            throw "$Name exceeded 35 seconds"
        }
        $stdout = $stdoutTask.GetAwaiter().GetResult()
        $stderr = $stderrTask.GetAwaiter().GetResult()
        [IO.File]::WriteAllText((Join-Path $runDirectory "$Name.stdout.txt"), $stdout)
        [IO.File]::WriteAllText((Join-Path $runDirectory "$Name.stderr.txt"), $stderr)
        Record-Check "$Name exit" ($process.ExitCode -eq $ExpectedExitCode)
        Record-Check "$Name stderr" (($ExpectedExitCode -ne 0) -or [string]::IsNullOrEmpty($stderr))
        return $stdout
    } finally { $process.Dispose() }
}

$version = (Invoke-Capture 'version' @('--version')).Trim()
$null = Invoke-Capture 'help' @('--help')
$null = Invoke-Capture 'invalid-option' @('--invalid-acceptance-option') 2
$sectionNames = @('cpu', 'mem', 'os', 'gpu', 'disk', 'net', 'battery')
foreach ($section in $sectionNames) {
    foreach ($detail in @($false, $true)) {
        $mode = if ($detail) { 'detail' } else { 'normal' }
        $flags = @("--$section")
        if ($detail) { $flags += '-d' }
        $text = Invoke-Capture "$section-$mode-text" $flags
        Record-Check "$section-$mode text present" (-not [string]::IsNullOrWhiteSpace($text))
        $json = Invoke-Capture "$section-$mode-json" ($flags + '--json') | ConvertFrom-Json
        Record-Check "$section-$mode selection" (($json.PSObject.Properties.Name -join ',') -eq $section)
        $captures["$section-$mode"] = $json.$section
    }
}
$all = Invoke-Capture 'all-detail-json' @('--all', '-d', '--json') | ConvertFrom-Json
Record-Check 'all sections' (($all.PSObject.Properties.Name -join ',') -eq ($sectionNames -join ','))
$null = Invoke-Capture 'default-text' @()
$skipped = Invoke-Capture 'skip-admin-pause-json' @('--disk', '-d', '--skip-admin', '--pause', '--json') | ConvertFrom-Json
Record-Check 'skip-admin status' ($skipped.disk.disk_health.status -eq 'requires_admin')
Record-Check 'skip-admin no predictions' (@($skipped.disk.disk_health.devices).Count -eq 0)
$health = $captures['disk-detail'].disk_health
Record-Check 'disk health policy' $(if ($elevated) { $health.status -in @('available', 'unknown') } else { $health.status -eq 'requires_admin' })
Record-Check 'normal disk omits health' (-not ($captures['disk-normal'].PSObject.Properties.Name -contains 'disk_health'))
Record-Check 'unknown link speed sentinel omitted' (@($captures['net-detail'].adapters | Where-Object { $_.link_speed_bps -eq [uint64]::MaxValue }).Count -eq 0)

# CIM comparisons supplement, but do not replace, Task Manager/msinfo32 checks.
$processors = @(Get-CimInstance Win32_Processor)
$system = Get-CimInstance Win32_ComputerSystem
$os = Get-CimInstance Win32_OperatingSystem
$revision = (Get-ItemProperty 'HKLM:/SOFTWARE/Microsoft/Windows NT/CurrentVersion').UBR
Record-Check 'CIM CPU logical cores' ($captures['cpu-normal'].logical_cores -eq ($processors | Measure-Object NumberOfLogicalProcessors -Sum).Sum)
Record-Check 'CIM CPU physical cores' ($captures['cpu-normal'].physical_cores -eq ($processors | Measure-Object NumberOfCores -Sum).Sum)
Record-Check 'CIM CPU name' (@($processors | Where-Object { $_.Name.Trim() -eq $captures['cpu-normal'].name }).Count -gt 0)
Record-Check 'CIM visible memory' ($captures['mem-normal'].total_bytes -eq $system.TotalPhysicalMemory)
$memoryRows = @(Get-CimInstance Win32_PhysicalMemory)
foreach ($module in $captures['mem-detail'].modules) {
    $reference = $memoryRows | Where-Object { $_.DeviceLocator -eq $module.slot } | Select-Object -First 1
    if ($reference) {
        Record-Check 'CIM configured RAM speed' $(if ($reference.ConfiguredClockSpeed) { $module.speed_mhz -eq $reference.ConfiguredClockSpeed } else { $null -eq $module.speed_mhz })
    }
}
Record-Check 'CIM OS build plus revision' ($captures['os-normal'].build -eq ($os.BuildNumber + '.' + $revision))
if ($system.HypervisorPresent) {
    Record-Check 'hypervisor does not imply disabled firmware virtualization' ($captures['cpu-detail'].virtualization_enabled -ne $false)
}

$smartReference = @{ status = 'not_attempted'; predictions = @() }
if ($elevated) {
    try {
        $rows = @(Get-CimInstance -Namespace root/wmi -ClassName MSStorageDriver_FailurePredictStatus | Where-Object Active)
        $smartReference.status = 'queried'
        $smartReference.predictions = @($rows | Select-Object InstanceName, PredictFailure)
        $actual = @($health.devices | ForEach-Object { $_.instance_name + ':' + $_.failure_predicted } | Sort-Object)
        $expected = @($rows | ForEach-Object { $_.InstanceName + ':' + $_.PredictFailure } | Sort-Object)
        Record-Check 'CIM SMART predictions' (($actual -join '|') -eq ($expected -join '|'))
    } catch {
        $smartReference.status = 'provider_unavailable'
        $smartReference.error = $_.Exception.Message
        Record-Check 'unavailable SMART remains unknown' ($health.status -eq 'unknown')
    }
    # An elevated observer can read the native runas child's executable/arguments.
    $processes = @(Get-CimInstance Win32_Process -Filter "Name='hwspec.exe'" | Select-Object ProcessId, ParentProcessId, ExecutablePath, CommandLine)
    $processes | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 (Join-Path $runDirectory 'live-processes.json')
}
$smartReference | ConvertTo-Json -Depth 5 | Set-Content -Encoding UTF8 (Join-Path $runDirectory 'smart-reference.json')
$failed = @($checks | Where-Object { -not $_.passed })
$summary = [ordered]@{
    timestamp_utc = [DateTime]::UtcNow.ToString('o')
    executable = $exePath
    executable_sha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash
    version = $version
    elevated = $elevated
    os = $os.Caption
    os_build = $os.BuildNumber
    architecture = $os.OSArchitecture
    machine_model = $system.Model
    hypervisor_present = $system.HypervisorPresent
    virtualization_enabled = $captures['cpu-detail'].virtualization_enabled
    disk_health_status = $health.status
    smart_reference_status = $smartReference.status
    battery = $captures['battery-detail']
    checks = @($checks.ToArray())
    failed_checks = $failed.Count
    manual_ui_comparison = 'pending'
    interactive_uac = 'not_exercised_by_this_script'
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -Encoding UTF8 (Join-Path $runDirectory 'summary.json')
Write-Output "Results: $runDirectory"
Write-Output "Elevated: $elevated; checks: $($checks.Count); failures: $($failed.Count); SMART: $($health.status)"
if ($failed.Count) { $failed | Format-Table; exit 1 }
