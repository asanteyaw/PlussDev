# `HoldoutSplit::has_frame`

## API Signature

- `HoldoutSplit::has_frame() -> bool`

## Notes

Boolean introspection helper that reports whether a capability/state is available.

## Inputs

None.

## Output

- Returns `bool`.

## Options

Option semantics used by this method:

- `HoldoutSplit` (`struct`):
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
auto value = HoldoutSplit::has_frame();
out.info("has_frame", value);
```

**Expected output**

```text
has_frame: true
```
