#!/usr/bin/env python3
"""Run the Android CPU Poisson benchmark repeatedly through adb (stdlib only).

The reported solver time is measured inside the Android process using a
steady clock. Host round-trip timings are intentionally NOT used for speedups.
"""

import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import statistics
import subprocess
import sys


REQUIRED = (
    "backend",
    "grid",
    "iterations",
    "converged",
    "relative_residual",
    "max_abs_pressure_error",
    "solver_wall_time_ms",
)


def adb_cmd(adb, serial, *args):
    cmd = [adb]
    if serial:
        cmd.extend(["-s", serial])
    cmd.extend(args)
    completed = subprocess.run(cmd, capture_output=True, text=True, check=False)
    if completed.returncode != 0:
        raise RuntimeError(
            f"adb command failed ({completed.returncode}): {' '.join(cmd)}\n"
            f"{completed.stderr.strip()}\n{completed.stdout.strip()}"
        )
    return completed.stdout.strip()


def getprop(adb, serial, key):
    return adb_cmd(adb, serial, "shell", "getprop", key).strip()


def parse_output(stdout):
    fields = {}
    for line in stdout.splitlines():
        key, sep, value = line.partition("=")
        if sep:
            fields[key.strip()] = value.strip()
    missing = [key for key in REQUIRED if key not in fields]
    if missing:
        raise ValueError(f"Missing benchmark fields: {missing}; output={stdout!r}")
    if fields["backend"] != "cpu_jacobi":
        raise ValueError(f"Expected CPU reference, got {fields['backend']}")
    result = {
        "backend": fields["backend"],
        "grid": fields["grid"],
        "iterations": int(fields["iterations"]),
        "converged": fields["converged"] == "true",
        "relative_residual": float(fields["relative_residual"]),
        "max_abs_pressure_error": float(fields["max_abs_pressure_error"]),
        "solver_wall_time_ms": float(fields["solver_wall_time_ms"]),
    }
    if fields["converged"] not in ("true", "false"):
        raise ValueError("Invalid converged flag")
    if not all(math.isfinite(result[k]) and result[k] >= 0 for k in
               ("relative_residual", "max_abs_pressure_error", "solver_wall_time_ms")):
        raise ValueError("Benchmark contains invalid numeric measurements")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary", type=Path, default=Path("build/android-arm64/hfs_bench"),
                        help="Android arm64 hfs_bench executable (not a desktop binary)")
    parser.add_argument("--adb", default="adb", help="adb executable, e.g. adb.exe")
    parser.add_argument("--serial", default=None, help="adb device serial if more than one")
    parser.add_argument("--sizes", default="16,32,64", help="comma-separated square grid widths")
    parser.add_argument("--runs", type=int, default=5, help="recorded runs per grid")
    parser.add_argument("--warmups", type=int, default=1, help="unrecorded runs per grid")
    parser.add_argument("--max-iterations", type=int, default=10000)
    parser.add_argument("--tolerance", type=float, default=1e-4)
    parser.add_argument("--output-dir", type=Path, default=Path("benchmark-results"))
    args = parser.parse_args()

    try:
        sizes = [int(s.strip()) for s in args.sizes.split(",")]
        if not sizes or any(s < 1 or s > 4096 for s in sizes):
            raise ValueError("sizes must be integers between 1 and 4096")
        if args.runs < 1 or args.warmups < 0 or args.max_iterations < 0:
            raise ValueError("runs >= 1, warmups >= 0 and max-iterations >= 0 required")
        if not math.isfinite(args.tolerance) or args.tolerance <= 0:
            raise ValueError("tolerance must be a positive finite number")
        if not args.binary.is_file():
            raise FileNotFoundError(
                f"Android executable not found: {args.binary}. "
                "Download the android-arm64 artifact or build with the NDK."
            )
        state = adb_cmd(args.adb, args.serial, "get-state")
        if state != "device":
            raise RuntimeError(f"Device not ready: {state}")

        device = {
            "manufacturer": getprop(args.adb, args.serial, "ro.product.manufacturer"),
            "model": getprop(args.adb, args.serial, "ro.product.model"),
            "device": getprop(args.adb, args.serial, "ro.product.device"),
            "soc": getprop(args.adb, args.serial, "ro.soc.model"),
            "sdk": getprop(args.adb, args.serial, "ro.build.version.sdk"),
            "android": getprop(args.adb, args.serial, "ro.build.version.release"),
            "abi": getprop(args.adb, args.serial, "ro.product.cpu.abi"),
            "build_fingerprint": getprop(args.adb, args.serial, "ro.build.fingerprint"),
        }
        if device["abi"] != "arm64-v8a":
            raise RuntimeError(f"Expected arm64-v8a Android target, got {device['abi']}")

        remote = "/data/local/tmp/hfs_bench"
        adb_cmd(args.adb, args.serial, "push", str(args.binary), remote)
        adb_cmd(args.adb, args.serial, "shell", "chmod", "700", remote)

        timestamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%SZ")
        report = {
            "timestamp_utc": timestamp,
            "device": device,
            "parameters": {
                "sizes": sizes,
                "runs_per_grid": args.runs,
                "warmups_per_grid": args.warmups,
                "max_iterations": args.max_iterations,
                "relative_tolerance": args.tolerance,
                "timing_definition": "solver_wall_time_ms: Android steady_clock time",
                "method": "cold-started process per sample; synthetic RHS; no GPU/NPU",
            },
            "samples": [],
        }
        print(f"Device: {device['manufacturer']} {device['model']} "
              f"(Android {device['android']}, SoC: {device['soc'] or 'unknown'})")
        for size in sizes:
            current = []
            for repeat in range(-args.warmups, args.runs):
                raw = adb_cmd(
                    args.adb, args.serial, "shell", remote,
                    str(size), str(size), str(args.max_iterations),
                    str(args.tolerance),
                )
                sample = parse_output(raw)
                if sample["grid"] != f"{size}x{size}":
                    raise ValueError("Benchmark returned an unexpected grid size")
                if repeat >= 0:
                    sample["repeat"] = repeat + 1
                    sample["size"] = size
                    report["samples"].append(sample)
                    current.append(sample)
            times = [s["solver_wall_time_ms"] for s in current]
            ok_count = sum(s["converged"] for s in current)
            print(f"{size}x{size}: median={statistics.median(times):.4f} ms, "
                  f"min={min(times):.4f} ms, "
                  f"converged={ok_count}/{len(current)}, "
                  f"median_residual={statistics.median(s['relative_residual'] for s in current):.3g}")
        args.output_dir.mkdir(parents=True, exist_ok=True)
        destination = args.output_dir / f"cpu-jacobi-{timestamp}.json"
        destination.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n",
                               encoding="utf-8")
        print(f"Saved: {destination}")
        if not all(s["converged"] for s in report["samples"]):
            print("WARNING: Some runs did not reach the requested residual. "
                  "Do not compare their timing as time-to-accuracy.", file=sys.stderr)
        return 0
    except (RuntimeError, ValueError, OSError, subprocess.SubprocessError) as error:
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
