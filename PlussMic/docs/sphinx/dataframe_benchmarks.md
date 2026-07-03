# DataFrame Benchmark Results

This gate compares equivalent PlussMic and pandas workloads for alignment,
grouping, key unification, wide-to-long reshaping, fixed windows, and covariance.
It is a reproducible engineering benchmark, not a claim that the two runtimes
have identical allocation, caching, or process-startup behavior.

## Reproduce

```bash
cmake --build build_cpp_env --target plussmic_table_gap_benchmark
conda run -n cpp_env python PlussMic/benchmarks/compare_dataframe_ops.py \
  --rows 1000000 \
  --output PlussMic/benchmarks/latest_dataframe_ops.json
```

## Latest million-case run

The run used pandas 2.2.3. Unification was capped at 100,000 input cases per side
because the repeated-key fixture produces 10 million joined rows.

| Operation | PlussMic | pandas | Pluss/pandas |
| --- | ---: | ---: | ---: |
| Alignment | 1191.35 ms | 30.31 ms | 39.30× |
| Grouping | 95.80 ms | 10.68 ms | 8.97× |
| Unification | 1055.48 ms | 290.34 ms | 3.64× |
| Reshaping | 132.07 ms | 42.12 ms | 3.14× |
| Moving mean | 88.51 ms | 33.92 ms | 2.61× |
| Covariance | 134.18 ms | 17.23 ms | 7.79× |

The benchmark passes as a reproducibility and regression gate; it does not pass
PlussMic as performance-equivalent to pandas. Numeric window kernels are the
closest result in this fixture. Case-key alignment and grouping remain the
largest optimization targets.
