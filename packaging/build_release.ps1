[CmdletBinding()]
param(
    [string]$BuildDirectory = "$PSScriptRoot/../build/release",
    [string]$OutputDirectory = "$PSScriptRoot/../dist/native",
    [string]$ISCC
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$output = [IO.Path]::GetFullPath($OutputDirectory)
$cmakeText = [IO.File]::ReadAllText((Join-Path $root 'CMakeLists.txt'))
if ($cmakeText -notmatch 'project\(hwspec VERSION (\d+\.\d+\.\d+)') { throw 'Missing CMake version.' }
$version = $Matches[1]
if (-not $ISCC) {
    foreach ($candidate in @("${env:LOCALAPPDATA}/Programs/Inno Setup 6/ISCC.exe", "${env:ProgramFiles(x86)}/Inno Setup 6/ISCC.exe", "$env:ProgramFiles/Inno Setup 7/ISCC.exe")) {
        if (Test-Path -LiteralPath $candidate) { $ISCC = $candidate; break }
    }
}
if (-not $ISCC) { $ISCC = (Get-Command ISCC.exe -ErrorAction Stop).Source }
& cmake -S $root -B $build -G 'Visual Studio 17 2022' -A x64 -DHWSPEC_STATIC_RUNTIME=ON -DHWSPEC_BUILD_PYTHON=OFF
if ($LASTEXITCODE) { throw 'Release configure failed.' }
& cmake --build $build --config Release --parallel 4
if ($LASTEXITCODE) { throw 'Release build failed.' }
& ctest --test-dir $build -C Release --output-on-failure
if ($LASTEXITCODE) { throw 'Release CTest failed.' }
$exe = Join-Path $build 'Release/hwspec.exe'
& (Join-Path $root 'tests/verify_distribution.ps1') -Executable $exe -Version $version -OutputDirectory (Join-Path $build 'distribution-check')
if (-not $?) { throw 'Distribution smoke check failed.' }
# dumpbin is used only by the builder; distributed binaries require no toolchain.
$vswhere = "${env:ProgramFiles(x86)}/Microsoft Visual Studio/Installer/vswhere.exe"
$visualStudio = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
$dumpbin = Get-ChildItem -Path "$visualStudio/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe" | Sort-Object FullName -Descending | Select-Object -First 1
$dependencies = & $dumpbin.FullName /DEPENDENTS $exe
if ($LASTEXITCODE -or ($dependencies -match '(?i)(MSVCP\d+|VCRUNTIME\d+|UCRTBASE)\.dll')) { throw 'Standalone executable has an external C++ runtime dependency.' }
$null = New-Item -ItemType Directory -Path $output -Force
$stage = Join-Path $build 'portable'
$null = New-Item -ItemType Directory -Path $stage -Force
Copy-Item -LiteralPath $exe -Destination (Join-Path $stage 'hwspec.exe')
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination $stage
@"
hwspec $version - Windows 10/11 x64

No Python or Visual C++ runtime installation is required.
Extract this ZIP and open a terminal in its directory:

  .\hwspec.exe
  .\hwspec.exe --cpu --mem --os
  .\hwspec.exe --all -d --json
  .\hwspec.exe --disk -d --elevate

For installation and PATH setup, use the separate per-user setup executable.
Unknown hardware values are reported honestly. Disk SMART needs administrator
rights and may be unsupported. Text/JSON details can contain network addresses
and the computer name; inspect them before sharing.

https://github.com/maaz0166/hwspec
"@ | Set-Content -Encoding UTF8 (Join-Path $stage 'README.txt')
Copy-Item -LiteralPath $exe -Destination (Join-Path $output "hwspec-$version-windows-x64.exe")
Compress-Archive -LiteralPath @((Join-Path $stage 'hwspec.exe'),(Join-Path $stage 'LICENSE'),(Join-Path $stage 'README.txt')) -DestinationPath (Join-Path $output "hwspec-$version-windows-x64.zip") -Force
& $ISCC "/DAppVersion=$version" "/DStageDir=$stage" "/DReleaseDir=$output" (Join-Path $PSScriptRoot 'windows/hwspec.iss')
if ($LASTEXITCODE) { throw 'Inno Setup compilation failed.' }
& (Join-Path $PSScriptRoot 'write_winget.ps1') -ReleaseDirectory $output -Version $version
@("hwspec-$version-windows-x64.exe","hwspec-$version-windows-x64.zip","hwspec-$version-windows-x64-setup.exe","hwspec-$version-winget.zip") | ForEach-Object {
    '{0}  {1}' -f (Get-FileHash -LiteralPath (Join-Path $output $_) -Algorithm SHA256).Hash.ToLowerInvariant(), $_
} | Set-Content -Encoding ASCII (Join-Path $output 'SHA256SUMS.txt')
$dependencies | Set-Content -Encoding UTF8 (Join-Path $build 'dependencies.txt')
Write-Output "Release $version prepared in $output"
