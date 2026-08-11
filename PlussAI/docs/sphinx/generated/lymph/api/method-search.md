# `Search`

## API Signature

- `axes_(arg1: std::move(axes)), arg2: kind_(kind), arg3: trials_(trials), arg4: random_(random), arg5: split_(make_cross_validation_split_options(cv)), arg6: cv_(std::move(cv)) -> Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, CrossValidationOptions cv) :`
- `axes_(arg1: std::move(axes)), arg2: kind_(kind), arg3: trials_(trials), arg4: random_(random), arg5: split_(std::move(split)) -> Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, SplitOptions split) :`

## Notes

Namespace-level API function.

## Inputs

- `arg1` (`std::move(axes))`)
  Method argument; see signature type for accepted form.
- `arg2` (`kind_(kind)`)
  Method argument; see signature type for accepted form.
- `arg3` (`trials_(trials)`)
  Method argument; see signature type for accepted form.
- `arg4` (`random_(random)`)
  Method argument; see signature type for accepted form.
- `arg5` (`split_(make_cross_validation_split_options(cv)) | split_(std::move(split)`)
  Method argument; see signature type for accepted form.
- `arg6` (`cv_(std::move(cv)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, CrossValidationOptions cv) :`.
- Returns `Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, SplitOptions split) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = axes_(/* arg1 */, /* arg2 */, /* arg3 */, /* arg4 */, /* arg5 */, /* arg6 */);
out.info("Search", value);
```

**Expected output**

```text
Search returned Search(std::vector<GridAxis> axes, SearchKind kind, std::int64_t trials, RandomState random, CrossValidationOptions cv) :
```
