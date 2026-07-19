<!-- pluss-hand-authored -->

# DataFrame::freq

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::freq(pluss::RangeView<std::string_view> features = {}, std::int64_t top_n = 0, bool dropnull = true) const -> Thread
```

## What It Does

Counts how often each distinct value (or, with several features, each distinct
*combination* of values) occurs, and returns the counts as a `Thread` labeled by the
value itself — sorted most-frequent first, ties broken alphabetically by the value's
own text. With exactly one feature named, this delegates straight to that feature's
own [`Thread::freq()`](tally.md). With zero or several features, it builds a
`"|"`-joined composite key per row from the selected features' string representations
and counts occurrences of that composite key instead.

This is a values-first counting tool: it answers "what are the most common cities in
this column" or "what are the most common (city, product) combinations," rather than
"how many rows/features does this frame have" (see [`ndistinct()`](ndistinct.md) for a
plain distinct-count instead of a per-value breakdown).

## Parameters

**features** (`pluss::RangeView<std::string_view>`, default `{}`)

Which feature(s) define the value being counted:

- Empty (the default): every feature on the frame participates in the composite key.
- One name: delegates to that feature's own frequency count.
- Several names: counts distinct combinations across exactly those features, joined
  with `"|"` into one composite key per row.

**top_n** (`std::int64_t`, default `0`)

Keep only the `top_n` most frequent entries after sorting. `0` (the default) keeps
every distinct value or combination observed.

**dropnull** (`bool`, default `true`)

- `true` (the default): a row where any selected feature is missing is excluded from
  the count entirely.
- `false`: a missing selected feature contributes the literal placeholder `"NULL"` to
  that row's key instead of being dropped — only the missing feature's own part of the
  key is affected; every other, still-present feature in the same row keeps its real
  value.

## Returns

`Thread`
    One `Int64` entry per distinct value (or combination), labeled by that value's (or
    joined combination's) string form, named `"freq"`, sorted by count descending and
    then alphabetically by label ascending on ties.

## Examples

Example 1. Count the most frequent values in a single feature.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"city", Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Accra", "Tema"}))},
});
std::cout << orders;
```

### Output

```text
=============
     | city  
-------------
0    | Accra 
1    | Tema  
2    | Accra 
3    | Kumasi
4    | Accra 
5    | Tema  
-------------
[6 cs x 1 feat]
```

```cpp
auto counts = orders.freq();
std::cout << counts;
```

### Output

```text
 Accra  3
  Tema  2
Kumasi  1
Name: freq, ptype: Series
```

`Accra` is the most frequent city (`3` occurrences), `Tema` next (`2`), `Kumasi` least
(`1`) — sorted strictly by count, with no ties to break here.

Example 2. Count distinct combinations across two features at once.

```cpp
auto sales = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema", "Accra"}))},
    {"product", Thread(pluss::make_note({"Laptop", "Laptop", "Phone", "Phone", "Phone"}))},
});

auto combo_counts = sales.freq({"city", "product"});
std::cout << combo_counts;
```

### Output

```text
Accra|Laptop  2
  Tema|Phone  2
 Accra|Phone  1
Name: freq, ptype: Series
```

`(Accra, Laptop)` and `(Tema, Phone)` both occur twice; with the count tied at `2`, the
alphabetically earlier composite key (`"Accra|Laptop"` before `"Tema|Phone"`) sorts
first. `(Accra, Phone)`, occurring once, sorts last.

Example 3. Keep missing values in the count instead of dropping their rows.

```cpp
auto with_gap = DataFrame({
    {"city",    Thread(pluss::make_note({"Accra", "Accra", "Tema"}))},
    {"product", Thread(pluss::make_note({"Laptop", "", "Phone"}),
                       pluss::make_series({true, false, true}, pluss::ScalarType::Bool))},
    {"region",  Thread(pluss::make_note({"South", "South", "North"}))},
});
std::cout << with_gap;
```

### Output

```text
=============================
     | city   product  region
-----------------------------
0    | Accra  Laptop   South 
1    | Accra  NULL     South 
2    | Tema   Phone    North 
-----------------------------
[3 cs x 3 feat]
```

```cpp
auto gap_counts = with_gap.freq({"city", "product", "region"}, 0, /*dropnull=*/false);
std::cout << gap_counts;
```

### Output

```text
Accra|Laptop|South  1
  Accra|NULL|South  1
  Tema|Phone|North  1
Name: freq, ptype: Series
```

Row `1`'s `product` is missing, so its key contribution becomes `"NULL"` — but `region`
at row `1`, a perfectly present value (`"South"`), keeps reading as `"South"` in the
composite key rather than also being swallowed into `"NULL"`. Each selected feature's
contribution to the composite key is decided independently, so one missing feature
never affects any other feature's part of the same row's key.

## Notes

- With one feature named, `freq()` is exactly that feature's own frequency count (see
  [`Thread::freq()`](tally.md)); the multi-feature composite-key behavior described
  above only applies when zero or several feature names are given.
- Sorting is always most-frequent first; ties are broken by ascending alphabetical
  order of the value (or composite key) itself, never by first-appearance order.
- `top_n = 0` (the default) keeps every distinct value or combination; a positive
  `top_n` truncates the result *after* sorting, so it always keeps the most frequent
  entries, never an arbitrary sample.
- Every missing-value placeholder throughout this reference — here, in
  [`duplicated()`](duplicated.md), and elsewhere — reads as the literal text `"NULL"`,
  matching the `NULL` convention used for missing cells in a rendered frame.

## See Also

- [`DataFrame::ndistinct`](ndistinct.md): a plain distinct-value *count*, without the
  per-value frequency breakdown.
- [`DataFrame::duplicated`](duplicated.md): flag individual duplicate *rows* instead of
  counting distinct values.
- [`DataFrame::isnull`](isnull.md): inspect missingness directly, useful before
  deciding whether `dropnull = false`'s composite-key behavior matters for your data.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fold.html">
    <span class="label">Previous</span>
    <span class="title">fold</span>
  </a>
  <a class="next-link" href="ge.html">
    <span class="label">Next</span>
    <span class="title">ge</span>
  </a>
</div>
