[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [string]$OutputDirectory = "$PSScriptRoot/../docs/images"
)
$ErrorActionPreference = 'Stop'
$exe = (Resolve-Path -LiteralPath $Executable).Path
$null = New-Item -ItemType Directory -Path $OutputDirectory -Force
Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Diagnostics;
using System.Text;
using System.Runtime.InteropServices;
public static class HwspecCapture {
  [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left, Top, Right, Bottom; }
  [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out Rect r);
  [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
  [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr h, int mode);
  [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint msg, IntPtr w, IntPtr l);
  [DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint flags);
  [DllImport("user32.dll", CharSet=CharSet.Unicode)] public static extern IntPtr FindWindow(string cls, string title);
  [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct Startup {
    public uint cb; public string reserved, desktop, title;
    public uint x,y,width,height,charsX,charsY,fill,flags;
    public ushort show, reservedBytes;
    public IntPtr reservedData,input,output,error;
  }
  [StructLayout(LayoutKind.Sequential)] struct ProcessInfo { public IntPtr process,thread; public uint pid,tid; }
  [DllImport("kernel32.dll", CharSet=CharSet.Unicode, SetLastError=true)] static extern bool CreateProcess(
    string app, StringBuilder command, IntPtr pa, IntPtr ta, bool inherit, uint flags,
    IntPtr environment, string directory, ref Startup startup, out ProcessInfo process);
  [DllImport("kernel32.dll")] static extern bool CloseHandle(IntPtr h);
  public static Process Launch(string exe, string args, string title) {
    var startup = new Startup { cb=(uint)Marshal.SizeOf(typeof(Startup)), title=title, flags=1, show=1 };
    ProcessInfo info;
    if (!CreateProcess(exe,new StringBuilder("\""+exe+"\" "+args),IntPtr.Zero,IntPtr.Zero,false,
      0x10,IntPtr.Zero,System.IO.Path.GetDirectoryName(exe),ref startup,out info))
      throw new System.ComponentModel.Win32Exception(Marshal.GetLastWin32Error());
    try { return Process.GetProcessById((int)info.pid); }
    finally { CloseHandle(info.thread); CloseHandle(info.process); }
  }
}
'@
foreach ($sample in @(@{Name='overview'; Args=@('--cpu','--mem','--os','--pause')}, @{Name='cpu-detail'; Args=@('--cpu','-d','--pause')})) {
    $title = 'hwspec screenshot '+$sample.Name
    $process = [HwspecCapture]::Launch($exe,($sample.Args -join ' '),$title)
    $handle = [IntPtr]::Zero
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(15)
        do {
            Start-Sleep -Milliseconds 200
            $process.Refresh()
            $handle = $process.MainWindowHandle
            if ($handle -eq 0) { $handle = [HwspecCapture]::FindWindow($null,$title) }
        } while ($handle -eq 0 -and [DateTime]::UtcNow -lt $deadline)
        if ($handle -eq 0) { throw 'No console window available for screenshot.' }
        $null = [HwspecCapture]::ShowWindow($handle,9)
        $null = [HwspecCapture]::SetForegroundWindow($handle)
        Start-Sleep -Seconds 1
        $rect = New-Object HwspecCapture+Rect
        if (-not [HwspecCapture]::GetWindowRect($handle,[ref]$rect)) { throw 'Could not read console bounds.' }
        $bitmap = New-Object Drawing.Bitmap (($rect.Right-$rect.Left),($rect.Bottom-$rect.Top))
        $graphics = [Drawing.Graphics]::FromImage($bitmap)
        try {
            $dc = $graphics.GetHdc()
            try { $printed = [HwspecCapture]::PrintWindow($handle,$dc,2) } finally { $graphics.ReleaseHdc($dc) }
            if (-not $printed) { throw 'Could not capture console window.' }
            $bitmap.Save((Join-Path ([IO.Path]::GetFullPath($OutputDirectory)) ("hwspec-"+$sample.Name+'.png')),[Drawing.Imaging.ImageFormat]::Png)
        } finally { $graphics.Dispose(); $bitmap.Dispose() }
    } finally {
        # Send Enter only to this screenshot process's console window.
        if (-not $process.HasExited -and $handle -ne 0) {
            $null = [HwspecCapture]::PostMessage($handle,0x0100,[IntPtr]13,[IntPtr]0)
            $null = [HwspecCapture]::PostMessage($handle,0x0101,[IntPtr]13,[IntPtr]0)
            if (-not $process.WaitForExit(5000)) { $null = [HwspecCapture]::PostMessage($handle,0x0010,[IntPtr]0,[IntPtr]0) }
        }
        if (-not $process.WaitForExit(5000)) { $process.Kill() }
        $process.Dispose()
    }
}
