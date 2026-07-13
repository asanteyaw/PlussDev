# DataFrame::shuffle

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::shuffle(std::optional<std::uint64_t> random_state = std::nullopt) -> DataFrame
```

## What It Does

Randomly permutes row order while keeping the feature axis unchanged.

## Parameters

**random_state**
    Deterministic seed for reproducible row shuffles.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Shuffle row order while keeping the feature order unchanged.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shuffle(7);
```

### Output

```text
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema
```

## Practical Behavior

- `shuffle(random_state)` is the row-order helper for the common case where you want to permute rows and leave the feature order alone.
- It is equivalent to `reshuffle(axis=0, ...)`, but reads more directly in train/test split and stochastic preview workflows.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="skew.html">
    <span class="label">Next</span>
    <span class="title">skew</span>
  </a>
</div>
