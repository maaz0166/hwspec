hwspec 0.5.1 fixes administrator-menu input in the Windows pip CLI and native CLI.

Python configures Windows C standard input in binary mode. The previous menu
compared the whole input line with "2" or "3", so a retained carriage return
could silently turn either choice into the default skip action. The menu now
accepts surrounding whitespace/CRLF and asks again for invalid input. Enter or
closed input still safely skips privileged collection.

The old PyPI 0.4.1 package reproduced this failure in a real Windows console.
The fixed package passed console checks through both the pip entry point and
`python -m hwspec`; C++ tests exercise option 2 with a fake relauncher so they
can verify preserved arguments without opening a UAC dialog.

Download the setup executable for per-user installation, or extract the portable
ZIP and run `hwspec.exe --help`. The standalone executable and native installer
require no Python or separate Visual C++ runtime installation. Windows 10/11
x64 is required. Binaries remain unsigned.

Python users can upgrade to the matching 0.5.1 package once available on PyPI:

```powershell
python -m pip install --upgrade hwspec==0.5.1
python -m hwspec --disk -d
```

Use `--elevate` to request elevation directly. Disk health can still be Unknown
when the disk/driver does not expose supported SMART data. This input fix does
not promise hardware health availability or establish the remote reporter's
exact environment. Broader hardware and actual user-observed UAC-rejection
acceptance items remain open.

Assets include the native executable, portable ZIP, per-user installer, validated
winget manifest ZIP and SHA-256 checksums. The winget manifest is not yet in
Microsoft's community catalog. Earlier release files remain unchanged.