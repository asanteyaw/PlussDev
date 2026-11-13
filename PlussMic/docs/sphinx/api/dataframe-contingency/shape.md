<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 09195badbb06 -->

# ContingencyTable::shape

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
std::vector<std::int64_t> ContingencyTable::shape() const
```

## What It Does

Returns the size of each retained dimension, in dimension order.

The output is aligned with `dims()`, so `shape()[i]` is the number of levels in
`dims()[i]`.

## Returns

`std::vector<std::int64_t>`
    One extent per retained dimension.

## When To Use It

Use `shape()` when you need to know the retained tensor geometry before deciding
whether to render, facet, or export to `to_array()`.

## Examples

Example 1. A two-way `city × channel` table with two sorted levels on each side
has shape `[2, 2]`.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

const auto shape = df.contingency("city", "channel").shape();
std::cout << "[" << shape[0] << ", " << shape[1] << "]\n";
```

### Output

```text
[2, 2]
```

Example 2. A three-way table becomes smaller after marginalizing one
dimension away.

```cpp
auto df = DataFrame({
    {"city", Slate(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Slate(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Slate(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

const auto full = df.contingency({"city", "channel", "weekend"}).shape();
const auto collapsed = df.contingency({"city", "channel", "weekend"})
                           .marginalize({"weekend"})
                           .shape();

std::cout << "[" << full[0] << ", " << full[1] << ", " << full[2] << "]\n";
std::cout << "[" << collapsed[0] << ", " << collapsed[1] << "]\n";
```

### Output

```text
[2, 2, 2]
[2, 2]
```

## Notes

- `shape()` and `dims()` should be read together. `shape()[i]` is meaningful only with
  the corresponding `dims()[i]`.
- `share()` and `condition(...)` do not change `shape()` because they preserve the
  retained structure.
