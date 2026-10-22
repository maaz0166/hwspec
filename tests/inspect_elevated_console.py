"""Inspect one test process's console and submit Enter after verifying its pause.

Run only through verify_python_uac.ps1, which validates the process command line.
Uses the Windows console APIs; no desktop-wide key events are generated.
"""
import argparse
import ctypes as ct
from ctypes import wintypes as wt
import json
from pathlib import Path
import time


class Coord(ct.Structure):
    _fields_ = [("x", wt.SHORT), ("y", wt.SHORT)]


class Rect(ct.Structure):
    _fields_ = [("left", wt.SHORT), ("top", wt.SHORT), ("right", wt.SHORT), ("bottom", wt.SHORT)]


class BufferInfo(ct.Structure):
    _fields_ = [("size", Coord), ("cursor", Coord), ("attributes", wt.WORD),
                ("window", Rect), ("maximum", Coord)]


class KeyEvent(ct.Structure):
    _fields_ = [("down", wt.BOOL), ("repeat", wt.WORD), ("key", wt.WORD),
                ("scan", wt.WORD), ("character", wt.WCHAR), ("state", wt.DWORD)]


class Event(ct.Union):
    _fields_ = [("key", KeyEvent), ("padding", ct.c_byte * 16)]


class InputRecord(ct.Structure):
    _fields_ = [("kind", wt.WORD), ("event", Event)]


def inspect(process_id):
    kernel = ct.WinDLL("kernel32", use_last_error=True)
    security = ct.WinDLL("advapi32", use_last_error=True)
    kernel.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
    kernel.OpenProcess.restype = wt.HANDLE
    kernel.CloseHandle.argtypes = [wt.HANDLE]
    kernel.AttachConsole.argtypes = [wt.DWORD]
    kernel.AttachConsole.restype = wt.BOOL
    kernel.CreateFileW.argtypes = [wt.LPCWSTR, wt.DWORD, wt.DWORD, ct.c_void_p, wt.DWORD, wt.DWORD, wt.HANDLE]
    kernel.CreateFileW.restype = wt.HANDLE
    kernel.GetConsoleScreenBufferInfo.argtypes = [wt.HANDLE, ct.POINTER(BufferInfo)]
    kernel.ReadConsoleOutputCharacterW.argtypes = [wt.HANDLE, wt.LPWSTR, wt.DWORD, Coord, ct.POINTER(wt.DWORD)]
    kernel.WriteConsoleInputW.argtypes = [wt.HANDLE, ct.POINTER(InputRecord), wt.DWORD, ct.POINTER(wt.DWORD)]
    kernel.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]
    kernel.WaitForSingleObject.restype = wt.DWORD
    kernel.GetExitCodeProcess.argtypes = [wt.HANDLE, ct.POINTER(wt.DWORD)]
    security.OpenProcessToken.argtypes = [wt.HANDLE, wt.DWORD, ct.POINTER(wt.HANDLE)]
    security.GetTokenInformation.argtypes = [wt.HANDLE, ct.c_int, ct.c_void_p, wt.DWORD, ct.POINTER(wt.DWORD)]

    def require(ok):
        if not ok:
            raise ct.WinError(ct.get_last_error())

    process = kernel.OpenProcess(0x100000 | 0x1000, False, process_id)
    require(process)
    token, output, input_handle = wt.HANDLE(), None, None
    attached = False
    try:
        require(security.OpenProcessToken(process, 0x0008, ct.byref(token)))
        elevation, size = wt.DWORD(), wt.DWORD()
        require(security.GetTokenInformation(token, 20, ct.byref(elevation), ct.sizeof(elevation), ct.byref(size)))
        if not elevation.value:
            raise RuntimeError("Test process is not elevated")
        kernel.FreeConsole()
        require(kernel.AttachConsole(process_id))
        attached = True
        output = kernel.CreateFileW("CONOUT$", 0x80000000, 3, None, 3, 0, None)
        require(output not in (None, ct.c_void_p(-1).value))
        info = BufferInfo()
        text = ""
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            require(kernel.GetConsoleScreenBufferInfo(output, ct.byref(info)))
            count = min(info.size.x * info.size.y, 20000)
            buffer = ct.create_unicode_buffer(count + 1)
            read = wt.DWORD()
            require(kernel.ReadConsoleOutputCharacterW(output, buffer, count, Coord(0, 0), ct.byref(read)))
            text = buffer[:read.value]
            if "Press Enter to close..." in text:
                break
            time.sleep(0.1)
        lines = [text[i:i + info.size.x].rstrip() for i in range(0, len(text), info.size.x)]
        sections = [line for line in lines if line in ("CPU", "Memory", "OS", "GPU", "Disk", "Network", "Battery")]
        if sections != ["CPU"] or "Press Enter to close..." not in text or "Traceback" in text:
            raise RuntimeError("Unexpected elevated console contents: " + repr([line for line in lines if line]))
        input_handle = kernel.CreateFileW("CONIN$", 0x40000000, 3, None, 3, 0, None)
        require(input_handle not in (None, ct.c_void_p(-1).value))
        events = (InputRecord * 2)()
        for i, down in enumerate((True, False)):
            events[i].kind = 1
            events[i].event.key = KeyEvent(down, 1, 0x0D, 0x1C, "\r", 0)
        written = wt.DWORD()
        require(kernel.WriteConsoleInputW(input_handle, events, 2, ct.byref(written)))
        if written.value != 2 or kernel.WaitForSingleObject(process, 10000) != 0:
            raise RuntimeError("Elevated process did not exit after Enter")
        code = wt.DWORD()
        require(kernel.GetExitCodeProcess(process, ct.byref(code)))
        if code.value != 0:
            raise RuntimeError("Elevated process exited with code " + str(code.value))
        return {"passed": True, "pid": process_id, "elevated": True,
                "selected_sections": sections, "pause_observed": True,
                "enter_submitted": True, "exit_code": code.value,
                "console_lines": [line for line in lines if line]}
    finally:
        for handle in (input_handle, output, token, process):
            if handle and handle != ct.c_void_p(-1).value:
                kernel.CloseHandle(handle)
        if attached:
            kernel.FreeConsole()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    try:
        result = inspect(args.pid)
    except Exception as error:
        result = {"passed": False, "error": str(error), "pid": args.pid}
    args.report.write_text(json.dumps(result, indent=2), encoding="utf-8")
    raise SystemExit(0 if result["passed"] else 1)
