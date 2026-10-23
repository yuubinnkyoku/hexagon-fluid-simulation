"""Unit checks for Android benchmark log parsing; no phone required."""
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "scripts"))
from bench_android import parse_output  # noqa: E402


VALID = """backend=cpu_jacobi
grid=16x16
spacing=0.05882353
iterations=540
converged=true
relative_residual=0.00009386
max_abs_pressure_error=0.00009304
solver_wall_time_ms=0.23143300
"""


class ParseBenchmarkTests(unittest.TestCase):
    def test_valid(self):
        result = parse_output(VALID)
        self.assertEqual(result["iterations"], 540)
        self.assertTrue(result["converged"])
        self.assertAlmostEqual(result["solver_wall_time_ms"], 0.231433)

    def test_missing_field(self):
        with self.assertRaises(ValueError):
            parse_output(VALID.replace("iterations=540\n", ""))

    def test_wrong_backend(self):
        with self.assertRaises(ValueError):
            parse_output(VALID.replace("cpu_jacobi", "npu_guess"))

    def test_nonfinite_timing(self):
        with self.assertRaises(ValueError):
            parse_output(VALID.replace("0.23143300", "nan"))


if __name__ == "__main__":
    unittest.main()
