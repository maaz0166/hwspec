"""Windows x64 hardware specifications. Missing hardware values are None."""
import struct
import sys

if sys.platform != "win32" or struct.calcsize("P") != 8:
    raise ImportError("hwspec supports Windows x64 Python only")

from . import _native

__version__ = _native.__version__
__all__ = ["collect", "__version__"]


def collect(sections=None, *, detail=False, skip_admin=False):
    """Return the selected sections as dictionaries, lists and typed values.

    sections is a name ("cpu"), an iterable of names, or None for all sections.
    Library calls never prompt or request elevation. In a standard-user process,
    detailed disk health has status "requires_admin". Unknown values are None.
    """
    if sections is None:
        names = []
    elif isinstance(sections, str):
        names = [sections]
    else:
        names = list(sections)
    if not all(isinstance(name, str) for name in names):
        raise TypeError("sections must contain section names as strings")
    return _native.collect(names, detail, skip_admin)
