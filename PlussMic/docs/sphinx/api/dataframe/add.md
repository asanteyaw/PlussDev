<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 838fbd248ff2 -->

# DataFrame::add

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::add(double other) const -> DataFrame
DataFrame::add(const DataFrame& other) const -> DataFrame
DataFrame::add(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::add(const Slate& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::add(const Slate& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::add(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise addition, for every numeric feature on the frame.
`add()` is one of eight identically-shaped arithmetic methods
(`add`, [`sub`](sub.md), [`mul`](mul.md), [`div`](div.md), [`truediv`](truediv.md),
[`floordiv`](floordiv.md), [`mod`](mod.md), [`pow`](pow.md)) that all share the exact
same overload set and alignment rules — only the arithmetic operation itself differs.
Reach for the named method over the `+` operator when you want to chain it inline in a
pipeline, or when the broadcast direction needs to be explicit rather than implied by
operator overloading.

## Parameters

**other**

The right-hand operand, in one of three shapes:

- `double`: one scalar, broadcast against every numeric feature.
- `const DataFrame&`: another frame, added feature-by-feature and case-by-case after
  the two frames are aligned.
- `const Slate&`: one column of values, broadcast according to `axis` (see below).

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

Controls how a `Slate` right-hand operand is broadcast:

- `Axis::Feature` (the default): `other` supplies **one value per feature** — its
  length must match `nfeatures()`. Feature `i`'s value is added to every case in
  column `i`. This is the natural shape for "add a different constant to each column."
- `Axis::Case`: `other` supplies **one value per case** — its length must match
  `ncases()`. Case `i`'s value is added across every feature in row `i`. This is the
  natural shape for "add a per-row adjustment across every column."

**options** (`AlignOptions`, optional)

Controls how the `DataFrame` or `Slate` right-hand operand is aligned to the current
frame before the arithmetic runs (by position, case label, or feature name), and how
missing values are filled during alignment. Defaults to strict positional alignment.

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature increased by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Broadcast one scalar across every numeric feature.

```cpp
using namespace pluss::table;

auto prices = DataFrame({
    {"units", pluss::make_series({2.0, 5.0, 1.0})},
    {"price", pluss::make_series({10.0, 20.0, 30.0})},
}, pluss::make_note({"a", "b", "c"}));

std::cout << prices;
```

### Output (before)

```text
===================
     | units  price
-------------------
a    | 2      10   
b    | 5      20   
c    | 1      30   
-------------------
[3 cs x 2 feat]
```

```cpp
auto out = prices.add(10.0);
std::cout << out;
```

### Output (after)

```text
===================
     | units  price
-------------------
a    | 12     20   
b    | 15     30   
c    | 11     40   
-------------------
[3 cs x 2 feat]
```

Example 2. Add a `Slate` with one value per feature (`Axis::Feature`, the default) —
`units` gets `+100`, `price` gets `+1`.

```cpp
auto per_feature_bump = Slate(pluss::make_series({100.0, 1.0}));
auto out = prices.add(per_feature_bump);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 102    11   
b    | 105    21   
c    | 101    31   
-------------------
[3 cs x 2 feat]
```

Example 3. Add a `Slate` with one value per case (`Axis::Case`) instead — row `a` gets
`+1`, row `b` gets `+2`, row `c` gets `+3`, applied across every feature in that row.

```cpp
auto per_case_bump = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out = prices.add(per_case_bump, Axis::Case);
std::cout << out;
```

### Output

```text
===================
     | units  price
-------------------
a    | 3      11   
b    | 7      22   
c    | 4      33   
-------------------
[3 cs x 2 feat]
```

Example 4. Add another `DataFrame` using the default (`const DataFrame&)` overload
— a plain positional pairing that ignores both frames' case labels and feature
names.

```cpp
auto bumps = DataFrame({
    {"col1", Slate(pluss::make_series({300.0, 100.0, 200.0}))},
    {"col2", Slate(pluss::make_series({3000.0, 1000.0, 2000.0}))},
}, Slate(pluss::make_note({"c", "a", "b"})));
std::cout << bumps;

auto out = prices.add(bumps);
std::cout << out;
```

### Output (bumps)

```text
=================
     | col1  col2
-----------------
c    | 300   3000
a    | 100   1000
b    | 200   2000
-----------------
[3 cs x 2 feat]
```

### Output (prices.add(bumps))

```text
===================
     | units  price
-------------------
a    | 302    3010 
b    | 105    1020 
c    | 201    2030 
-------------------
[3 cs x 2 feat]
```

`bumps`'s case labels are `"c"`, `"a"`, `"b"` — a different order than
`prices`'s `"a"`, `"b"`, `"c"` — and its own feature names (`col1`, `col2`)
don't match `prices`'s (`units`, `price`) at all. Neither matters for the
default overload: it only requires identical shape (`3` cases, `2` features
on both sides), then pairs cells purely by physical position, keeping
`prices`'s own case labels and feature names in the result. The result is
almost certainly *not* what you want when the two frames' rows represent the
same entities in a different order — see Example 5 for the label-aware
alternative.

Example 5. The same two frames, using `AlignOptions{.align =
AlignBy::Caselbl}` to pair rows by case label instead of physical position.

```cpp
auto out_by_label = prices.add(bumps, AlignOptions{.align = AlignBy::Caselbl});
std::cout << out_by_label;
```

### Output

```text
===================
     | units  price
-------------------
a    | 102    1010 
b    | 205    2020 
c    | 301    3030 
-------------------
[3 cs x 2 feat]
```

With `AlignBy::Caselbl`, `bumps`'s rows are matched to `prices`'s by their
case label first — `"a"` pairs with `"a"` (`2 + 100 = 102`, `10 + 1000 =
1010`) regardless of which physical row it occupies in `bumps` — rather than
Example 4's row-0-with-row-0 positional pairing, which silently paired
mismatched entities (`"a"` with `"c"`'s values).

## Notes

- Non-numeric features (`Note`, `Timeline`, `TimeIndex`) are always left unchanged,
  whether the right-hand operand is a scalar, a `Slate`, or another `DataFrame`.
- `Axis::Feature` (one value per column) is the default for the `Slate` overload;
  `Axis::Case` (one value per row) must be requested explicitly.
- For the reflected form (useful mainly for readability when the frame is the
  right-hand conceptual operand), see [`radd()`](radd.md) — though for addition
  specifically, `a.add(b)` and `b.radd(a)`-style reflection produce the same result
  either way, since addition is commutative.

## See Also

- [`DataFrame::sub`](sub.md), [`DataFrame::mul`](mul.md), [`DataFrame::div`](div.md):
  the other arithmetic methods sharing this exact overload shape.
- [`DataFrame::match`](match.md): inspect or control how two frames are aligned before
  paired operations.
- [`DataFrame::assign`](assign.md): compute and attach a derived feature instead of
  transforming existing ones in place.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="abs.html">
    <span class="label">Previous</span>
    <span class="title">abs</span>
  </a>
  <a class="next-link" href="aggregate.html">
    <span class="label">Next</span>
    <span class="title">aggregate</span>
  </a>
</div>
