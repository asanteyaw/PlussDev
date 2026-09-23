<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 9c52b371c186 -->

# ContingencyTable::dims

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable::dims() const -> std::vector<std::string>
```

## What It Does

Returns the retained dimension names, in tensor order.

This is the name-side companion to `shape()`, and it also defines the dimension
order used by `to_array()`.

## Returns

`std::vector<std::string>`
    The retained dimension names in their current order.

## When To Use It

Use `dims()` whenever you need to answer one of these questions before proceeding:

- which dimensions are still present after slicing or marginalizing?
- what order do the axes of `to_array()` correspond to?
- what row/column names are legal to hand to `to_frame(...)`, `facet(...)`,
  `slice(...)`, `marginalize(...)`, or `condition(...)`?

## Examples

Example 1. The one-list constructor preserves the supplied dimension order.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
    {"weekend", Thread(pluss::make_note({"no", "no", "no", "yes", "yes"}))},
});

const auto dims = df.contingency({"city", "channel", "weekend"}).dims();
std::cout << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]\n";
```

### Output

```text
[city, channel, weekend]
```

Example 2. The row/column overload retains dimensions as `rows..., columns...`.

```cpp
const auto dims = df.contingency({"city", "weekend"}, {"channel"}).dims();
std::cout << "[" << dims[0] << ", " << dims[1] << ", " << dims[2] << "]\n";
```

### Output

```text
[city, weekend, channel]
```

Example 3. `slice(...)` and `marginalize(...)` physically remove dimensions
from the retained object.

```cpp
auto ct = df.contingency({"city", "channel", "weekend"});
const auto sliced = ct.slice({{"weekend", "yes"}}).dims();
const auto collapsed = ct.marginalize({"channel"}).dims();

std::cout << "[" << sliced[0] << ", " << sliced[1] << "]\n";
std::cout << "[" << collapsed[0] << ", " << collapsed[1] << "]\n";
```

### Output

```text
[city, channel]
[city, weekend]
```

## Notes

- `dims()` is not just metadata. It is the authoritative axis order for `shape()` and
  `to_array()`.
- `slice(...)` and `marginalize(...)` physically remove dimensions, so `dims()` is one
  of the fastest sanity checks after either operation.
- The one-list constructor preserves your supplied order. The row/column constructor
  retains dimensions as `rows..., columns...`.
