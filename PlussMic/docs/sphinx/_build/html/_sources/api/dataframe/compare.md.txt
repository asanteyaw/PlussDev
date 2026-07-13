# DataFrame::compare

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::compare(const DataFrame& other) -> DataFrame

DataFrame::compare(double other) -> DataFrame
```

Compare this frame against a scalar or another frame and return a boolean-style result frame.

`DataFrame::compare(...)` is the frame-level not-equal alias. It is useful when you want a mask-like result without spelling `ne(...)` directly.

## Parameters

**other**

Right-hand comparison target.

This can be:
- a scalar, which broadcasts across the current frame selection, or
- another frame, which is compared cell by cell after alignment.

## Returns

`DataFrame`
    Boolean-style frame whose cells mark where the compared values differ.

## See Also

- <a href="ne.html"><code>DataFrame::ne</code></a>: explicit not-equal comparison.
- <a href="eq.html"><code>DataFrame::eq</code></a>: equality comparison.
- <a href="../thread/compare.html"><code>Thread::compare</code></a>: lower-level typed comparison surface.

## Notes

Scalar comparison broadcasts the scalar across every selected cell in the current frame.

Frame-to-frame comparison requires compatible structure on the participating features and rows.

## Examples

Example 1. Broadcast one scalar across selected numeric features.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).with_cases(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.features({"units", "price"}).compare(5.0);
```

### Output

```text
===================
     | units  price
-------------------
A100 | true   true 
B200 | true   true 
B200 | true   true 
D400 | false  false
-------------------
[4 cs x 2 feat]
```

Example 2. Compare two aligned frames row by row and feature by feature.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"math", pluss::make_series({80.0, 90.0, 70.0})},
    {"science", pluss::make_series({75.0, 88.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto right = DataFrame({
    {"math", pluss::make_series({80.0, 91.0, 70.0})},
    {"science", pluss::make_series({75.0, 80.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = left.compare(right);
```

### Output

```text
=====================
     | math   science
---------------------
s1   | false  false  
s2   | true   true   
s3   | false  false  
---------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="combine.html">
    <span class="label">Previous</span>
    <span class="title">combine</span>
  </a>
  <a class="next-link" href="conform.html">
    <span class="label">Next</span>
    <span class="title">conform</span>
  </a>
</div>
