import tempfile
import unittest
from pathlib import Path

from compare import compare, parse_line, report_markdown, summarize_run


def sample(**overrides):
    metrics = dict(duration_ms=1000, total=60, received=58, submitted=56,
                   dropped=2, host_01ms=580, submit_us=112000,
                   decoder_us=4000, rtt_ms=8, rtt_var_ms=1,
                   tv_copy_us=58000, tv_feed_us=174000)
    metrics.update(overrides)
    return "app-log-prefix [Benchmark] ML_BENCH_V1 " + " ".join(
        f"{k}={v}" for k, v in metrics.items()) + "\n"


class BenchmarkTests(unittest.TestCase):
    def test_parse(self):
        row = parse_line(sample())
        self.assertEqual(row["received"], 58)
        self.assertIsNone(parse_line("ordinary message"))

    def test_reject_invalid_and_incomplete(self):
        with self.assertRaises(ValueError):
            parse_line(sample(received=70))
        with self.assertRaises(ValueError):
            parse_line(sample().replace(" submit_us=112000", ""))

    def test_reject_duplicate_and_garbage_fields(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            parse_line(sample().strip() + " received=999")
        with self.assertRaisesRegex(ValueError, "invalid"):
            parse_line(sample().strip() + " garbage")
        with self.assertRaisesRegex(ValueError, "invalid"):
            parse_line(sample().strip() + " received=1oops")

    def test_malformed_sample_is_not_ignored(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "bad.log"
            path.write_text(sample() + "ML_BENCH_V1 received=1\n")
            with self.assertRaisesRegex(ValueError, "bad.log:2"):
                summarize_run(path, 1)

    def test_run_aggregates_raw_counters(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "run.log"
            path.write_text(sample() * 22)
            report = summarize_run(path)
            self.assertAlmostEqual(report["received_fps"], 58)
            self.assertAlmostEqual(report["submitted_fps"], 56)
            self.assertAlmostEqual(report["drop_pct"], 100 * 2 / 60)
            self.assertAlmostEqual(report["host_ms"], 1)
            self.assertAlmostEqual(report["submit_ms"], 2)
            self.assertAlmostEqual(report["decoder_ms"], 4)
            self.assertAlmostEqual(report["tv_copy_ms"], 1)
            self.assertAlmostEqual(report["tv_feed_ms"], 3)
            self.assertEqual(report["windows"], 22)

    def test_no_data_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "missing.log"
            path.write_text("No video captured\n")
            with self.assertRaisesRegex(ValueError, "only 0"):
                summarize_run(path)

    def test_incomplete_decoder_stats_not_fabricated(self):
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "run.log"
            path.write_text(sample(decoder_us=-1) * 20)
            self.assertIsNone(summarize_run(path)["decoder_ms"])

    def test_report_does_not_claim_displayed_fps(self):
        base = [{"received_fps": 60, "submitted_fps": 59, "drop_pct": 0,
                 "host_ms": 2, "submit_ms": 3, "tv_copy_ms": 1, "tv_feed_ms": 2,
                 "decoder_ms": 4, "rtt_ms": 5}]
        pr = [{**base[0], "received_fps": 55}]
        text = report_markdown("4K60 HEVC", "aa", "bb", compare(base, pr), 1)
        self.assertIn("-8.3%", text)
        self.assertIn("not displayed FPS", text)
        self.assertIn("Profile: `4K60 HEVC`", text)
        self.assertIn("Base: `aa` · Candidate: `bb`", text)


if __name__ == "__main__":
    unittest.main()
