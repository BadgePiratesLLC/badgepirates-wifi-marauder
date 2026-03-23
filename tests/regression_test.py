#!/usr/bin/env python3
"""BSidesKC Badge - Phase 7 Regression Test Suite
Automated build verification and serial-based runtime tests.
Run: python3 tests/regression_test.py [--port /dev/ttyACM0]
"""

import subprocess, sys, os, re, json, time, argparse
from datetime import datetime

PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RESULTS = {"timestamp": "", "build": {}, "memory": {}, "serial_tests": [], "summary": {}}

def run_cmd(cmd, cwd=PROJECT_DIR):
    r = subprocess.run(cmd, shell=True, capture_output=True, text=True, cwd=cwd)
    return r.returncode, r.stdout, r.stderr

def test_build():
    """Verify clean build succeeds and capture memory stats."""
    print("[BUILD] Compiling...")
    rc, out, err = run_cmd("pio run -e bsideskc-badge 2>&1")
    full = out + err
    build_ok = rc == 0 and "SUCCESS" in full

    ram_match = re.search(r'RAM:\s+\[.*?\]\s+([\d.]+)%\s+\(used (\d+) bytes from (\d+) bytes\)', full)
    flash_match = re.search(r'Flash:\s+\[.*?\]\s+([\d.]+)%\s+\(used (\d+) bytes from (\d+) bytes\)', full)

    RESULTS["build"] = {
        "status": "PASS" if build_ok else "FAIL",
        "ram_pct": float(ram_match.group(1)) if ram_match else -1,
        "ram_used": int(ram_match.group(2)) if ram_match else -1,
        "ram_total": int(ram_match.group(3)) if ram_match else -1,
        "flash_pct": float(flash_match.group(1)) if flash_match else -1,
        "flash_used": int(flash_match.group(2)) if flash_match else -1,
        "flash_total": int(flash_match.group(3)) if flash_match else -1,
    }
    print(f"  Build: {'PASS' if build_ok else 'FAIL'}")
    if ram_match:
        print(f"  RAM:   {ram_match.group(1)}% ({ram_match.group(2)}/{ram_match.group(3)})")
    if flash_match:
        print(f"  Flash: {flash_match.group(1)}% ({flash_match.group(2)}/{flash_match.group(3)})")
    return build_ok

def test_size_analysis():
    """Detailed per-object size analysis."""
    print("[SIZE] Analyzing object sizes...")
    rc, out, _ = run_cmd("pio run -e bsideskc-badge -t size 2>&1")
    size_match = re.search(r'(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+\w+\s+.*firmware\.elf', out)
    if size_match:
        RESULTS["memory"] = {
            "text": int(size_match.group(1)),
            "data": int(size_match.group(2)),
            "bss": int(size_match.group(3)),
            "total": int(size_match.group(4)),
        }
        print(f"  .text={size_match.group(1)}  .data={size_match.group(2)}  .bss={size_match.group(3)}")

def test_serial_boot(port):
    """Monitor serial output during boot for expected init messages."""
    if not port:
        print("[SERIAL] Skipped (no port specified)")
        return
    try:
        import serial
        ser = serial.Serial(port, 115200, timeout=10)
        print(f"[SERIAL] Monitoring {port} for boot messages (10s)...")
        boot_log = ""
        start = time.time()
        while time.time() - start < 10:
            if ser.in_waiting:
                boot_log += ser.read(ser.in_waiting).decode("utf-8", errors="replace")
        ser.close()

        expected = [
            ("[BSidesKC] Booting", "Boot message"),
            ("[BSidesKC] Rotary encoder", "Encoder init"),
            ("[Buzzer] Initialized", "Buzzer init"),
            ("[Battery] Monitor", "Battery init"),
            ("[Power] Manager", "Power manager init"),
            ("[Badge] Menu items", "Badge menu init"),
            ("[BSidesKC] Marauder ready", "Boot complete"),
        ]
        for pattern, name in expected:
            found = pattern in boot_log
            RESULTS["serial_tests"].append({"test": name, "status": "PASS" if found else "FAIL"})
            print(f"  {name}: {'PASS' if found else 'FAIL'}")
    except ImportError:
        print("[SERIAL] pyserial not installed, skipping")
    except Exception as e:
        print(f"[SERIAL] Error: {e}")

def generate_report():
    RESULTS["timestamp"] = datetime.now().isoformat()
    passed = sum(1 for t in RESULTS.get("serial_tests", []) if t["status"] == "PASS")
    total = len(RESULTS.get("serial_tests", []))
    RESULTS["summary"] = {
        "build": RESULTS["build"].get("status", "UNKNOWN"),
        "serial_tests_passed": passed,
        "serial_tests_total": total,
        "ram_usage_pct": RESULTS["build"].get("ram_pct", -1),
        "flash_usage_pct": RESULTS["build"].get("flash_pct", -1),
    }
    report_path = os.path.join(PROJECT_DIR, "tests", "regression_report.json")
    with open(report_path, "w") as f:
        json.dump(RESULTS, f, indent=2)
    print(f"\n[REPORT] Saved to {report_path}")

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", help="Serial port for runtime tests")
    args = parser.parse_args()

    print("=" * 60)
    print("BSidesKC Badge - Phase 7 Regression Test Suite")
    print("=" * 60)

    test_build()
    test_size_analysis()
    test_serial_boot(args.port)
    generate_report()
