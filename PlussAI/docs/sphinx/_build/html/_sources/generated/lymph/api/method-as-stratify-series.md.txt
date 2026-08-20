# `as_stratify_series`

## API Signature

- `as_stratify_series(thread: const pluss::table::Thread&) -> Series`

## Notes

Namespace-level API function.

## Inputs

- `thread` (`const pluss::table::Thread&`)
  Pluss numeric vector/tensor input.

## Output

- Returns `Series`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = as_stratify_series(/* thread */);
out.info("as_stratify_series", value);
```

**Expected output**

```text
as_stratify_series returned Series
```
