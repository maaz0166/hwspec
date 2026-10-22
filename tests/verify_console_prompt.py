"""Exercise menu input in a hidden console owned by this test; never send global keys."""
import argparse
import ctypes as ct
from ctypes import wintypes as wt
import json
from pathlib import Path
import subprocess
import time
from inspect_elevated_console import Coord, BufferInfo, KeyEvent, InputRecord


def probe(command, answer, expected_code, retry=False):
    kernel = ct.WinDLL("kernel32", use_last_error=True)
    kernel.AttachConsole.argtypes = [wt.DWORD]
    kernel.AttachConsole.restype = wt.BOOL
    kernel.CreateFileW.argtypes = [wt.LPCWSTR, wt.DWORD, wt.DWORD, ct.c_void_p, wt.DWORD, wt.DWORD, wt.HANDLE]
    kernel.CreateFileW.restype = wt.HANDLE
    kernel.CloseHandle.argtypes = [wt.HANDLE]
    kernel.GetConsoleScreenBufferInfo.argtypes = [wt.HANDLE, ct.POINTER(BufferInfo)]
    kernel.ReadConsoleOutputCharacterW.argtypes = [wt.HANDLE, wt.LPWSTR, wt.DWORD, Coord, ct.POINTER(wt.DWORD)]
    kernel.WriteConsoleInputW.argtypes = [wt.HANDLE, ct.POINTER(InputRecord), wt.DWORD, ct.POINTER(wt.DWORD)]
    startup = subprocess.STARTUPINFO()
    startup.dwFlags |= subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0
    child = subprocess.Popen(command + ["--disk", "-d"], creationflags=subprocess.CREATE_NEW_CONSOLE, startupinfo=startup)
    output = input_handle = None
    attached = False

    def require(ok):
        if not ok:
            raise ct.WinError(ct.get_last_error())

    def screen():
        info = BufferInfo()
        require(kernel.GetConsoleScreenBufferInfo(output, ct.byref(info)))
        count = min(info.size.x * info.size.y, 30000)
        buffer = ct.create_unicode_buffer(count + 1)
        read = wt.DWORD()
        require(kernel.ReadConsoleOutputCharacterW(output, buffer, count, Coord(0, 0), ct.byref(read)))
        return buffer[:read.value]

    def wait_for(marker):
        deadline = time.monotonic() + 15
        while time.monotonic() < deadline:
            text = screen()
            if marker in text:
                return text
            if child.poll() is not None:
                raise RuntimeError("Child exited before " + marker + ": " + text.rstrip())
            time.sleep(0.1)
        raise RuntimeError("Timed out waiting for " + marker)

    def type_text(text):
        events = (InputRecord * (2 * len(text)))()
        for index, char in enumerate(text):
            for offset, down in enumerate((True, False)):
                event = events[2 * index + offset]
                event.kind = 1
                event.event.key = KeyEvent(down, 1, 13 if char == "\r" else 0, 0, char, 0)
        written = wt.DWORD()
        require(kernel.WriteConsoleInputW(input_handle, events, len(events), ct.byref(written)))
        if written.value != len(events):
            raise RuntimeError("Partial console input")

    try:
        kernel.FreeConsole()
        deadline = time.monotonic() + 8
        while not kernel.AttachConsole(child.pid):
            if child.poll() is not None or time.monotonic() >= deadline:
                raise RuntimeError("Cannot attach to test console")
            time.sleep(0.1)
        attached = True
        output = kernel.CreateFileW("CONOUT$", 0x80000000, 3, None, 3, 0, None)
        input_handle = kernel.CreateFileW("CONIN$", 0x40000000, 3, None, 3, 0, None)
        for handle in (output, input_handle):
            require(handle not in (None, ct.c_void_p(-1).value))
        wait_for("Choice [1]:")
        if retry:
            type_text("invalid\r")
            wait_for("Please enter 1, 2, or 3")
        type_text(answer)
        code = child.wait(timeout=20)
        text = screen()
        if code != expected_code:
            raise RuntimeError("Expected exit %d, got %d; console: %s" % (expected_code, code, text.rstrip()))
        if expected_code == 0 and "requires admin" not in text:
            raise RuntimeError("Skipped health status missing")
        return {"passed": True, "answer": repr(answer), "retry": retry, "exit_code": code}
    finally:
        if child.poll() is None:
            child.kill()
            child.wait(timeout=10)
        for handle in (input_handle, output):
            if handle and handle != ct.c_void_p(-1).value:
                kernel.CloseHandle(handle)
        if attached:
            kernel.FreeConsole()


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    target = parser.add_mutually_exclusive_group(required=True)
    target.add_argument("--python", type=Path)
    target.add_argument("--executable", type=Path)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    command = [str(args.python.resolve()), "-m", "hwspec"] if args.python else [str(args.executable.resolve())]
    checks = []
    try:
        for answer, code, retry in [("1\r", 0, False), ("3\r", 1, False), (" 3 \r", 1, False), ("3\r", 1, True)]:
            checks.append(probe(command, answer, code, retry))
        result = {"passed": True, "checks": checks}
    except Exception as error:
        result = {"passed": False, "checks": checks, "error": str(error)}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(result, indent=2), encoding="utf-8")
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if result["passed"] else 1)