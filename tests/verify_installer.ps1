[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Installer,
    [string]$WorkDirectory = "$PSScriptRoot/../build/installer-validation",
    [string]$Version = '0.5.0'
)
$ErrorActionPreference = 'Stop'
$installerPath = (Resolve-Path -LiteralPath $Installer).Path
$work = [IO.Path]::GetFullPath($WorkDirectory)
$app = Join-Path $work ('installed hwspec ' + [char]0x03A9)
$stateKey = 'Software\maaz0166\hwspec'
$uninstallKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{99AE202A-6C6B-413A-8320-D3C32F5A6A1A}_is1'
if ([Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($stateKey) -or [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($uninstallKey)) {
    throw 'An existing hwspec installation is registered; refusing to replace it for a test.'
}
if (Test-Path -LiteralPath $app) { throw 'Test install directory already exists.' }
$null = New-Item -ItemType Directory -Path $work -Force
$environmentKey = [Microsoft.Win32.Registry]::CurrentUser.CreateSubKey('Environment')
$original = $environmentKey.GetValue('Path', $null, [Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
$originalKind = if ($null -eq $original) { $null } else { $environmentKey.GetValueKind('Path') }
$checks = New-Object 'System.Collections.Generic.List[string]'
function Read-UserPath {
    return $environmentKey.GetValue('Path',$null,[Microsoft.Win32.RegistryValueOptions]::DoNotExpandEnvironmentNames)
}
function Assert([bool]$Passed, [string]$Name) {
    if (-not $Passed) { throw "Installer check failed: $Name" }
    $checks.Add($Name)
}
function Run-Setup([string]$Task = 'addtopath', [string]$LogName = 'install') {
    $arguments = @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART','/SP-',('/DIR="'+$app+'"'),('/TASKS="'+$Task+'"'),('/LOG="'+(Join-Path $work ($LogName+'.log'))+'"'))
    $process = Start-Process -FilePath $installerPath -ArgumentList $arguments -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(90000)) { throw 'Installer timed out.' }
    Assert ($process.ExitCode -eq 0) "$LogName exit code"
}
function Uninstall([string]$Name) {
    $uninstaller = Join-Path $app 'unins000.exe'
    if (-not (Test-Path -LiteralPath $uninstaller)) { throw 'Missing test uninstaller.' }
    $process = Start-Process -FilePath $uninstaller -WindowStyle Hidden -ArgumentList @('/VERYSILENT','/SUPPRESSMSGBOXES','/NORESTART',('/LOG="'+(Join-Path $work ($Name+'.log'))+'"')) -PassThru
    if (-not $process.WaitForExit(90000)) { throw 'Uninstaller timed out.' }
    Assert ($process.ExitCode -eq 0) "$Name exit code"
    $deadline = [DateTime]::UtcNow.AddSeconds(15)
    while ((Test-Path -LiteralPath (Join-Path $app 'hwspec.exe')) -and [DateTime]::UtcNow -lt $deadline) { Start-Sleep -Milliseconds 100 }
    Assert (-not (Test-Path -LiteralPath (Join-Path $app 'hwspec.exe'))) "$Name removes executable"
    Assert ($null -eq [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($uninstallKey)) "$Name removes uninstall registration"
}
function Set-Path([string]$Value) { $environmentKey.SetValue('Path',$Value,[Microsoft.Win32.RegistryValueKind]::ExpandString) }
function Append-Path([string]$Before, [string]$Segment) {
    if (-not $Before) { return $Segment }
    if ($Before.EndsWith(';')) { return $Before+$Segment }
    return $Before+';'+$Segment
}
try {
    Run-Setup
    Assert (Test-Path -LiteralPath (Join-Path $app 'hwspec.exe')) 'installs to a path with spaces and Unicode'
    $after = Read-UserPath
    Assert (@($after -split ';' | Where-Object { $_ -eq $app }).Count -eq 1) 'adds PATH once'
    & "$PSScriptRoot/verify_distribution.ps1" -Executable (Join-Path $app 'hwspec.exe') -Version $Version -OutputDirectory (Join-Path $work 'installed-smoke')
    if (-not $?) { throw 'Installed executable failed distribution checks.' }
    $checks.Add('installed command runs without Python')
    Run-Setup 'addtopath' 'reinstall'
    Assert ((Read-UserPath) -ceq $after) 'reinstall does not duplicate PATH'
    $sentinel = Join-Path $work 'user-added-path'
    Set-Path (Append-Path $after $sentinel)
    Uninstall 'uninstall-owned'
    Assert ((Read-UserPath) -ceq (Append-Path $original $sentinel)) 'uninstall preserves later user PATH changes'
    Set-Path (Append-Path $original ('"'+$app+'\"'))
    $preexisting = Read-UserPath
    Run-Setup 'addtopath' 'preexisting'
    Assert ((Read-UserPath) -ceq $preexisting) 'recognizes quoted preexisting PATH entry'
    Uninstall 'uninstall-preexisting'
    Assert ((Read-UserPath) -ceq $preexisting) 'uninstall preserves an unowned PATH entry'
    Set-Path ([string]$original)
    Run-Setup '' 'without-path'
    Assert ((Read-UserPath) -ceq ([string]$original)) 'optional PATH task can be disabled'
    Uninstall 'uninstall-without-path'
    $environmentKey.DeleteValue('Path',$false)
    Run-Setup 'addtopath' 'missing-path'
    Assert ((Read-UserPath) -ceq $app) 'installation handles an absent user PATH'
    Uninstall 'uninstall-missing-path'
    Assert ($null -eq (Read-UserPath)) 'uninstall restores absent PATH value'
} finally {
    # This directory and registry identity were absent before this test.
    try {
        # Inno's uninstaller stub can remain briefly after successful removal.
        # Do not run it again once the product and its registration are gone.
        if ((Test-Path -LiteralPath (Join-Path $app 'hwspec.exe')) -or
            ($null -ne [Microsoft.Win32.Registry]::CurrentUser.OpenSubKey($uninstallKey))) { Uninstall 'cleanup' }
    } finally {
        if ($null -eq $original) { $environmentKey.DeleteValue('Path',$false) }
        else { $environmentKey.SetValue('Path',$original,$originalKind) }
        Assert ((Read-UserPath) -ceq $original) 'test restores original user PATH'
        $environmentKey.Dispose()
    }
}
@{passed=$true; version=$Version; checks=@($checks.ToArray()); installer_sha256=(Get-FileHash -LiteralPath $installerPath).Hash} |
    ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 (Join-Path $work 'summary.json')
Write-Output "Installer lifecycle checks passed: $($checks.Count)"
