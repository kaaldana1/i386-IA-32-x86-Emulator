#!/usr/bin/env python3
"""Time every tests/cases/*.bin on four routes and write one CSV row per bin.

Routes:
  interp  - the C interpreter at the repo root (./c --program <txt> --interp-only)
  hybrid  - hybrid_binary_translator <bin>            (direct route where eligible, else IR)
  ir      - hybrid_binary_translator --force_ir <bin> (every block through the IR route)
  ir_opt  - hybrid_binary_translator --optimize_ir <bin> (IR route with the dead-ZF pass)

Usage:
  tests/bench_routes.py [--reps N] [--interp PATH] [--translator PATH]
                        [--extra-interp NAME=PATH ...] [--gen-loops N,N,...] [--out CSV | --append CSV]
"""
import argparse
import csv
import os
import statistics
import struct
import subprocess
import sys
import tempfile
import time
from pathlib import Path

TESTS_DIR = Path(__file__).resolve().parent
CA_PROJ = TESTS_DIR.parent
REPO = CA_PROJ.parent


def run(cmd, timeout):
    t0 = time.perf_counter_ns()
    try:
        p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=timeout)
        out, code = p.stdout.decode(errors="replace"), p.returncode
    except subprocess.TimeoutExpired:
        out, code = "(timed out)", -1
    return time.perf_counter_ns() - t0, out.strip(), code


def bin_to_text(bin_path, txt_path):
    data = bin_path.read_bytes()
    hex_bytes = [f"{b:02X}" for b in data] + ["F4"]
    # 16 bytes per line: the interpreter's loader reads 200-char chunks and splits longer lines mid-byte
    txt_path.write_text("\n".join(" ".join(hex_bytes[i:i + 16]) for i in range(0, len(hex_bytes), 16)) + "\n")


def expected_output(bin_path):
    exp = bin_path.with_suffix(".expect")
    if not exp.exists():
        return None
    return "\n".join(l for l in exp.read_text().splitlines() if not l.startswith("#")).strip()


def loop_bin(iterations):
    # mov eax,0 ; mov ecx,-N ; L: inc eax ; inc ecx ; jne L   (same shape as big_loop)
    return (b"\xB8" + struct.pack("<I", 0) + b"\xB9" + struct.pack("<i", -iterations)
            + b"\x40\x41\x75\xFC")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--reps", type=int, default=30, help="timed runs per bin and route (median is reported)")
    ap.add_argument("--translator", default=str(CA_PROJ / "build-rc" / "hybrid_binary_translator"))
    ap.add_argument("--interp", default=str(REPO / "c"))
    ap.add_argument("--extra-interp", action="append", default=[], metavar="NAME=PATH",
                    help="time another interpreter build as an extra column, e.g. interp_O2=/path/to/c")
    ap.add_argument("--cases", default=str(TESTS_DIR / "cases"))
    ap.add_argument("--gen-loops", default="",
                    help="comma-separated iteration counts: also time generated big_loop-style bins")
    ap.add_argument("--timeout", type=float, default=60)
    ap.add_argument("--out", default=str(TESTS_DIR / "output" / "route_times.csv"))
    ap.add_argument("--append", metavar="CSV",
                    help="append rows to this existing CSV (same route columns) instead of writing a new one")
    args = ap.parse_args()

    for exe in [args.translator, args.interp]:
        if not os.access(exe, os.X_OK):
            sys.exit(f"not executable: {exe}")

    interps = [("interp", args.interp)]
    for spec in args.extra_interp:
        name, _, path = spec.partition("=")
        interps.append((name, path))

    work = Path(tempfile.mkdtemp(prefix="bench_routes_"))
    bins = sorted(Path(args.cases).glob("*.bin"))
    for n in filter(None, args.gen_loops.split(",")):
        p = work / f"gen_loop_{n}.bin"
        p.write_bytes(loop_bin(int(n)))
        bins.append(p)

    routes = [(name, (lambda path: lambda b, t: [path, "--program", str(t), "--interp-only"])(path))
              for name, path in interps]
    routes += [("hybrid", lambda b, t: [args.translator, str(b)]),
               ("ir", lambda b, t: [args.translator, "--force_ir", str(b)]),
               ("ir_opt", lambda b, t: [args.translator, "--optimize_ir", str(b)])]

    empty_bin, hlt_txt = work / "empty.bin", work / "empty.txt"
    empty_bin.write_bytes(b"")
    bin_to_text(empty_bin, hlt_txt)
    baseline_cmd = {name: fn(empty_bin, hlt_txt) for name, fn in routes}
    baseline_samples = {name: [] for name, _ in routes}

    def measure(name, cmd):
        """Returns (median net ms, median raw ms, output, exit code)."""
        run(baseline_cmd[name], args.timeout)  # warm-up
        run(cmd, args.timeout)
        raw, net = [], []
        for _ in range(args.reps):
            base_dt, _, _ = run(baseline_cmd[name], args.timeout)
            dt, out, code = run(cmd, args.timeout)
            baseline_samples[name].append(base_dt)
            raw.append(dt)
            net.append(dt - base_dt)
        return statistics.median(net) / 1e6, statistics.median(raw) / 1e6, out, code

    rows = []
    for b in bins:
        t = work / (b.stem + ".txt")
        bin_to_text(b, t)
        row = {"bin": b.stem, "bytes": b.stat().st_size}
        exp = expected_output(b)
        for name, fn in routes:
            net_ms, raw_ms, out, code = measure(name, fn(b, t))
            row[f"{name}_ms"] = round(net_ms, 4)
            row[f"{name}_raw_ms"] = round(raw_ms, 4)
            if name in ("hybrid", "ir", "ir_opt"):
                actual = (out + "\n" if out else "") + f"exit={code}"
                row[f"{name}_result"] = ("ok" if actual == exp else "MISMATCH") if exp is not None else f"exit={code}"
            else:
                row[f"{name}_result"] = out.splitlines()[-1] if out else f"exit={code}"
        rows.append(row)
        print(f"{b.stem:20s} " + "  ".join(f"{n}={row[f'{n}_ms']:8.3f}ms" for n, _ in routes))

    names = [n for n, _ in routes]
    fields = (["bin", "bytes"] + [f"{n}_ms" for n in names] + [f"{n}_raw_ms" for n in names]
              + [f"{n}_result" for n in names])
    if args.append:
        out_path = Path(args.append)
        with out_path.open(newline="") as f:
            if next(csv.reader(f)) != fields:
                sys.exit(f"{out_path}: columns differ from this run's routes, not appending")
    else:
        base = Path(args.out)
        k = 1
        while (out_path := base.with_name(f"{base.stem}_{k}{base.suffix}")).exists():
            k += 1
        out_path.parent.mkdir(parents=True, exist_ok=True)
    with out_path.open("a" if args.append else "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=fields)
        if not args.append:
            w.writeheader()
        w.writerows(rows)
        w.writerow({"bin": "#baseline",
                    **{f"{n}_raw_ms": round(statistics.median(baseline_samples[n]) / 1e6, 4) for n in names}})
    print(f"\nwrote {out_path}  (*_ms = median of (run - paired baseline run), {args.reps} reps; "
          f"*_raw_ms = median wall time)")


if __name__ == "__main__":
    main()
