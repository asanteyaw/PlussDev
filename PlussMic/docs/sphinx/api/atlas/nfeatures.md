<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d37344265293 -->

# Atlas::nfeatures

[Back to Atlas API Reference](../../atlas_api_reference)

## Signature

```cpp
Atlas::nfeatures() const -> std::int64_t
```

## What It Does

Returns the column count shared by every frame in the stack — the same schema
guarantee behind [`ncases()`](ncases.md), applied to columns instead of rows. Every
frame in the stack has the same number of features, the same names, and the same
per-column kind and dtype, so `nfeatures()` describes the whole `Atlas`, not just one
frame within it.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of features (columns) in every frame of the stack. `0` for an atlas built
    from an empty frame vector.

## Examples

Example 1. Every quarter in the stack has the same two features.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"sales",  Thread(pluss::make_series({100.0, 120.0}))},
    {"orders", Thread(pluss::make_series({12.0, 15.0}))},
});
auto q2 = DataFrame({
    {"sales",  Thread(pluss::make_series({110.0, 90.0}))},
    {"orders", Thread(pluss::make_series({14.0, 9.0}))},
});
auto atlas = Atlas({q1, q2});

std::cout << atlas.nfeatures();
```

### Output

```text
2
```

Example 2. Stacking frames with mismatched column counts throws immediately, at
construction.

```cpp
using namespace pluss::table;

auto q1 = DataFrame({
    {"sales",  Thread(pluss::make_series({100.0, 120.0}))},
    {"orders", Thread(pluss::make_series({12.0, 15.0}))},
});
auto q2_wrong_shape = DataFrame({{"sales", Thread(pluss::make_series({110.0, 90.0}))}});

try {
  auto atlas = Atlas({q1, q2_wrong_shape});
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Atlas: all frames must have identical feature count
```

## Notes

- Requires a defined `Atlas`; throws `"Atlas: undefined handle"` on a
  default-constructed `Atlas()`.
- Every frame in the stack shares this exact feature count, name set, and per-column
  kind/dtype by construction — see [`column_names`](column_names.md) for the names
  themselves.

## See Also

- [`Atlas::column_names`](column_names.md): the actual feature names behind this count.
- [`Atlas::ncases`](ncases.md): the other shared dimension, the row count.
- [`Atlas::features`](features.md): project the whole stack down to a named subset of
  features, changing this count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ncases.html">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="column_names.html">
    <span class="label">Next</span>
    <span class="title">column_names</span>
  </a>
</div>
