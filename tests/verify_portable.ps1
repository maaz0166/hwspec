[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Archive,
    [string]$OutputDirectory = "$PSScriptRoot/../build/portable-validation",
    [string]$Version = '0.5.1'
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$archivePath = (Resolve-Path -LiteralPath $Archive).Path
$zip = [IO.Compression.ZipFile]::OpenRead($archivePath)
try {
    $names = @($zip.Entries | ForEach-Object FullName | Sort-Object)
    if (($names -join ',') -ne 'hwspec.exe,LICENSE,README.txt') { throw 'Unexpected files in portable ZIP.' }
} finally { $zip.Dispose() }
if (Test-Path -LiteralPath $OutputDirectory) { throw 'Portable test directory must be fresh.' }
Expand-Archive -LiteralPath $archivePath -DestinationPath $OutputDirectory
& "$PSScriptRoot/verify_distribution.ps1" -Executable (Join-Path $OutputDirectory 'hwspec.exe') -Version $Version -OutputDirectory (Join-Path $OutputDirectory 'verification')
if (-not $?) { throw 'Extracted portable distribution failed.' }
