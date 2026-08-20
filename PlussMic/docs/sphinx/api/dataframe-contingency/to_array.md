<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 479f9bb84868 -->

# ContingencyTable::to_array

[Back to ContingencyTable API Reference](../../dataframe_contingency_accessor_api_reference)

## Signature

```cpp
ContingencyTable::to_array() const -> pluss::Series
```

## What It Does

Returns the retained contingency tensor as an N-dimensional `pluss::Series`.

Use this when you want the actual dense N-dimensional payload rather than a 2D
rendered `DataFrame`.

## Examples

Example 1. Inspect the dense count tensor's rank, shape, and flattened cell
values.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"city", Thread(pluss::make_note({"A", "A", "B", "B", "B"}))},
    {"channel", Thread(pluss::make_note({"Cash", "Card", "Cash", "Cash", "Card"}))},
});

auto array = df.contingency("city", "channel")
               .to_array()
               .to(pluss::codem::Platform::CPU)
               .contiguous();
std::cout << array.dim() << "\n";
std::cout << array.sizes()[0] << " " << array.sizes()[1] << "\n";

const auto* raw = reinterpret_cast<const std::int64_t*>(array.raw_data());
for (std::int64_t i = 0; i < array.numel(); ++i) {
  std::cout << raw[i] << " ";
}
std::cout << "\n";
```

### Output

```text
2
2 2
1 1 1 2
```

Example 2. `to_array()` respects the current value interpretation, so shares
come back as floating-point tensors.

```cpp
auto array = df.contingency("city", "channel")
               .share()
               .to_array()
               .to(pluss::codem::Platform::CPU)
               .contiguous();

const auto* raw = reinterpret_cast<const double*>(array.raw_data());
for (std::int64_t i = 0; i < array.numel(); ++i) {
  std::cout << raw[i] << " ";
}
std::cout << "\n";
```

### Output

```text
0.2 0.2 0.2 0.4
```
