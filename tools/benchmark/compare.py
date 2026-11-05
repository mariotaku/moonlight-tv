#!/usr/bin/env python3
"""Compare matched, independently recorded Moonlight TV streaming runs.

These metrics describe received and decoder-submitted frames. They do NOT
measure frames actually presented on the TV panel.
"""
import argparse
import json
import re
from pathlib import Path
from statistics import median

MARKER = "ML_BENCH_V1 "
FIELDS = (
    "duration_ms", "total", "received", "submitted", "dropped",
    "host_01ms", "submit_us", "decoder_us", "rtt_ms", "rtt_var_ms",
    "tv_copy_us", "tv_feed_us",
)
VALUES = re.compile(r"([a-z_][a-z_0-9]*)=(-?[0-9]+)")
METRICS = (
    ("received_fps", "Received FPS"),
    ("submitted_fps", "Decoder-submitted FPS"),
    ("drop_pct", "Network frame drops (%)"),
    ("host_ms", "Host processing (ms)"),
    ("submit_ms", "Submit delay (ms)"),
    ("tv_copy_ms", "TV frame copy duration (ms)"),
    ("tv_feed_ms", "TV→SS4S Feed call duration (ms)"),
    ("decoder_ms", "Estimated NDL decoder latency (ms)"),
    ("rtt_ms", "RTT (ms)"),
)


def parse_line(line):
    if MARKER not in line:
        return None
    # Parse complete tokens so unknown, malformed, or duplicate counters do
    # not silently alter the outcome of an A/B comparison.
    values = {}
    for token in line.split(MARKER, 1)[1].split():
        match = VALUES.fullmatch(token)
        if match is None or match.group(1) in values:
            raise ValueError("invalid or duplicate benchmark field")
        values[match.group(1)] = match.group(2)
    if set(values) != set(FIELDS):
        raise ValueError("bad benchmark sample fields")
    row = {key: int(value) for key, value in values.items()}
    if (row["duration_ms"] <= 0 or row["total"] < row["received"] or
        row["received"] < row["submitted"] or row["dropped"] > row["total"] or
        any(row[k] < 0 for k in FIELDS if k != "decoder_us") or
        row["decoder_us"] < -1):
        raise ValueError("invalid benchmark sample counters")
    return row


def summarize_run(path, min_samples=20):
    rows = []
    for line_no, line in enumerate(Path(path).read_text(errors="replace").splitlines(), 1):
        try:
            row = parse_line(line)
        except ValueError as exc:
            raise ValueError(f"{path}:{line_no}: {exc}") from exc
        if row is not None:
            rows.append(row)
    if len(rows) < min_samples:
        raise ValueError(f"{path}: only {len(rows)} benchmark windows; need {min_samples}")
    def total(key):
        return sum(row[key] for row in rows)
    ms = total("duration_ms")
    recv = total("received")
    submit = total("submitted")
    expected = total("total")
    lat = [row for row in rows if row["decoder_us"] >= 0 and row["submitted"]]
    return {
        "windows": len(rows),
        "seconds": ms / 1000,
        "received_fps": recv * 1000 / ms,
        "submitted_fps": submit * 1000 / ms,
        "drop_pct": total("dropped") * 100 / expected if expected else None,
        "host_ms": total("host_01ms") / recv / 10 if recv else None,
        "submit_ms": total("submit_us") / submit / 1000 if submit else None,
        "tv_copy_ms": total("tv_copy_us") / recv / 1000 if recv else None,
        "tv_feed_ms": total("tv_feed_us") / recv / 1000 if recv else None,
        "decoder_ms": (sum(row["decoder_us"] * row["submitted"] for row in lat) /
                       sum(row["submitted"] for row in lat) / 1000) if lat else None,
        "rtt_ms": sum(row["rtt_ms"] * row["duration_ms"] for row in rows) / ms,
    }


def aggregate(paths, min_samples):
    return [summarize_run(path, min_samples) for path in paths]


def value_of(runs, key):
    valid = [run[key] for run in runs if run[key] is not None]
    return median(valid) if len(valid) == len(runs) else None


def compare(base, candidate):
    return {key: {"base": value_of(base, key), "candidate": value_of(candidate, key)}
            for key, _ in METRICS}


def report_markdown(profile, base_sha, candidate_sha, results, count):
    lines = [
        "# Moonlight TV matched streaming comparison", "",
        f"Profile: `{profile}`  ",
        f"Base: `{base_sha}` · Candidate: `{candidate_sha}`  ",
        f"Independent runs per build: **{count}**", "",
        "| Measurement | Base median | PR median | Difference |",
        "|---|---:|---:|---:|",
    ]
    for key, title in METRICS:
        b, c = results[key]["base"], results[key]["candidate"]
        if b is None or c is None:
            lines.append(f"| {title} | n/a | n/a | n/a |")
        else:
            difference = f"{c-b:+.2f}" + (f" ({(c-b)/b*100:+.1f}%)" if b else "")
            lines.append(f"| {title} | {b:.2f} | {c:.2f} | {difference} |")
    lines += [
        "", "Interpretation: received FPS and decoder-submitted FPS are **not displayed FPS**.",
        "All numbers are measured client observations, not a statistical significance test.",
        "Capture raw client logs, settings and host/decoder metadata alongside this report.",
    ]
    return "\n".join(lines) + "\n"


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--profile", required=True, help="Identical workload/settings identifier")
    parser.add_argument("--base-sha", required=True)
    parser.add_argument("--candidate-sha", required=True)
    parser.add_argument("--base", nargs="+", required=True, type=Path, metavar="LOG")
    parser.add_argument("--candidate", nargs="+", required=True, type=Path, metavar="LOG")
    parser.add_argument("--min-runs", type=int, default=5)
    parser.add_argument("--min-windows", type=int, default=20)
    parser.add_argument("--output", type=Path, help="Write Markdown to this path")
    parser.add_argument("--json-output", type=Path, help="Write summary JSON to this path")
    args = parser.parse_args(argv)
    if (args.min_runs < 1 or args.min_windows < 1 or
        len(args.base) != len(args.candidate) or len(args.base) < args.min_runs or
        args.base_sha == args.candidate_sha):
        parser.error("require equal-length baseline/candidate runs, distinct SHAs and positive minimums")
    try:
        baseline = aggregate(args.base, args.min_windows)
        candidate = aggregate(args.candidate, args.min_windows)
    except (OSError, ValueError) as exc:
        parser.error(str(exc))
    results = compare(baseline, candidate)
    markdown = report_markdown(args.profile, args.base_sha, args.candidate_sha,
                               results, len(baseline))
    print(markdown, end="")
    if args.output:
        args.output.write_text(markdown)
    if args.json_output:
        args.json_output.write_text(json.dumps({
            "schema_version": 1, "profile": args.profile,
            "base_sha": args.base_sha, "candidate_sha": args.candidate_sha,
            "base_runs": baseline, "candidate_runs": candidate,
            "comparison": results, "displayed_fps_measured": False,
        }, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
