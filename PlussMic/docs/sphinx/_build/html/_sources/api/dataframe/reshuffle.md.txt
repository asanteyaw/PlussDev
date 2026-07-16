# DataFrame::reshuffle

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::reshuffle(Axis axis = Axis::Case, std::optional<std::uint64_t> random_state = std::nullopt) -> DataFrame
```

## What It Does

Randomly permutes rows or features, depending on the selected axis.

## Parameters

**axis**
    Shuffle axis. Use `0` to permute rows and `1` to permute feature order.

**random_state**
    Deterministic seed for reproducible row or feature shuffles.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Shuffle rows or features depending on the selected axis.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto rows = df.reshuffle(0, 7);
auto cols = df.reshuffle(1, 7);
```

### Output

```text
rows.head():
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema

cols.head():
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```

## Practical Behavior

- `reshuffle(axis=0)` permutes rows and keeps the feature order unchanged.
- `reshuffle(axis=1)` permutes feature order and keeps the row order unchanged.
- Pass `random_state` when you need deterministic shuffling for docs, tests, or reproducible data splits.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="replace.html">
    <span class="label">Previous</span>
    <span class="title">replace</span>
  </a>
  <a class="next-link" href="rfloordiv.html">
    <span class="label">Next</span>
    <span class="title">rfloordiv</span>
  </a>
</div>
