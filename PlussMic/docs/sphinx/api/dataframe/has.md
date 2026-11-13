<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 27895befc63a -->

# DataFrame::has

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame DataFrame::has(double value) const
DataFrame DataFrame::has(std::string_view value) const
DataFrame DataFrame::has(const Slate& values) const
```

## What It Does

A per-cell membership check, returning a boolean-valued frame the same shape as the
source.

```{important} has(double) and has(string_view) are just eq() by another name
`has(84.0)` and `has("Ada")` are implemented as plain calls to
[`eq(84.0)`](eq.md)/`eq("Ada")` — there is no added value over calling `eq()`
directly for a single value. The real reason to reach for `has()` is the `Slate`
overload below, which `eq()` cannot do.
```

`has(const Slate& values)` is the genuinely distinct overload: it checks each cell
against a whole **set** of candidate values, marking `true` if the cell matches *any*
of them — an "is this value one of these?" check, rather than "does this value equal
one specific thing?".

## Parameters

**value** (`double` or `std::string_view`)

A single value, broadcast against every numeric (for `double`) or `Note`-kind (for
`std::string_view`) feature. Equivalent to calling [`eq()`](eq.md) with the same
argument.

**values** (`const Slate&`)

A set of candidate values to check membership against. Each cell in the frame reports
`true` if it equals *any* valid (non-null) entry in `values`; null entries in
`values` are skipped and never match anything, including a null cell.

## Returns

`DataFrame`
    A new frame, same shape as the source, where every feature holds `true`/`false`
    according to whether that cell matches `value` (or any entry in `values`).

## Examples

Example 1. Check membership against a small set of candidate values.

```cpp
using namespace pluss::table;

auto orders = DataFrame({
    {"status", pluss::make_note({"shipped", "pending", "cancelled", "shipped", "returned"})},
}, pluss::make_note({"o1", "o2", "o3", "o4", "o5"}));

auto closed_states = Slate(pluss::make_note({"shipped", "cancelled"}));
auto out = orders.has(closed_states);
std::cout << out;
```

### Output

```text
=============
     | status
-------------
o1   | true  
o2   | false 
o3   | true  
o4   | true  
o5   | false 
-------------
[5 cs x 1 feat]
```

`o1`, `o3`, and `o4` each match one of the two candidates in `closed_states`
(`"shipped"` or `"cancelled"`) — `has()` doesn't care *which* candidate matched, only
that at least one did. `o2` (`"pending"`) and `o5` (`"returned"`) match neither.

Example 2. `has(84.0)` produces the same result as `eq(84.0)`.

```cpp
auto scores = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_has = scores.has(84.0);
std::cout << out_has;
```

### Output

```text
============
     | quiz 
------------
s1   | true 
s2   | false
s3   | true 
------------
[3 cs x 1 feat]
```

## Notes

- `has(double)`/`has(std::string_view)` are plain aliases for [`eq()`](eq.md) — prefer
  calling `eq()` directly for single-value checks; reach for `has()` specifically for
  the `Slate` (set-membership) overload.
- A null entry in the `values` candidate set never matches anything, including a
  null cell — nulls in `values` are simply skipped.
- A null cell in the source frame produces `NULL` in the result, not `false`, matching
  [`eq()`](eq.md)'s missing-value propagation.

## See Also

- [`DataFrame::eq`](eq.md): the single-value equality this method's scalar overloads
  delegate to.
- [`DataFrame::isnull`](isnull.md) / [`DataFrame::notnull`](notnull.md): membership
  checks against missingness specifically, rather than a value set.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
