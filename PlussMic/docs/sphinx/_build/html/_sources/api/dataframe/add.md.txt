# DataFrame::add

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::add(const DataFrame& other) -> DataFrame

DataFrame::add(double other) -> DataFrame
```

Add either a scalar or another DataFrame to the current DataFrame.

`DataFrame::add(...)` is the explicit arithmetic method form behind frame addition. Use it when you want a named verb instead of the operator surface, when you are chaining methods, or when you want the documentation to make the broadcast/alignment behavior completely explicit.

## Parameters

**other**

Right-hand operand to add.

- In the scalar overload, pass one numeric value such as `1.0` to broadcast across compatible numeric features.
- In the frame overload, pass another `DataFrame` whose features and cases can be aligned for pairwise addition.

Choose the frame overload when you are applying per-cell adjustments from another table. Choose the scalar overload when you are shifting values by one shared amount.

## Returns

`DataFrame`
    A new frame with the addition result.

    - Scalar addition updates compatible numeric features while preserving the original shape.
    - Frame-to-frame addition aligns the two inputs first and then adds matched numeric features pairwise.

## See Also

- <a href="sub.html"><code>DataFrame::sub</code></a>: subtract a scalar or another frame using the same alignment model.
- <a href="mul.html"><code>DataFrame::mul</code></a>: multiply numeric features without leaving the method-chaining surface.
- <a href="match.html"><code>DataFrame::match</code></a>: inspect or control how two frames are aligned before paired operations.

## Notes

Scalar addition is broadcast over compatible numeric features. Non-numeric features are preserved in the output instead of being dropped.

When you add another `DataFrame`, PlussMic first aligns the two frames on their shared structure and then performs pairwise arithmetic on the matched columns.

The result is non-mutating. The current frame is left unchanged and a new frame is returned.

## Examples

Example 1. Broadcast one scalar across the numeric features of a frame.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5})},
}).title("sales");

auto out = sales.add(1.0);
```

### Output

```text
===========================
     | city    units  price
---------------------------
0    | Accra   11     4
1    | Tema    8      5
2    | Kumasi  13     3.5
---------------------------
[3 cs x 3 feat]
```

Example 2. Add one numeric frame to another frame feature by feature.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"midterm_1", pluss::make_series({15.0, 18.0, 12.0})},
    {"midterm_2", pluss::make_series({16.0, 14.0, 13.0})},
}).title("left");

auto curve = DataFrame({
    {"midterm_1", pluss::make_series({1.0, 1.0, 1.0})},
    {"midterm_2", pluss::make_series({0.5, 0.5, 0.5})},
}).title("curve");

auto out = left.add(curve);
```

### Output

```text
===========================
     | midterm_1  midterm_2
---------------------------
0    | 16         16.5
1    | 19         14.5
2    | 13         13.5
---------------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="next-link" href="add_prefix.html">
    <span class="label">Next</span>
    <span class="title">add_prefix</span>
  </a>
</div>
