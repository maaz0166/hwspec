[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ReleaseDirectory,
    [Parameter(Mandatory = $true)][string]$Version
)
$ErrorActionPreference = 'Stop'
$installer = Join-Path $ReleaseDirectory "hwspec-$Version-windows-x64-setup.exe"
$sha = (Get-FileHash -LiteralPath $installer -Algorithm SHA256).Hash
$manifest = Join-Path $ReleaseDirectory 'winget'
$null = New-Item -ItemType Directory -Path $manifest -Force
@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.version.1.10.0.schema.json
PackageIdentifier: Maaz0166.hwspec
PackageVersion: $Version
DefaultLocale: en-US
ManifestType: version
ManifestVersion: 1.10.0
"@ | Set-Content -Encoding UTF8 (Join-Path $manifest 'Maaz0166.hwspec.yaml')
@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.defaultLocale.1.10.0.schema.json
PackageIdentifier: Maaz0166.hwspec
PackageVersion: $Version
PackageLocale: en-US
Publisher: MuhammadMaaz
PublisherUrl: https://github.com/maaz0166
PackageName: hwspec
PackageUrl: https://github.com/maaz0166/hwspec
License: MIT
LicenseUrl: https://github.com/maaz0166/hwspec/blob/v$Version/LICENSE
ShortDescription: Windows x64 hardware and OS specifications from a native CLI.
Moniker: hwspec
Tags:
- hardware
- system-information
- cli
ManifestType: defaultLocale
ManifestVersion: 1.10.0
"@ | Set-Content -Encoding UTF8 (Join-Path $manifest 'Maaz0166.hwspec.locale.en-US.yaml')
@"
# yaml-language-server: `$schema=https://aka.ms/winget-manifest.installer.1.10.0.schema.json
PackageIdentifier: Maaz0166.hwspec
PackageVersion: $Version
InstallerType: inno
Scope: user
MinimumOSVersion: 10.0.10240.0
UpgradeBehavior: install
Commands:
- hwspec
ProductCode: '{99AE202A-6C6B-413A-8320-D3C32F5A6A1A}_is1'
Installers:
- Architecture: x64
  InstallerUrl: https://github.com/maaz0166/hwspec/releases/download/v$Version/hwspec-$Version-windows-x64-setup.exe
  InstallerSha256: $sha
ManifestType: installer
ManifestVersion: 1.10.0
"@ | Set-Content -Encoding UTF8 (Join-Path $manifest 'Maaz0166.hwspec.installer.yaml')
& winget validate --manifest $manifest
if ($LASTEXITCODE) { throw 'winget manifest validation failed.' }
Compress-Archive -Path (Join-Path $manifest '*.yaml') -DestinationPath (Join-Path $ReleaseDirectory "hwspec-$Version-winget.zip") -Force
