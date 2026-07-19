<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 304eedb4c06c -->

# DataFrame::csmax

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::csmax(Axis axis = Axis::Case, bool skipnull = true) const -> Thread
```

## What It Does

Reports *where* each numeric feature's maximum value occurs, not the maximum value
itself — the argmax-style companion to [`max()`](max.md). The returned `Thread` always
holds label strings (case labels or feature names), never numeric values, so it always
prints with `ptype: Note`.

## Parameters

**axis** (`Axis`, default `Axis::Case`)

Direction to search along:

- `Axis::Case` (the default): for each numeric feature, find which *case* holds that
  feature's maximum value. The result has one entry per numeric feature, labeled by
  feature name.
- `Axis::Feature`: for each case, find which *feature* holds that row's maximum value.
  The result has one entry per case, labeled by the source frame's case labels.

**skipnull** (`bool`, default `true`)

Missing-value policy while searching for the maximum.

## Returns

`Thread`
    A `Note`-typed label thread: case labels (`Axis::Case`) or feature names
    (`Axis::Feature`), one per participating numeric feature or case respectively.

## Examples

Example 1. Default axis: for each feature, which case holds its maximum.

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
auto peak_case = regions.csmax();
std::cout << peak_case;
```

### Output (after)

```text
sales  q2
 cost  q3
ptype: Note
```

`sales` peaks at case `q2` (`120`), and `cost` peaks at case `q3` (`95`). The result has
one entry per numeric feature, labeled by feature name.

Example 2. `Axis::Feature`: for each case, which feature holds the larger value.

```cpp
auto peak_feature = regions.csmax(Axis::Feature);
std::cout << peak_feature;
```

### Output

```text
q1  sales
q2  sales
q3  cost
ptype: Note
```

At `q1` and `q2`, `sales` is larger than `cost`; at `q3`, `cost` (`95`) overtakes
`sales` (`90`). The result now has one entry per case, labeled by the source frame's
case labels.

## Notes

- `csmax()` always returns label strings, never the numeric maximum itself — reach for
  [`max()`](max.md) when the value is what you want, and `csmax()` when you want to
  know where it came from.
- The two axes produce results with different lengths and different label kinds: one
  entry per feature (`Axis::Case`) or one entry per case (`Axis::Feature`) — do not
  assume the result lines up positionally with the source frame's rows.

## See Also

- [`DataFrame::csmin`](csmin.md): the same lookup for the minimum instead of the
  maximum.
- [`DataFrame::max`](max.md): the numeric maximum value itself, instead of where it
  occurs.
- [`DataFrame::cummax`](cummax.md): a running maximum per case, instead of a single
  argmax lookup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="covar.html">
    <span class="label">Previous</span>
    <span class="title">covar</span>
  </a>
  <a class="next-link" href="csmin.html">
    <span class="label">Next</span>
    <span class="title">csmin</span>
  </a>
</div>
