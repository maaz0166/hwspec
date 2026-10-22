"""Windows CI / local smoke check. Checks JSON shape, not hardware-dependent numbers."""
import json
import subprocess
import sys

exe = sys.argv[1]
sections = ("cpu", "mem", "os", "gpu", "disk", "net", "battery")

def read(*args):
    result = subprocess.run([exe, *args], check=True, text=True, capture_output=True, timeout=35)
    assert not result.stderr, result.stderr
    return json.loads(result.stdout)

normal = read("--all", "--json")
assert tuple(normal) == sections, normal.keys()
assert isinstance(normal["cpu"], dict)
assert isinstance(normal["gpu"]["adapters"], list)
assert isinstance(normal["disk"]["drives"], list)
assert isinstance(normal["net"]["adapters"], list)
assert normal["battery"]["present"] is None or isinstance(normal["battery"]["present"], bool)
assert "max_boost_clock_ghz" not in normal["cpu"]

detailed = read("--json", "-d", "--all")
assert tuple(detailed) == sections
assert "max_boost_clock_ghz" in detailed["cpu"]
assert "modules" in detailed["mem"]
assert "uptime_seconds" in detailed["os"]
assert detailed["disk"]["disk_health"]["status"] in ("requires_admin", "unknown", "available")
assert isinstance(detailed["disk"]["disk_health"]["devices"], list)
skipped = read("--disk", "-d", "--json", "--skip-admin", "--pause")
assert skipped["disk"]["disk_health"] == {"status": "requires_admin", "devices": []}
assert "disk_health" not in read("--disk", "--json")["disk"]
assert "health_percent" in detailed["battery"]
for adapter in detailed["net"]["adapters"]:
    assert adapter["link_speed_bps"] != 2**64 - 1, adapter

battery = read("--battery", "-d", "--json")
assert list(battery) == ["battery"]

print("hwspec JSON smoke test passed")
