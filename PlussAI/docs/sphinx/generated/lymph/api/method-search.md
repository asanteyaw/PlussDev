# `Search`

## API Signature

- `axes_(arg1: std::move(axes)), arg2: kind_(kind), arg3: trials_(trials), random: random_() -> Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random) :`

## Notes

Namespace-level API function.

## Inputs

- `arg1` (`std::move(axes))`)
  Method argument; see signature type for accepted form.
- `arg2` (`kind_(kind)`)
  Method argument; see signature type for accepted form.
- `arg3` (`trials_(trials)`)
  Method argument; see signature type for accepted form.
- `random` (`random_(`)
  Randomization control for reproducible behavior.

## Output

- Returns `Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = axes_(/* arg1 */, /* arg2 */, /* arg3 */, /* random */);
out.info("Search", value);
```

**Expected output**

```text
Search returned Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random) :
```
