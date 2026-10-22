"""The pip command and python -m hwspec share the native CLI entry point."""
import sys
from . import _native


def main(argv=None):
    # JSON/text uses the same UTF-8 representation as the standalone executable.
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
    return _native.run_cli(list(sys.argv[1:] if argv is None else argv), sys.executable)


if __name__ == "__main__":
    raise SystemExit(main())
