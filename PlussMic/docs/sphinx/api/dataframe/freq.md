<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e1fc19bc864e -->

# DataFrame::freq

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
Slate DataFrame::freq() const
Slate DataFrame::freq(FreqOptions options) const
```

## What It Does

Counts distinct rows of the current frame and returns the result as a `Slate`.
This is the features-first value-count surface:

```cpp
df.features({"default", "is_adult"}).freq();
df.features({"default", "is_adult"}).freq({
    .kind = DataFrame::FreqKind::Prop,
    .dropnull = false,
});
```

The selected frame defines the grouping columns. `freq()` no longer accepts a
feature selector itself.

For multi-feature counts, the result carries structured case labels and prints like a
stacked index instead of packing keys into a pipe-joined string.

## Parameters

**options** (`FreqOptions`, optional)

**options.kind** (`FreqKind`, default `FreqKind::Count`)

Selects the output measure:

- `DataFrame::FreqKind::Count`: raw counts
- `DataFrame::FreqKind::Prop`: counts divided by the total

**options.dropnull** (`bool`, default `true`)

When `true`, any row with a missing grouping value is excluded. When `false`,
missing grouping values participate as `NULL`.

**options.sort** (`bool`, default `true`)

When `true`, rows are ordered by frequency. When `false`, the grouped result stays
in first-seen order.

**options.ascending** (`bool`, default `false`)

Only used when `sort = true`. `false` gives the usual largest-first ordering.

**options.top** (`std::int64_t`, default `0`)

Keeps only the first `top` rows after ordering. `0` keeps every group.

## Returns

`Slate`
    A frequency series named `count` or `prop`.

- With one grouping feature, labels are the distinct values of that feature.
- With several grouping features, labels are structured and render as separate
  index columns.

## Examples

Example 1. Count distinct combinations of two grouping features.

```cpp
using namespace pluss::table;

auto df = DataFrame({
    {"default",  Slate(pluss::make_note({"yes", "yes", "yes", "no", "yes", "no"}))},
    {"is_adult", Slate(pluss::make_series(std::vector<bool>{true, true, true, true, false, false}))},
});

auto counts = df.features({"default", "is_adult"}).freq();
std::cout << counts;
```

### Output

```text
default  is_adult
yes      true        3
no       true        1
yes      false       1
no       false       1
Name: count, ptype: Series
```

Example 2. Use `FreqKind::Prop` to get proportions instead of raw counts.

```cpp
auto prop = df.features({"default", "is_adult"}).freq({
    .kind = DataFrame::FreqKind::Prop,
});
std::cout << prop;
```

### Output

```text
default  is_adult
yes      true        0.5
no       true        0.1666666667
yes      false       0.1666666667
no       false       0.1666666667
Name: prop, ptype: Series
```

Example 3. Keep missing grouping values instead of dropping them, and skip sorting.

```cpp
auto keep_null = DataFrame({
    {"city", Slate(pluss::make_note({"Accra", "Accra", "Tema"}))},
    {"zone", Slate(pluss::make_note({"West", "", "North"}),
                    pluss::make_series(std::vector<bool>{true, false, true}, pluss::ScalarType::Bool))},
});

auto out = keep_null.features({"city", "zone"}).freq({
    .dropnull = false,
    .sort = false,
});
std::cout << out;
```

### Output

```text
city   zone
Accra  West    1
Accra  NULL    1
Tema   North   1
Name: count, ptype: Series
```

## Notes

- `DataFrame::FreqKind` is the frequency measure enum used by `FreqOptions`.
- `DataFrame::FreqOptions` is the public options struct for `freq()`.

## See Also

- [`DataFrame::tally`](tally.md)
- [`Slate::freq`](../slate/freq.md)

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
