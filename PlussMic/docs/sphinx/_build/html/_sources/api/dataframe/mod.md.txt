<!-- pluss-hand-authored -->

# DataFrame::mod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::mod(double other) const -> DataFrame
DataFrame::mod(const DataFrame& other) const -> DataFrame
DataFrame::mod(const DataFrame& other, const AlignOptions& options) const -> DataFrame
DataFrame::mod(const Thread& other, Axis axis = Axis::Feature) const -> DataFrame
DataFrame::mod(const Thread& other, Axis axis, const AlignOptions& options) const -> DataFrame
DataFrame::mod(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) const -> DataFrame
```

## What It Does

The named-method form of elementwise remainder (modulo). `mod()` shares the exact same
six-overload shape and alignment rules as [`add()`](add.md) and the rest of the
arithmetic family — see `add()`'s "What It Does" for the general explanation of the
scalar/frame/`Thread` operand shapes and the `axis`/`options` parameters. `mod()` is
the natural companion to [`floordiv()`](floordiv.md): together they satisfy
`x == divisor * x.floordiv(divisor) + x.mod(divisor)` — every example on this page
uses the identical dataset and divisors as `floordiv()`'s own examples so the two
pages can be compared side by side.

## Parameters

**other**

The right-hand operand (the divisor), in one of three shapes:

- `double`: one scalar divisor, applied to every numeric feature.
- `const DataFrame&`: another frame supplying one divisor per cell, after the two
  frames are aligned.
- `const Thread&`: one column of divisors, broadcast according to `axis`.

**axis** (`Axis`, default `Axis::Feature`, Thread overloads only)

- `Axis::Feature` (the default): `other` supplies **one divisor per feature** — its
  length must match `nfeatures()`.
- `Axis::Case`: `other` supplies **one divisor per case** — its length must match
  `ncases()`.

**options** (`AlignOptions`, optional)

Same meaning as [`add()`](add.md)'s `options` — controls how a `DataFrame`/`Thread`
divisor is aligned before the remainder is computed. Defaults to strict positional
alignment (`AlignBy::Pos`).

## Returns

`DataFrame`
    A new frame, same shape as the source, with every numeric feature replaced by its
    remainder after dividing by `other`. Non-numeric features are left unchanged.

## Examples

Example 1. Take the remainder of every numeric feature against a scalar divisor.

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
auto out = counts.mod(2.0);
std::cout << out;
```

### Output (after)

```text
====================
      | total  extra
--------------------
a     | 1      1    
b     | 1      0    
c     | 0      1    
--------------------
[3 cs x 2 feat]
```

`10 mod 2 = 0` — `total` divides evenly on row `c`, consistent with
[`floordiv(2.0)`](floordiv.md) also landing on a whole quotient (`5`) for that same
row with nothing left over.

Example 2. A `Thread` with one divisor per feature (`Axis::Feature`, the default) —
`total` is taken mod `2`, `extra` mod `3`.

```cpp
auto per_feature_divisor = Thread(pluss::make_series({2.0, 3.0}));
auto out = counts.mod(per_feature_divisor);
std::cout << out;
```

### Output

```text
====================
      | total  extra
--------------------
a     | 1      0    
b     | 1      1    
c     | 0      1    
--------------------
[3 cs x 2 feat]
```

Example 3. A `Thread` with one divisor per *case* instead (`Axis::Case`) — row `a`'s
divisor is `2`, row `b`'s is `3`, row `c`'s is `4`, applied across both features in
that row.

```cpp
auto per_case_divisor = Thread(pluss::make_series({2.0, 3.0, 4.0}));
auto out = counts.mod(per_case_divisor, Axis::Case);
std::cout << out;
```

### Output

```text
====================
      | total  extra
--------------------
a     | 1      1    
b     | 0      1    
c     | 2      1    
--------------------
[3 cs x 2 feat]
```

Example 4. Divide by another `DataFrame` using the default (`const DataFrame&`)
overload — a plain positional pairing that ignores both frames' case labels.

```cpp
auto divisors = DataFrame({
    {"col1", Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"col2", Thread(pluss::make_series({4.0, 5.0, 3.0}))},
}, Thread(pluss::make_note({"x", "y", "z"})));
std::cout << divisors;

auto out = counts.mod(divisors);
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

### Output (counts.mod(divisors))

```text
====================
      | total  extra
--------------------
a     | 1      3    
b     | 0      2    
c     | 2      1    
--------------------
[3 cs x 2 feat]
```

`divisors`'s case labels (`x`, `y`, `z`) share nothing with `counts`'s (`a`, `b`,
`c`); the default overload pairs purely by physical row position (`total`'s row 0,
`7`, is taken mod `col1`'s row 0, `2`, giving `1`), regardless of what either side's
case labels say.

Example 5. The exact same `divisors` values, but with the case labels shuffled to
`c`, `a`, `b` — now using `AlignOptions{.align = AlignBy::Caselbl}` to pair rows by
case label instead of physical position.

```cpp
auto divisors_by_label = DataFrame({
    {"col1", Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"col2", Thread(pluss::make_series({4.0, 5.0, 3.0}))},
}, Thread(pluss::make_note({"c", "a", "b"})));
std::cout << divisors_by_label;

auto out = counts.mod(divisors_by_label, AlignOptions{.align = AlignBy::Caselbl});
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

### Output (counts.mod(divisors_by_label, Caselbl))

```text
====================
      | total  extra
--------------------
a     | 1      0    
b     | 1      1    
c     | 0      1    
--------------------
[3 cs x 2 feat]
```

With `AlignBy::Caselbl`, case `"a"` in `counts` is matched to case `"a"` in
`divisors_by_label` — its *second* physical row here (`col1 = 3, col2 = 5`) — giving
`total_a mod 3 = 7 mod 3 = 1` and `extra_a mod 5 = 15 mod 5 = 0`. Compare this to
Example 4's positional pairing, which matched `counts`'s row 0 (`"a"`) to
`divisors`'s row 0 (`"x"`, `col1 = 2, col2 = 4`) and got a different answer
(`1`, `3`) — same underlying divisor values, different pairing rule, different
result.

## Notes

- Non-numeric features are always left unchanged.
- `mod()` and [`floordiv()`](floordiv.md) are complementary: the floor-divided
  quotient times the divisor, plus the remainder from `mod()`, reconstructs the
  original value.
- Shares its full overload shape, alignment semantics, and `AlignOptions` behavior with
  [`add()`](add.md), [`sub()`](sub.md), [`mul()`](mul.md), [`div()`](div.md),
  [`floordiv()`](floordiv.md), and [`pow()`](pow.md).

## See Also

- [`DataFrame::floordiv`](floordiv.md): the matching floor-division quotient, using
  the identical dataset and divisors as this page's examples.
- [`DataFrame::div`](div.md) / [`DataFrame::truediv`](truediv.md): ordinary division,
  without a remainder.
- [`DataFrame::rmod`](rmod.md): the reflected form, computing `other mod frame`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
