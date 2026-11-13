<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d635fe11a708 -->

# DataFrame::csmin

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
Slate DataFrame::csmin(Axis axis = Axis::Case, bool skipnull = true) const
```

## What It Does

Reports *where* each numeric feature's minimum value occurs, not the minimum value
itself — the argmin-style companion to [`min()`](min.md), and the mirror of
[`csmax()`](csmax.md). The returned `Slate` always holds label strings (case labels or
feature names), never numeric values, so it always prints with `ptype: Note`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to search along:

- `Axis::Case` (the default): for each numeric feature, find which *case* holds that
  feature's minimum value. The result has one entry per numeric feature, labeled by
  feature name.
- `Axis::Feature`: for each case, find which *feature* holds that row's minimum value.
  The result has one entry per case, labeled by the source frame's case labels.

**skipnull** (`bool`, default `true`)

Missing-value policy while searching for the minimum.

## Returns

`Slate`
    A `Note`-typed label slate: case labels (`Axis::Case`) or feature names
    (`Axis::Feature`), one per participating numeric feature or case respectively.

## Examples

Example 1. Default axis: for each feature, which case holds its minimum.

```cpp
using namespace pluss::table;

auto regions = DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost",  pluss::make_series({60.0, 80.0, 95.0})},
}, pluss::make_note({"q1", "q2", "q3"}));

std::cout << regions;
```

### Output (before)

```text
==================
     | sales  cost
------------------
q1   | 100    60  
q2   | 120    80  
q3   | 90     95  
------------------
[3 cs x 2 feat]
```

```cpp
auto trough_case = regions.csmin();
std::cout << trough_case;
```

### Output (after)

```text
sales  q3
 cost  q1
ptype: Note
```

`sales` bottoms out at case `q3` (`90`), and `cost` bottoms out at case `q1` (`60`). The
result has one entry per numeric feature, labeled by feature name.

Example 2. `Axis::Feature`: for each case, which feature holds the smaller value.

```cpp
auto trough_feature = regions.csmin(Axis::Feature);
std::cout << trough_feature;
```

### Output

```text
q1  cost
q2  cost
q3  sales
ptype: Note
```

At `q1` and `q2`, `cost` is smaller than `sales`; at `q3`, `sales` (`90`) drops below
`cost` (`95`). The result now has one entry per case, labeled by the source frame's
case labels.

Example 3. `skipnull = false`: a single missing value anywhere in a feature
poisons that feature's whole result to `NULL`, even though the same feature
still finds a perfectly good minimum among its non-null cells under the
default `skipnull = true`.

```cpp
auto regions2 = DataFrame({
    {"sales", pluss::make_series({100.0, std::numeric_limits<double>::quiet_NaN(), 90.0})},
    {"cost",  pluss::make_series({60.0, 80.0, 95.0})},
}, pluss::make_note({"q1", "q2", "q3"}));

auto trough_skip = regions2.csmin(Axis::Case, true);
std::cout << trough_skip;

auto trough_no_skip = regions2.csmin(Axis::Case, false);
std::cout << trough_no_skip;
```

### Output (skipnull = true, the default)

```text
sales  q3
 cost  q1
ptype: Note
```

### Output (skipnull = false)

```text
sales  NULL
 cost  q1
ptype: Note
```

`sales` is missing at `q2`; with `skipnull = true`, the search simply ignores
that cell and finds the minimum (`90`) among what's left, at `q3`. With
`skipnull = false`, the presence of any missing value anywhere in `sales`
invalidates the whole result for that feature — `sales` comes back `NULL`
outright, even though a perfectly good minimum among the non-null values still
exists. `cost` has no missing values at all, so it reports `q1` identically
under both settings. This same call also demonstrates `Axis::Case` passed
explicitly, rather than relying on it being the default.

## Notes

- `csmin()` always returns label strings, never the numeric minimum itself — reach for
  [`min()`](min.md) when the value is what you want, and `csmin()` when you want to
  know where it came from.
- The two axes produce results with different lengths and different label kinds: one
  entry per feature (`Axis::Case`) or one entry per case (`Axis::Feature`) — do not
  assume the result lines up positionally with the source frame's rows.

## See Also

- [`DataFrame::csmax`](csmax.md): the same lookup for the maximum instead of the
  minimum.
- [`DataFrame::min`](min.md): the numeric minimum value itself, instead of where it
  occurs.
- [`DataFrame::cummin`](cummin.md): a running minimum per case, instead of a single
  argmin lookup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="csmax.html">
    <span class="label">Previous</span>
    <span class="title">csmax</span>
  </a>
  <a class="next-link" href="cummax.html">
    <span class="label">Next</span>
    <span class="title">cummax</span>
  </a>
</div>
