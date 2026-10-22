# Live native integration check. Approve observer UAC, then application UAC.
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Python,
    [string]$OutputDirectory = "$PSScriptRoot/../build/native-uac-validation",
    [switch]$Observer
)
$ErrorActionPreference = 'Stop'
$exePath = (Resolve-Path -LiteralPath $Executable).Path
$pythonPath = (Resolve-Path -LiteralPath $Python).Path
$outputPath = [IO.Path]::GetFullPath($OutputDirectory)
$principal = [Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()
$elevated = $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
if ($Observer) {
    if (-not $elevated) { throw 'The observer must run elevated.' }
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(120)
        $testProcess = $null
        while ([DateTime]::UtcNow -lt $deadline) {
            $testProcess = Get-CimInstance Win32_Process -Filter "Name='hwspec.exe'" |
                Where-Object {
                    $_.ExecutablePath -eq $exePath -and
                    $_.CommandLine -match '--disk' -and $_.CommandLine -match '(?:^|\s)"?-d"?(?:\s|$)' -and
                    $_.CommandLine -match '--pause' -and $_.CommandLine -notmatch '--elevate'
                } | Sort-Object CreationDate -Descending | Select-Object -First 1
            if ($testProcess) { break }
            Start-Sleep -Milliseconds 500
        }
        if (-not $testProcess) { throw 'Timed out waiting for the native elevated child.' }
        $testProcess | Select-Object ProcessId,ParentProcessId,ExecutablePath,CommandLine |
            ConvertTo-Json | Set-Content -Encoding UTF8 (Join-Path $outputPath 'child-process.json')
        & $pythonPath (Join-Path $PSScriptRoot 'inspect_elevated_console.py') --pid $testProcess.ProcessId --section Disk --resume-selection --report (Join-Path $outputPath 'console.json')
        exit $LASTEXITCODE
    } catch {
        @{ passed = $false; error = $_.Exception.Message } | ConvertTo-Json |
            Set-Content -Encoding UTF8 (Join-Path $outputPath 'console.json')
        exit 1
    }
}
if ($elevated) { throw 'Start from a standard-user process to exercise UAC.' }
$outputPath = Join-Path $outputPath ([DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss-fff'))
$null = New-Item -ItemType Directory -Path $outputPath -Force
$observerArgs = @('-NoProfile','-File',('"' + $PSCommandPath + '"'),'-Observer',
    '-Executable',('"' + $exePath + '"'),'-Python',('"' + $pythonPath + '"'),
    '-OutputDirectory',('"' + $outputPath + '"'))
$null = Start-Process -FilePath (Get-Process -Id $PID).Path -Verb RunAs -WindowStyle Hidden -ArgumentList $observerArgs -PassThru
& $exePath --disk -d --elevate 1> (Join-Path $outputPath 'parent.stdout.txt') 2> (Join-Path $outputPath 'parent.stderr.txt')
$parentExitCode = $LASTEXITCODE
$deadline = [DateTime]::UtcNow.AddSeconds(140)
$reportPath = Join-Path $outputPath 'console.json'
while (-not (Test-Path -LiteralPath $reportPath) -and [DateTime]::UtcNow -lt $deadline) {
    Start-Sleep -Milliseconds 500
}
if (-not (Test-Path -LiteralPath $reportPath)) { throw "Observer did not produce a report: $outputPath" }
$console = Get-Content -LiteralPath $reportPath -Raw | ConvertFrom-Json
$summary = [ordered]@{
    executable = $exePath
    executable_sha256 = (Get-FileHash -LiteralPath $exePath -Algorithm SHA256).Hash
    parent_elevated = $elevated
    parent_exit_code = $parentExitCode
    parent_stdout_empty = ((Get-Item (Join-Path $outputPath 'parent.stdout.txt')).Length -eq 0)
    parent_stderr_empty = ((Get-Item (Join-Path $outputPath 'parent.stderr.txt')).Length -eq 0)
    console = $console
}
$summary | ConvertTo-Json -Depth 6 | Set-Content -Encoding UTF8 (Join-Path $outputPath 'summary.json')
Write-Output "Results: $outputPath"
$summary | ConvertTo-Json -Depth 6
if (-not $console.passed -or $parentExitCode -ne 0 -or -not $summary.parent_stdout_empty -or -not $summary.parent_stderr_empty) { exit 1 }
