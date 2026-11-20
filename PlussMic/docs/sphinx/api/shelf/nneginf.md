<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 08526cb99cbf -->

# Shelf::nneginf

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
std::int64_t Shelf::nneginf() const
```

## What It Does

Returns how many entries of the shelf are `-inf` entries, as an `int64`.

It is the counting counterpart of the matching `is...` question: `x.nneginf()` equals the number of `true` in `x.isneginf()`.
The four counts `nnull`, `nnan`, `nposinf` and `nneginf` never overlap: a cell is exactly one of missing, `NaN`, `+inf`, `-inf` or ordinary.

## Parameters

`None`

## Returns

`std::int64_t`
    The number of entries of that kind.

## Examples

Example 1. Count the `-inf` entries.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

auto n = x.nneginf();
std::cout << "nneginf: " << n << "\n";
```

### Output

```text
nneginf: 1
```

The data has exactly one of each kind, so `nneginf()` is `1`.

Example 2. The four counts never double-count a cell.

```cpp
using namespace pluss::table;

const double nan = std::numeric_limits<double>::quiet_NaN();
const double inf = std::numeric_limits<double>::infinity();

auto values = pluss::make_series({1.0, nan, inf, -inf, 0.0, 5.0});
auto present = pluss::make_series(std::vector<bool>{true, true, true, true, false, true});
auto x = Shelf(values, present);

auto total = x.nnull() + x.nnan() + x.nposinf() + x.nneginf();
std::cout << "not ordinary: " << total << " of " << x.size() << "\n";
std::cout << "ordinary: " << x.size() - total << "\n";
```

### Output

```text
not ordinary: 4 of 6
ordinary: 2
```

`NULL`, `NaN`, `+inf` and `-inf` are four different kinds of cell, so adding the four counts gives the number of cells that are **not** ordinary numbers. Here that is 4 of 6, leaving the 2 ordinary values.

Example 3. A shelf that cannot hold NaN or infinity reports `0` for them.

```cpp
using namespace pluss::table;

auto x = Shelf(pluss::make_series(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}));

auto n = x.nneginf();
std::cout << "nneginf: " << n << "\n";
```

### Output

```text
nneginf: 0
```

Integers can be neither `NaN` nor infinite, and this shelf has no validity mask, so `nneginf()` is `0`.

## Notes

- A shelf that cannot hold `NaN` or an infinity (integers, text, timestamps) reports `0` for `nnan`, `nposinf` and `nneginf`.
- For a table-wide report use [`DataFrame::count_kinds`](../dataframe/count_kinds.md).

## See Also

- [`Shelf::isnull`](isnull.md) / [`isnan`](isnan.md) / [`isinf`](isinf.md): the per-cell questions behind the counts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="nnan.html">
    <span class="label">Previous</span>
    <span class="title">nnan</span>
  </a>
  <a class="next-link" href="nnull.html">
    <span class="label">Next</span>
    <span class="title">nnull</span>
  </a>
</div>
