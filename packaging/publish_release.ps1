[CmdletBinding()]
param(
    [string]$ReleaseDirectory = "$PSScriptRoot/../dist/native",
    [string]$Repository = 'maaz0166/hwspec',
    [string]$TargetCommit
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = [IO.Path]::GetFullPath($ReleaseDirectory)
$text = Get-Content (Join-Path $root 'CMakeLists.txt') -Raw
if ($text -notmatch 'project\(hwspec VERSION (\d+\.\d+\.\d+)') { throw 'Missing version.' }
$version = $Matches[1]
if (-not $TargetCommit) { $TargetCommit = git -C $root rev-parse HEAD }
if (git -C $root status --porcelain) { throw 'Commit the reviewed source before publishing.' }
$names = @("hwspec-$version-windows-x64.exe","hwspec-$version-windows-x64.zip","hwspec-$version-windows-x64-setup.exe","hwspec-$version-winget.zip")
$seen = @()
foreach ($line in Get-Content (Join-Path $output 'SHA256SUMS.txt')) {
    if ($line -notmatch '^([0-9a-fA-F]{64})  (.+)$') { throw 'Invalid checksum manifest.' }
    $expected = $Matches[1]
    $name = $Matches[2]
    if ($name -notin $names -or $name -in $seen) { throw 'Unexpected or duplicate release asset.' }
    if ((Get-FileHash -LiteralPath (Join-Path $output $name)).Hash -ne $expected) { throw "Release hash mismatch: $name" }
    $seen += $name
}
if ($seen.Count -ne $names.Count) { throw 'Incomplete release manifest.' }
$permission = gh api "repos/$Repository" --jq '.permissions.push'
if ($LASTEXITCODE -or $permission -ne 'true') { throw 'GitHub authentication/write access is required. Run gh auth login locally.' }
$assets = @($names | ForEach-Object { Join-Path $output $_ }) + (Join-Path $output 'SHA256SUMS.txt')
# A draft prevents a partially uploaded release from appearing as published.
# Existing releases are never overwritten or silently reused.
gh release create "v$version" @assets --repo $Repository --target $TargetCommit --title "hwspec $version preview" --prerelease --draft --notes-file (Join-Path $PSScriptRoot 'RELEASE_NOTES.md')
if ($LASTEXITCODE) { throw 'Release creation failed; no retry or overwrite attempted.' }
gh release edit "v$version" --repo $Repository --draft=false
if ($LASTEXITCODE) { throw 'Assets are in a draft; publication failed. Inspect the draft before retrying.' }
gh release view "v$version" --repo $Repository --json url,isDraft,isPrerelease,assets
if ($LASTEXITCODE) { throw 'Publication completed but remote verification failed; inspect the existing release.' }
