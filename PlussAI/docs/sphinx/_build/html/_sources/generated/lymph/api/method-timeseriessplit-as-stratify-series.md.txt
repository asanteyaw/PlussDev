# `TimeSeriesSplit::as_stratify_series`

## API Signature

- `TimeSeriesSplit::as_stratify_series(thread: const pluss::table::Thread&) -> Series`

## Notes

Instance method on `self`.

## Inputs

- `thread` (`const pluss::table::Thread&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `Series`.

## Options

Option semantics used by this method:

- `TimeSeriesSplit` (`struct`):
  - `pluss::table::DataFrame train_set{}`
    Controls fold/trial/count sizing.
  - `pluss::table::DataFrame test_set{}`
    Option field used by this method.
  - `LabeledBatch train_batch{}`
    Controls fold/trial/count sizing.
  - `LabeledBatch test_batch{}`
    Option field used by this method.
  - `IndexSplit indices{}`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = TimeSeriesSplit::as_stratify_series(/* thread */);
out.info("as_stratify_series", value);
```

**Expected output**

```text
as_stratify_series returned Series
```
