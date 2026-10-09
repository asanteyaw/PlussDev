<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bb86a6bf5cc3 -->

# DataFrame::floordiv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::floordiv(double other) const -> DataFrame
DataFrame::floordiv(const DataFrame& other) const -> DataFrame
DataFrame::floordiv(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::floordiv(const Slate& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::floordiv(const Slate& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::floordiv(const Slate& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of floor division: divide, then round down to the nearest whole
number. `floordiv()` shares the exact same six-overload shape and alignment rules as
[`add()`](add.md) and the rest of the arithmetic family, but is the one member of the
family whose result is genuinely different from [`div()`](div.md)/[`truediv()`](truediv.md)
— those two perform ordinary division, while `floordiv()` always floors the quotient
afterward. Every example on this page uses the identical dataset and divisors as
[`mod()`](mod.md)'s own examples, since the two satisfy
`x == divisor * x.floordiv(divisor) + x.mod(divisor)`.

## Parameters

**other**

The right-hand operand (the divisor), in one of three shapes:

- `double`: one scalar divisor, applied to every numeric feature.
- `const DataFrame&`: another frame supplying one divisor per cell, after the two
  frames are aligned.
- `const Slate&`: one column of divisors, broadcast according to `axis`.

**axis** (`Axis`, default `Axis::Feature`, Slate overloads only)

- `Axis::Feature` (the default): `other` supplies **one divisor per feature**.
- `Axis::Case`: `other` supplies **one divisor per case**.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options`. Defaults to strict positional
alignment (`AlignBy::Pos`).

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature floor-divided by
    `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Floor-divide every numeric feature by a scalar.

```cpp
using namespace pluss::table;

auto counts = DataFrame({
    {"total", pluss::make_series({7.0, 9.0, 10.0})},
    {"extra", pluss::make_series({15.0, 22.0, 13.0})},
}, pluss::make_note({"a", "b", "c"}));

std::cout << counts;
```

### Output (before)

```text
====================
      | total  extra
--------------------
a     | 7      15   
b     | 9      22   
c     | 10     13   
--------------------
[3 cs x 2 feat]
```

```cpp
auto out = counts.floordiv(2.0);
std::cout << out;
```

### Output (after)

```text
====================
      | total  extra
--------------------
a     | 3      7    
b     | 4      11   
c     | 5      6    
--------------------
[3 cs x 2 feat]
```

`7 / 2 = 3.5` floors to `3`; `10 / 2 = 5` divides evenly, so `floordiv()` and
plain [`div()`](div.md) would agree on that one cell — compare against
[`mod(2.0)`](mod.md) on the same frame, which gives `0` for that same evenly-divided
cell and non-zero remainders everywhere else.

Example 2. A `Slate` with one divisor per feature (`Axis::Feature`, the default) —
`total` is floor-divided by `2`, `extra` by `3`.

```cpp
auto per_feature_divisor = Slate(pluss::make_series({2.0, 3.0}));
auto out = counts.floordiv(per_feature_divisor);
std::cout << out;
```

### Output

```text
====================
      | total  extra
--------------------
a     | 3      5    
b     | 4      7    
c     | 5      4    
--------------------
[3 cs x 2 feat]
```

Example 3. A `Slate` with one divisor per *case* instead (`Axis::Case`) — row `a`'s
divisor is `2`, row `b`'s is `3`, row `c`'s is `4`.

```cpp
auto per_case_divisor = Slate(pluss::make_series({2.0, 3.0, 4.0}));
auto out = counts.floordiv(per_case_divisor, Axis::Case);
std::cout << out;
```

### Output

```text
====================
      | total  extra
--------------------
a     | 3      7    
b     | 3      7    
c     | 2      3    
--------------------
[3 cs x 2 feat]
```

Example 4. Divide by another `DataFrame` using the default (`const DataFrame&`)
overload — a plain positional pairing that ignores both frames' case labels.

```cpp
auto divisors = DataFrame({
    {"col1", Slate(pluss::make_series({2.0, 3.0, 4.0}))},
    {"col2", Slate(pluss::make_series({4.0, 5.0, 3.0}))},
}, Slate(pluss::make_note({"x", "y", "z"})));
std::cout << divisors;

auto out = counts.floordiv(divisors);
std::cout << out;
```

### Output (divisors)

```text
==================
      | col1  col2
------------------
x     | 2     4   
y     | 3     5   
z     | 4     3   
------------------
[3 cs x 2 feat]
```

### Output (counts.floordiv(divisors))

```text
====================
      | total  extra
--------------------
a     | 3      3    
b     | 3      4    
c     | 2      4    
--------------------
[3 cs x 2 feat]
```

`divisors`'s case labels (`x`, `y`, `z`) share nothing with `counts`'s (`a`, `b`,
`c`); the default overload pairs purely by physical row position, regardless of what
either side's case labels say.

Example 5. The exact same `divisors` values, but with the case labels shuffled to
`c`, `a`, `b` — now using `AlignOptions{.align = AlignBy::Caselbl}` to pair rows by
case label instead of physical position.

```cpp
auto divisors_by_label = DataFrame({
    {"col1", Slate(pluss::make_series({2.0, 3.0, 4.0}))},
    {"col2", Slate(pluss::make_series({4.0, 5.0, 3.0}))},
}, Slate(pluss::make_note({"c", "a", "b"})));
std::cout << divisors_by_label;

auto out = counts.floordiv(divisors_by_label, AlignOptions{.align = AlignBy::Caselbl});
std::cout << out;
```

### Output (divisors_by_label)

```text
==================
      | col1  col2
------------------
c     | 2     4   
a     | 3     5   
b     | 4     3   
------------------
[3 cs x 2 feat]
```

### Output (counts.floordiv(divisors_by_label, Caselbl))

```text
====================
      | total  extra
--------------------
a     | 2      3    
b     | 2      7    
c     | 5      3    
--------------------
[3 cs x 2 feat]
```

With `AlignBy::Caselbl`, case `"a"` in `counts` is matched to case `"a"` in
`divisors_by_label` — its *second* physical row here (`col1 = 3, col2 = 5`) — giving
`total_a // 3 = 7 // 3 = 2` and `extra_a // 5 = 15 // 5 = 3`. Compare this to
Example 4's positional pairing, which matched `counts`'s row 0 (`"a"`) to
`divisors`'s row 0 (`"x"`, `col1 = 2, col2 = 4`) and got a different answer
(`3`, `3`) — same underlying divisor values, different pairing rule, different
result for `total_a` specifically (`extra_a` happens to agree at `3` both ways here).

## Notes

- Non-numeric features are always left unchanged.
- `floordiv()` is the one member of this arithmetic family that differs from
  `div()`/`truediv()` in its actual result, not just its name — it always rounds the
  quotient down to the nearest whole number.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`div()`](div.md),
  [`mod()`](mod.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::div`](div.md) / [`DataFrame::truediv`](truediv.md): ordinary division,
  without flooring.
- [`DataFrame::mod`](mod.md): the remainder left over after floor division, using the
  identical dataset and divisors as this page's examples.
- [`DataFrame::round`](round.md): round to a given decimal precision instead of always
  flooring.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
