<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 57058e82e86f -->

# ContingencyTable::ndim

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable::ndim() const -> std::int64_t
```

## What It Does

Returns how many retained dimensions the contingency table currently has.

This is the N in the object's N-dimensional joint distribution. It changes
after `slice(...)` and `marginalize(...)` because those operations physically
remove dimensions from the retained object.

## Returns

`std::int64_t`
    The number of currently retained dimensions.

## When To Use It

Use `ndim()` as a quick structural check before deciding how to render:

- `0` -> scalar contingency
- `1` -> one-dimensional frequency/share table
- `2` -> ordinary row-by-column contingency
- `3+` -> retained higher-dimensional object that may need `to_frame(...)` or
  `facet(...)`

## Examples

Example 1. A classic two-way contingency table has two retained dimensions.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

std::cout << df.contingency("city", "channel").ndim() << "\n";
```

### Output

```text
2
```

Example 2. Slicing one level out of a three-way table removes one retained
dimension.

```cpp
auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

auto ct = df.contingency({"city", "channel", "weekend"});
std::cout << ct.ndim() << "\n";
std::cout << ct.slice({{"weekend", "yes"}}).ndim() << "\n";
```

### Output

```text
3
2
```

Example 3. If you slice away every dimension, the retained object becomes a
scalar contingency with `0` dimensions.

```cpp
auto scalar_ct = df.contingency({"city", "channel"})
                  .slice({{"city", "A"}, {"channel", "Card"}});
std::cout << scalar_ct.ndim() << "\n";
```

### Output

```text
0
```

## Notes

- `ndim()` counts retained dimensions, not rendered row/column sides.
- `condition(...)` and `share()` do not change `ndim()` because they only change value
  interpretation, not structure.
