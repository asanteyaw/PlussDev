# DataFrame::div

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::div(const DataFrame& other) -> DataFrame

DataFrame::div(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::div(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::div(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::div(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::div(double other) -> DataFrame
```

Divide numeric DataFrame values by another aligned frame or by one scalar.

Use `div(...)` when you want elementwise division while preserving the table shape and case/feature structure.

The two overload families cover the common arithmetic routes:
- broadcast one scalar across the selected numeric cells,
- or divide one frame by another after structural matching.

## Parameters

**other**

Right-hand operand.

- `double`: broadcast one scalar divisor across every participating numeric feature.
- `const DataFrame&`: divide one frame by another after feature/case matching.

The scalar route is the direct “divide every selected value by this constant” operation. The frame route is the aligned elementwise operation when both sides carry matching business features such as `math`, `science`, or `units`.

## Returns

`DataFrame`
    New frame containing the elementwise division result for every participating numeric feature.

## See Also

- <a href="truediv.html"><code>DataFrame::truediv</code></a>: same arithmetic intent spelled with operator-style naming.
- <a href="conform.html"><code>DataFrame::conform</code></a>: align structure first when the other frame is shaped differently.

## Notes

Scalar division broadcasts one value across every participating numeric cell.

Frame-to-frame division follows the normal PlussMic frame matching rules for case labels and feature names.

In mixed-schema frames, select the numeric features you intend to divide before calling `div(...)`. That keeps text and identifier columns out of the arithmetic path.

## Examples

Example 1. Broadcast one scalar divisor across selected numeric features.

```cpp
using namespace pluss::table;

auto labeled = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).caselbl(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.features({"units", "price"}).div(2.0);
```

### Output

```text
===================
     | units  price
-------------------
A100 | 5      1.5
B200 | 3.5    2
B200 | 6      1.25
D400 | 2.5    2.5
-------------------
[4 cs x 2 feat]
```

Example 2. Divide one frame by another feature-by-feature after case and feature matching.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"math", pluss::make_series({80.0, 90.0, 70.0})},
    {"science", pluss::make_series({75.0, 88.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto right = DataFrame({
    {"math", pluss::make_series({10.0, 10.0, 10.0})},
    {"science", pluss::make_series({5.0, 8.0, 4.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = left.div(right);
```

### Output

```text
====================
     | math  science
--------------------
s1   | 8     15
s2   | 9     11
s3   | 7     23
--------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="describe.html">
    <span class="label">Previous</span>
    <span class="title">describe</span>
  </a>
  <a class="next-link" href="drop.html">
    <span class="label">Next</span>
    <span class="title">drop</span>
  </a>
</div>
