hwspec provides CPU, memory, Windows, GPU, disk, network and battery specifications
through a native Windows x64 command-line application.

This preview adds distribution without Python:

- A standalone executable with the C++ runtime bundled.
- A portable ZIP containing the executable, MIT license and usage instructions.
- A per-user Inno Setup installer with optional PATH setup and clean uninstall.
- A validated winget manifest ZIP and SHA-256 checksums for the release assets.

Use the setup executable for installation, or extract the portable ZIP and run
`hwspec.exe --help`. After installer PATH setup, open a new terminal and run
`hwspec --cpu --mem --os` or `hwspec --all -d --json`.

Windows 10/11 x64 is required. The binaries are unsigned, so Windows may ask
whether you trust the download. Python and a Visual C++ runtime installation
are not required. The separately published Python package remains available on
PyPI as version 0.4.1.

Known acceptance limits: the second physical PC, broader AMD/Windows 10/VM
coverage and actual user-observed UAC rejection remain unverified. Native
elevation, pause and Enter-to-close passed; this laptop's NVMe SMART provider
is unsupported and correctly reports Unknown. This preview does not claim those
remaining checks are complete. Hardware-dependent fields may be Unknown/null.

The winget manifest is supplied for review and local validation. Its inclusion
does not mean the package has been accepted into Microsoft's community catalog.
Public winget installation requires publicly accessible release asset URLs.
