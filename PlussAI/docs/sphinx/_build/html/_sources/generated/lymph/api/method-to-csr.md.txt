# `to_csr`

## API Signature

- `to_csr(sparse: const Triplet&) -> Csr`

## Notes

Namespace-level API function.

## Inputs

- `sparse` (`const Triplet&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `Csr`.

## Options

No explicit option struct/enum parameters were detected for this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto value = to_csr(/* sparse */);
out.info("to_csr", value);
```

**Expected output**

```text
to_csr returned Csr
```
