<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab0dee9a35dc -->

# DataFrame::drop

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::drop(pluss::RangeView<std::string_view> names) const

template <detail::StringViewNameRange NameRange>
DataFrame DataFrame::drop(const NameRange& names) const
```

## What It Does

Removes one or more features by name and keeps the remaining frame — same cases, same
case labels, same values — otherwise unchanged. `drop()` is a pure schema operation: it
never removes rows, and it never touches the values of any feature that survives.

Conceptually, `drop()` is the complement of [`features()`](features.md): where
`features({...})` names the columns you want to *keep*, `drop({...})` names the columns
you want to *remove*, with every unmentioned feature carried forward automatically. Reach
for whichever reads more naturally at the call site — dropping two columns from a
twenty-column frame is much shorter to write as a `drop()` than as a `features()` list of
the other eighteen.

## Parameters

**names** (`pluss::RangeView<std::string_view>`)

Feature names to remove. Pass one or more names in any order — order in the argument
list has no effect on the result. Every remaining feature keeps its original
left-to-right position in the schema; `drop()` does not reorder or re-sort what's left.
Any container of `std::string_view`-convertible names (`std::vector<std::string>`,
`std::set<std::string>`, and similar) works too, via a generic template overload — not
just a braced list or an explicit `RangeView`.

## Returns

`DataFrame`
    A new frame containing every unselected feature, in its original row alignment and
    original left-to-right feature order. The case axis (row labels) is preserved exactly
    as it was on the source frame.

## Examples

Example 1. Drop one feature and preserve the case axis.

```cpp
using namespace pluss::table;

auto labeled = DataFrame({
    {"city",   pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units",  pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price",  pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).label().set(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.drop({"region"});
std::cout << out;
```

### Output

```text
===========================
     | city    units  price
---------------------------
A100 | Accra   10     3
B200 | Tema    7      4
B200 | Kumasi  12     2.5
D400 | Tamale  5      5
---------------------------
[4 cs x 3 feat]
```

`region` is gone entirely — not blanked, not renamed, simply absent from the schema —
while `city`, `units`, and `price` keep both their values and their original left-to-right
order. Notice too that the case labels (`A100`, `B200`, `B200`, `D400`, including the
repeated `B200`) are untouched: `drop()` operates purely on the feature axis, so anything
about how rows are labeled or ordered is completely out of scope for this method.

## Notes

- `drop()` works on feature names, never on case labels — there is no row-dropping
  overload hiding behind this method. To remove rows, see
  [`dropnull()`](dropnull.md) (rule-based, missing-value driven) or
  [`filter()`](filter.md) (condition-based).
- The current case axis is preserved exactly; only the feature set changes.
- The surviving features keep their original left-to-right order — `drop()` does not
  re-sort the schema based on the order names were passed in `names`.
- Dropping a name that doesn't exist on the frame is a programming error, not a silent
  no-op; confirm the name is spelled exactly as it appears in
  [`features()`](features.md) if a drop unexpectedly leaves a column in place.

## See Also

- [`DataFrame::features`](features.md): the complementary operation — name what to
  *keep* rather than what to remove.
- [`DataFrame::dropnull`](dropnull.md): remove rows based on missingness, rather than
  removing whole columns.
- [`DataFrame::rename`](rename.md): change a feature's name instead of removing it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="div.html">
    <span class="label">Previous</span>
    <span class="title">div</span>
  </a>
  <a class="next-link" href="drop_duplicates.html">
    <span class="label">Next</span>
    <span class="title">drop_duplicates</span>
  </a>
</div>
