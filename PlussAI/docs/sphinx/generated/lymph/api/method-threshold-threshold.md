# `Threshold::Threshold`

## API Signature

- `scores_(arg1: std::move(scores)), arg2: labels_(std::move(labels)) -> Threshold(Series scores, Series labels) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`std::move(scores))`)
  Method argument; see signature type for accepted form.
- `arg2` (`labels_(std::move(labels)`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Threshold(Series scores, Series labels) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = scores_(/* arg1 */, /* arg2 */);
out.info("Threshold", value);
```

**Expected output**

```text
Threshold returned Threshold(Series scores, Series labels) :
```
