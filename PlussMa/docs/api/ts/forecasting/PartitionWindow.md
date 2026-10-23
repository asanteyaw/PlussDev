<!-- generated from PartitionWindow.md.in by PlussMa/docs/tools/render_guide.py; edit the template -->

# `pluss::subsp::stats::ts::PartitionWindow`

One training/test window of a `TsPartition`.

`PartitionWindow` is plain data holding 0-based inclusive index ranges; a range with `end < start` is empty.

## Fields

| Field | Type | Default | Meaning |
|---|---|---|---|
| `train_start, train_end` | `std::int64_t` | 0, −1 | The training range. |
| `test_start, test_end` | `std::int64_t` | 0, −1 | The test range. |
| `test_buffer_start, test_buffer_end` | `std::int64_t` | 0, −1 | The buffer of `max_lag` observations before the test range (empty when `max_lag = 0`). |

## Guidance

`train_size()`, `test_size()` and `test_buffer_size()` return the lengths.

## See Also

- [`TsPartition`](TsPartition.md)
