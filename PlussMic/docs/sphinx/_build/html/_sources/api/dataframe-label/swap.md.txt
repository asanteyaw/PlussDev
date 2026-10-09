<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: cd517d466c79 -->

# DataFrameLabelAccessor::swap

[Back to DataFrameLabelAccessor API Reference](../../dataframe_label_accessor_api_reference)

## Signature

```cpp
DataFrameLabelAccessor::swap(std::string_view left, std::string_view right) const -> DataFrame
```

## What It Does

Swaps the positions of two case-label levels, by name — a convenience for
`order()` with just those two levels' positions exchanged and every other
level left where it was. Requires a structured frame with both `left` and
`right` as existing level names; `left` and `right` must differ.

Like [`order()`](order.md) (which this method is built on), the composite
display string is rebuilt using whichever separator (custom or default) was
used when the levels were originally created.

## Parameters

**left** / **right** (`std::string_view`)

The two level names to exchange. Must both exist, and must differ from each
other.

## Returns

`DataFrame`
    A new frame with the same rows and values, `left` and `right`'s level
    positions swapped.

## Examples

Example 1. Swap a 2-level frame's only two levels.

```cpp
using namespace pluss::table;

auto readings = DataFrame(
    {{"region", Slate(pluss::make_note({"east", "east", "west", "west"}))},
     {"city",   Slate(pluss::make_note({"Boston", "NYC", "LA", "SF"}))},
     {"sales",  Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}))}});
auto grouped = readings.label().set({"region", "city"}, true, "/");
std::cout << grouped;

auto out = grouped.label().swap("region", "city");
std::cout << out;
```

### Output (before)

```text
===================
            | sales
-------------------
east/Boston | 10   
east/NYC    | 20   
west/LA     | 30   
west/SF     | 40   
-------------------
[4 cs x 1 feat]
```

### Output (after)

```text
===================
            | sales
-------------------
Boston/east | 10   
NYC/east    | 20   
LA/west     | 30   
SF/west     | 40   
-------------------
[4 cs x 1 feat]
```

`city` now comes first — and as with [`order()`](order.md), the custom `"/"`
separator from `set()` is preserved in the rebuilt composite label.

Example 2. Swap the outer two levels of a 3-level frame — the untouched
middle level stays exactly where it was.

```cpp
auto sales3 = DataFrame(
    {{"region",  Slate(pluss::make_note({"east", "west"}))},
     {"city",    Slate(pluss::make_note({"Boston", "LA"}))},
     {"quarter", Slate(pluss::make_note({"Q1", "Q2"}))},
     {"sales",   Slate(pluss::make_series({10.0, 20.0}))}});
auto grouped3 = sales3.label().set({"region", "city", "quarter"}, true, "/");
std::cout << grouped3;

auto swapped3 = grouped3.label().swap("region", "quarter");
std::cout << swapped3;
```

### Output (before)

```text
======================
               | sales
----------------------
east/Boston/Q1 | 10   
west/LA/Q2     | 20   
----------------------
[2 cs x 1 feat]
```

### Output (after)

```text
======================
               | sales
----------------------
Q1/Boston/east | 10   
Q2/LA/west     | 20   
----------------------
[2 cs x 1 feat]
```

`swap("region", "quarter")` exchanges only the outer two levels' positions —
`city` (`"Boston"`/`"LA"`) stays in the middle of the composite label both
before and after, exactly as `swap`'s "every other level left where it was"
description promises. This differs from Example 1's 2-level case, where
swapping is necessarily a full reversal since there's no third level to hold
in place.

Example 3. Pass the same name for both `left` and `right`, or an unknown
level name, to see each throw.

```cpp
try {
  auto out = grouped.label().swap("region", "region");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto out = grouped.label().swap("region", "country");
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrame::label().swap: label names must differ
DataFrame::label().swap: unknown label 'country'
```

`swap("region", "region")` names the same level twice, which isn't a
coherent swap, so it's rejected before even checking whether the name
exists. `swap("region", "country")` fails differently — `"region"` is valid,
but `"country"` isn't one of `grouped`'s levels (`"region"`, `"city"`).

## Notes

- With only two levels, `swap(a, b)` and `order({b, a})` produce identical
  results — `swap` is purely a convenience for that common case.
- `left` and `right` must both be existing level names and must differ, or the
  call throws.
- Same separator behavior as [`order()`](order.md): a custom separator is
  preserved.

## See Also

- [`DataFrameLabelAccessor::order`](order.md): the general form for reordering
  any number of levels, which this method delegates to.
- [`DataFrameLabelAccessor::names`](names.md),
  [`DataFrameLabelAccessor::rename`](rename.md): inspect or rename levels
  without changing their order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort.html">
    <span class="label">Previous</span>
    <span class="title">sort</span>
  </a>
  <a class="next-link" href="symdiff.html">
    <span class="label">Next</span>
    <span class="title">symdiff</span>
  </a>
</div>
