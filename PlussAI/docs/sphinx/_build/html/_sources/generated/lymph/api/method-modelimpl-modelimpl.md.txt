# `ModelImpl::ModelImpl`

## API Signature

- `ModelImpl(arg1: ModelCore in_core) : core(std::move(in_core)) -> auto`

## Notes

Instance method on `self`.

## Inputs

- `arg1` (`ModelCore in_core) : core(std::move(in_core)`)
  Method argument; see signature type for accepted form.

## Output

- Returns method result for the selected overload.

## Options

Option semantics used by this method:

- `ModelImpl` (`struct`):
  - `ModelCore core`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = ModelImpl(/* arg1 */);
out.info("ModelImpl", value);
```

**Expected output**

```text
ModelImpl returned result
```
