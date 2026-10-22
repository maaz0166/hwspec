[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$Version,
    [string]$OutputDirectory = "$PSScriptRoot/../build/distribution-check"
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $Executable).Path
$null = New-Item -ItemType Directory -Path $OutputDirectory -Force
$checks = New-Object 'System.Collections.Generic.List[string]'
function Capture([string[]]$Flags, [int]$Expected = 0) {
    $info = New-Object Diagnostics.ProcessStartInfo
    $info.FileName = $exe
    $info.Arguments = $Flags -join ' '
    $info.UseShellExecute = $false
    $info.CreateNoWindow = $true
    $info.RedirectStandardInput = $true
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    # No Python, compiler, venv or developer DLL directory can be resolved here.
    $info.EnvironmentVariables['PATH'] = "$env:SystemRoot/System32;$env:SystemRoot"
    $info.EnvironmentVariables.Remove('PYTHONPATH')
    $info.EnvironmentVariables.Remove('VIRTUAL_ENV')
    $info.StandardOutputEncoding = [Text.Encoding]::UTF8
    $info.StandardErrorEncoding = [Text.Encoding]::UTF8
    $process = New-Object Diagnostics.Process
    $process.StartInfo = $info
    try {
        $null = $process.Start()
        $process.StandardInput.Close()
        $stdout = $process.StandardOutput.ReadToEndAsync()
        $stderr = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit(35000)) { $process.Kill(); throw 'Distribution command timed out.' }
        $text = $stdout.GetAwaiter().GetResult()
        $errors = $stderr.GetAwaiter().GetResult()
        if ($process.ExitCode -ne $Expected -or ($Expected -eq 0 -and $errors)) { throw "Distribution command failed: $Flags; $errors" }
        return $text
    } finally { $process.Dispose() }
}
if ((Capture @('--version')).Trim() -ne "hwspec $Version") { throw 'Version mismatch.' }
$checks.Add('version')
if ((Get-Item -LiteralPath $exe).VersionInfo.ProductVersion -ne $Version) { throw 'Windows version resource mismatch.' }
$checks.Add('Windows version resource')
$normal = Capture @('--all','--json') | ConvertFrom-Json
if (($normal.PSObject.Properties.Name -join ',') -ne 'cpu,mem,os,gpu,disk,net,battery') { throw 'Missing sections.' }
$checks.Add('all-section JSON without Python')
$detail = Capture @('--all','-d','--json','--pause','--skip-admin') | ConvertFrom-Json
if ($detail.disk.disk_health.status -ne 'requires_admin' -or -not ($detail.cpu.PSObject.Properties.Name -contains 'l1_cache_bytes')) { throw 'Detailed/skip/pause behavior failed.' }
$checks.Add('detailed JSON, missing data and noninteractive admin/pause')
$text = Capture @('--cpu','--mem','--os')
if ($text -notmatch '(?m)^CPU' -or $text -notmatch '(?m)^Memory' -or $text -notmatch '(?m)^OS') { throw 'Text selection failed.' }
$checks.Add('combined text selection')
$null = Capture @('--invalid-distribution-test') 2
$checks.Add('invalid option exit code')
@{ passed=$true; version=$Version; executable_sha256=(Get-FileHash -LiteralPath $exe).Hash; checks=@($checks.ToArray()) } |
    ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 (Join-Path $OutputDirectory 'summary.json')
Write-Output "Distribution checks passed: $($checks.Count); PATH excludes Python/toolchain directories."
