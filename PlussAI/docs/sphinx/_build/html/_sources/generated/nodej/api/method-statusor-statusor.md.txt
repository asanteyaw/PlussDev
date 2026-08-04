# `StatusOr::StatusOr`

## API Signature

- `StatusOr(arg1: Status status) : status_(std::move(status)) -> auto`
- `StatusOr(arg1: T value) : status_(Status::success()), arg2: value_(std::move(value)), true: has_value_() -> auto`
- `status_(arg1: std::move(status)), arg2: value_(std::move(value)), arg3: has_value_(status_.ok()) -> StatusOr(Status status, T value) :`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`Status status) : status_(std::move(status) | T value) : status_(Status::success()) | std::move(status))`)
  Method argument; see signature type for accepted form.
- `arg2` (`value_(std::move(value))`)
  Method argument; see signature type for accepted form.
- `arg3` (`has_value_(status_.ok()`)
  Method argument; see signature type for accepted form.
- `true` (`has_value_(`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr(Status status, T value) :`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = StatusOr(/* arg1 */);
out.info("StatusOr", value);
```

**Expected output**

```text
StatusOr returned result
```
