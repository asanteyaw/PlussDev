<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 4f66a4155b49 -->

# DataFrame::label

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::label() const -> DataFrameLabelAccessor
```

## What It Does

Enters a dedicated accessor for everything to do with the **case-label axis** — the
per-row index every frame carries alongside its features. `label()` itself does
nothing but bind the accessor to the current frame; the accessor exposes around
thirty methods covering inspection (`.values()`, `.names()`, `.part()`), promoting a
feature into the label axis and back (`.set()`, `.reset()`), reordering
(`.sort()`, `.align()`), set-style operations between two label threads (`.unite()`,
`.intersect()`, `.diff()`, `.symdiff()`), positional lookup (`.find()`, `.pos()`), and
distinct/duplicate inspection (`.distinct()`, `.duplicated()`, `.repeated()`,
`.freq()`) — the label-axis counterparts of the same-named `DataFrame` methods, but
operating on row labels instead of feature values.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)
and use its left sidebar to jump straight to any method (`values`, `set`, `reset`,
`sort`, `unite`, `distinct`, and more).

## Parameters

`label()` itself takes no parameters. See each accessor method's own signature for
what it accepts.

## Returns

`DataFrameLabelAccessor`
    A lightweight accessor bound to the current frame. Methods that inspect labels
    return a `Thread`, `bool`, or `std::vector<std::string>`; methods that transform
    the label axis or the frame return a new `DataFrame`. The source frame is never
    mutated.

## Examples

Example 1. Read the current case-label axis as a plain `Thread`.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"id",    Thread(pluss::make_series({1, 2, 3}))},
    {"units", Thread(pluss::make_series({10.0, 7.0, 12.0}))},
}, Thread(pluss::make_note({"c1", "c2", "c3"})));
std::cout << sales;
```

### Output

```text
==================
     | id    units
------------------
c1   | 1     10   
c2   | 2     7    
c3   | 3     12   
------------------
[3 cs x 2 feat]
```

```cpp
auto labels = sales.label().values();
std::cout << labels;
```

### Output

```text
0  c1
1  c2
2  c3
ptype: Note
```

`.values()` is the direct read of the case-label axis — the same thread `caselbl()`
returns.

Example 2. Promote a feature into the case-label axis, dropping it from the columns.

```cpp
auto customers = DataFrame({
    {"id",   Thread(pluss::make_series({1, 2, 3}))},
    {"city", Thread(pluss::make_note({"Tema", "Accra", "Kumasi"}))},
}, Thread(pluss::make_note({"c1", "c2", "c3"})));

auto by_city = customers.label().set("city", /*drop_feature=*/true);
std::cout << by_city;
```

### Output

```text
=============
       | id  
-------------
Tema   | 1   
Accra  | 2   
Kumasi | 3   
-------------
[3 cs x 1 feat]
```

`city`'s original values (`Tema`, `Accra`, `Kumasi`) become the new case labels,
replacing `customers`'s original `c1, c2, c3` axis entirely, and `city` itself is
dropped from the feature columns since `drop_feature = true`. Pass `false` to keep
`city` as an ordinary feature alongside the new label axis.

Example 3. Sort the frame by its case labels.

```cpp
auto sorted_by_city = by_city.label().sort();
std::cout << sorted_by_city;
```

### Output

```text
=============
       | id  
-------------
Accra  | 2   
Kumasi | 3   
Tema   | 1   
-------------
[3 cs x 1 feat]
```

Rows are reordered into ascending alphabetical order by case label (`Accra`,
`Kumasi`, `Tema`), carrying each row's `id` value along with it.

## Notes

- `label()` is the dedicated surface for row-label operations, kept separate from
  `features()` and the feature-axis surface (`feataxis()`), which operate on column
  names instead.
- `.set(feature, true)` promotes a feature into the label axis and drops it from the
  columns; `.set(feature, false)` (the default) keeps the source feature as an
  ordinary column alongside the new label axis.
- `.reset(name)` is the inverse of `.set()`: it materializes the current label axis
  back into an ordinary feature under the given name.

## See Also

- [`DataFrame::feataxis`](feataxis.md): the equivalent dedicated accessor for feature
  *names*, rather than case labels.
- [`DataFrame::case_study`](case_study.md): select rows by label or position, once you
  know what the label axis looks like.
- [`DataFrame::sort_caselbl`](sort_caselbl.md): the root-level equivalent of
  `label().sort()`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kurtosis.html">
    <span class="label">Previous</span>
    <span class="title">kurtosis</span>
  </a>
  <a class="next-link" href="land.html">
    <span class="label">Next</span>
    <span class="title">land</span>
  </a>
</div>
