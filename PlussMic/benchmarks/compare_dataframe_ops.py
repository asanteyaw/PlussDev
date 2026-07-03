#!/usr/bin/env python3
"""Compare PlussMic alignment/group/unify/reshape/window throughput with pandas."""

from __future__ import annotations

import argparse
import json
import pathlib
import subprocess
import time

import numpy as np
import pandas as pd


def measure(fn):
    started = time.perf_counter()
    result = fn()
    elapsed = (time.perf_counter() - started) * 1000.0
    return elapsed, result


def pandas_results(rows: int) -> dict[str, float]:
    case = np.arange(rows, dtype=np.int64)
    left = pd.DataFrame(
        {
            "key": case % 1000,
            "x": (case % 997).astype(np.float64),
            "y": (case % 991).astype(np.float64),
        },
        index=case,
    )
    right_case = case + rows // 2
    right = pd.DataFrame(
        {
            "key": case % 1000,
            "x": (case % 997).astype(np.float64),
            "y": (case % 991).astype(np.float64),
        },
        index=right_case,
    )
    join_rows = min(rows, 100_000)
    alignment, _ = measure(lambda: left + right)
    grouping, _ = measure(lambda: left.groupby("key", sort=False)["x"].sum())
    unification, _ = measure(
        lambda: left.iloc[:join_rows].merge(
            right.iloc[:join_rows], on="key", how="inner"
        )
    )
    reshaping, _ = measure(
        lambda: left.reset_index().melt(
            id_vars=["index", "key"], value_vars=["x", "y"]
        )
    )
    windows, _ = measure(lambda: left[["x", "y"]].rolling(128, min_periods=1).mean())
    statistics, _ = measure(lambda: left[["key", "x", "y"]].cov())
    return {
        "alignment_ms": alignment,
        "grouping_ms": grouping,
        "unification_ms": unification,
        "reshaping_ms": reshaping,
        "windows_ms": windows,
        "statistics_ms": statistics,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--rows", type=int, default=100_000)
    parser.add_argument(
        "--pluss-binary",
        type=pathlib.Path,
        default=pathlib.Path("build_cpp_env/PlussMic/test/plussmic_table_gap_benchmark"),
    )
    parser.add_argument("--output", type=pathlib.Path)
    args = parser.parse_args()

    completed = subprocess.run(
        [str(args.pluss_binary), str(args.rows)],
        check=True,
        capture_output=True,
        text=True,
    )
    pluss = json.loads(completed.stdout.strip().splitlines()[-1])
    pandas = pandas_results(args.rows)
    comparisons = {
        operation: {
            "pluss_ms": pluss[f"{operation}_ms"],
            "pandas_ms": pandas[f"{operation}_ms"],
            "pluss_over_pandas": (
                pluss[f"{operation}_ms"] / pandas[f"{operation}_ms"]
            ),
        }
        for operation in (
            "alignment",
            "grouping",
            "unification",
            "reshaping",
            "windows",
            "statistics",
        )
    }
    report = {
        "rows": args.rows,
        "pandas_version": pd.__version__,
        "comparisons": comparisons,
    }
    rendered = json.dumps(report, indent=2)
    print(rendered)
    if args.output:
        args.output.write_text(rendered + "\n")


if __name__ == "__main__":
    main()
