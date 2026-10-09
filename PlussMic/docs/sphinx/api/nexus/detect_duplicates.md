<!-- pluss-hand-authored -->

# Nexus::detect_duplicates

[Back to Nexus API Reference](../../nexus_api_reference)

## Signatures

```cpp
Nexus::detect_duplicates(std::string_view table) const -> DataFrame

Nexus::detect_duplicates(std::string_view table, std::initializer_list<std::string_view> keys) const -> DataFrame
Nexus::detect_duplicates(std::string_view table, pluss::msharp::RangeView::Of<std::string_view> keys) const -> DataFrame
template <detail::StringViewNameRange NameRange>
Nexus::detect_duplicates(std::string_view table, const NameRange& keys) const -> DataFrame
```

## What It Does

Returns the actual rows that share a duplicate key combination for a table — the same
underlying check as [`validate_keys()`](validate_keys.md), but returning the offending
rows instead of a `bool`. Rows with a missing (null) value in any key feature are excluded
first (they're a completeness problem, not a uniqueness one — see
[`grain_audit()`](grain_audit.md) if you need both counted together), and every row that
remains involved in a duplicate key combination is returned — not just the second-and-later
occurrences, but all of them, so you can see the full group.

Same two-overload split as [`validate_keys()`](validate_keys.md): the one-argument
overload reads the table's declared [`TableMeta::primary_key`](table_meta.md); the
two-argument overload takes an explicit key list.

## Parameters

**table** (`std::string_view`)
    The table to check. Must already exist in the workspace.

**keys** (a range of `std::string_view` — a brace literal, a named container
such as `std::vector<std::string_view>`/`std::vector<std::string>`, or any
other range whose elements convert to `std::string_view`; two-argument
overload only)
    The candidate key features. Same validation as
    [`validate_keys()`](validate_keys.md)'s two-argument overload.

```{note} `pluss::msharp::RangeView` is never spelled out in these examples
See [`Nexus::validate_keys`](validate_keys.md)'s own note on this — the
same three-overload pattern (brace literal / named `RangeView`-convertible
container / any other qualifying range) applies here identically.
```

## Returns

`DataFrame`
    A frame containing every row (with the same features as the source table) that shares
    its key combination with at least one other row. Empty (but defined) if there are no
    duplicates.

## Examples

Example 1. Using the declared `primary_key`.

```cpp
using namespace pluss::table;
using namespace pluss::bundle;

auto customers = DataFrame({
    {"customer_id", Slate(pluss::make_series({1.0, 2.0, 3.0, 3.0}))},
    {"name",        Slate(pluss::make_note({"Ada", "Ben", "Cy", "Cy2"}))},
});

auto biz = Nexus({{"customers", customers}});

TableMeta meta;
meta.primary_key = {"customer_id"};
auto tagged = biz.set_table_meta("customers", meta);

auto dups = tagged.detect_duplicates("customers");

std::cout << dups.ncases() << "\n";
std::cout << dups.features("name");
```

### Output

```text
2
2  Cy
3  Cy2
Name: name, ptype: Note
```

Both rows sharing `customer_id = 3.0` are returned, at their original row positions (`2`
and `3`) — `detect_duplicates()` doesn't renumber or reorder.

Example 2. The two-argument overload with an explicit key list.

```cpp
auto dups2 = biz.detect_duplicates("customers", {"customer_id"});
std::cout << dups2.ncases();
```

### Output

```text
2
```

Example 3. The same explicit-key check via named containers — a
`std::vector<std::string_view>` (the `RangeView` overload) and a
`std::vector<std::string>` (the templated overload).

```cpp
std::vector<std::string_view> as_view = {"customer_id"};
std::vector<std::string> as_owned = {"customer_id"};

std::cout << biz.detect_duplicates("customers", as_view).ncases() << " "
          << biz.detect_duplicates("customers", as_owned).ncases();
```

### Output

```text
2 2
```

## Notes

- Rows with a missing key value are excluded before duplicate-checking — they're not
  reported here regardless of whether they'd otherwise collide.
- Returns every row in each duplicate group, not just the extras beyond the first.
- Original row positions are preserved in the returned frame.
- [`duplicates()`](duplicates.md) is an exact alias for both overloads.

## See Also

- [`Nexus::validate_keys`](validate_keys.md): the same underlying check, collapsed to a
  `bool`.
- [`Nexus::duplicates`](duplicates.md): an exact alias.
- [`Nexus::grain_audit`](grain_audit.md): counts of missing and duplicate rows together,
  across every declared key set on a table.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="detect_orphans.html">
    <span class="label">Next</span>
    <span class="title">detect_orphans</span>
  </a>
</div>
