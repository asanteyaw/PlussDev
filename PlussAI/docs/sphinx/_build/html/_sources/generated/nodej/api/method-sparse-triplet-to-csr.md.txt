# `sparse_triplet_to_csr`

## API Signature

- `sparse_triplet_to_csr(sparse: const SparseTripletMatrix&) -> StatusOr<SparseCsrMatrix>`

## Notes

Namespace-level API function.

## Inputs

- `sparse` (`const SparseTripletMatrix&`)
  Method argument; see signature type for accepted form.

## Output

- Returns `StatusOr<SparseCsrMatrix>`. Check `.ok()` first; on success read `.value()` for the computed result.

## Options

Option semantics used by this method:

- `SparseTripletMatrix` (`struct`):
  - `Series row_indices = Series::undefined()`
    Option field used by this method.
  - `Series column_indices = Series::undefined()`
    Controls fold/trial/count sizing.
  - `Series values = Series::undefined()`
    Option field used by this method.
  - `std::int64_t rows = 0`
    Option field used by this method.
  - `std::int64_t cols = 0`
    Option field used by this method.

## Examples

### Example 1 (Minimal Usage)

```cpp
pluss::OutputEngine out;
auto res = sparse_triplet_to_csr(/* sparse */);
if (!res.ok()) {
  out.error("sparse_triplet_to_csr failed", res.status().message);
} else {
  out.info("sparse_triplet_to_csr ok");
}
```

**Expected output**

```text
sparse_triplet_to_csr: ok
status.ok() == true
result.value() is available
```
