# Before/after Moonlight TV streaming benchmarks

This optional, **hardware-observed** benchmark workflow enables matched
main-vs-PR evidence without granting CI control of anyone's TV.

## Instrumented build

Compile **both** the baseline (usually main) and candidate with
`-DMOONLIGHT_BENCH_STATS=ON` using the same compiler, toolchain,
CMake build type and package dependencies. The setting defaults to OFF.

The client writes one `ML_BENCH_V1` log line per completed ~1-second
stats window, even while the stats overlay is closed. It records raw
frame counts and latency sums; no per-frame logging, network listener,
input automation, or background data upload is added. Capture logs using
your platform's normal app log facility. Keep one log per independent run.

The existing webOS CI already packages PR builds as downloadable .ipk files.
Once this workflow has landed on main, the optional **Prepare matched webOS
benchmark builds** GitHub Actions workflow can create baseline/candidate IPKs
with identical build options and commit provenance. For a PR from a fork, pass
`refs/pull/NUMBER/head` as the candidate ref. Both revisions must already
include the optional telemetry feature. It builds on public hosted runners;
it never controls, connects to, or installs anything on a TV.

Do not confuse a successful install or launch with a successful video
benchmark.

## Run and compare

Fix the host build and workload as well as client settings. Record resolution,
FPS, codec, bitrate, HDR/SDR, host SHA, decoder backend, TV model/firmware
and connection type. Use the same scene and duration for every run.
Alternate A/B runs (e.g. ABBAABBAAB), with >=5 independent runs on each
revision, >=20 seconds of sampled playback after warmup per run.

After saving normal TV logs to separate files, run:

```sh
python3 tools/benchmark/compare.py \
  --profile "LG C3 4K60 HEVC 150Mbps SDR" \
  --base-sha BASE_SHA --candidate-sha PR_SHA \
  --base runs/base1.log runs/base2.log runs/base3.log runs/base4.log runs/base5.log \
  --candidate runs/pr1.log runs/pr2.log runs/pr3.log runs/pr4.log runs/pr5.log \
  --output summary.md --json-output summary.json
```

Attach `summary.md`, `summary.json`, and the raw logs
to the PR (or to an artifact store linked from the PR). Before public sharing,
review logs for private hostnames, IP addresses, device identifiers, pairing
tokens, usernames, and other sensitive information; redact only a *copy* of
the raw logs and record the redaction.

## What the numbers mean

- **Received FPS:** complete decoded-unit callbacks reaching the client.
- **Decoder-submitted FPS:** successful submissions to the SS4S video backend.
  It does NOT mean frames reached the TV display.
- **Frame drops:** gaps in the host's numbered frame sequence as observed
  by the client; not a measurement of presentation drops.
- **Decoder latency:** SS4S' existing estimated latency, if available.
- **Host processing latency:** Moonlight's host-reported frame processing
  duration, not the total end-to-end latency.

The output reports medians of per-run aggregates, not confidence bounds.
Do not claim a speedup based on short smoke runs or unmatched workloads.
A reference TV and explicit operator permission are required for physical
runs; upstream CI must never take over a shared display automatically.

## Tests

```sh
python3 -m unittest discover -s tools/benchmark -p 'test_*.py'
```

These synthetic tests validate report parsing and arithmetic only.
They cannot verify a hardware decoder or client presentation cadence.
