<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d93f545dd037 -->

# AtlasLabelAccessor::pos

[Back to AtlasLabelAccessor API Reference](../../atlas_label_accessor_api_reference)

## Signatures

```cpp
AtlasLabelAccessor::pos(double value) const -> Thread
AtlasLabelAccessor::pos(std::string_view value) const -> Thread
AtlasLabelAccessor::pos(const char* value) const -> Thread
```

## What It Does

Looks up the position of a single `value` within the atlas's current depth labels,
returning it wrapped in a one-element `Thread` — the single-value counterpart to
[`find()`](find.md), which looks up several values at once. The `double` overload is for
numeric depth labels; the `std::string_view`/`const char*` overloads are for text depth
labels (`const char*` is a thin convenience wrapper that forwards to the
`std::string_view` overload).

## Parameters

**value** (`double`, `std::string_view`, or `const char*`)
    The single depth-label value to look up. Use the overload matching the atlas's
    depth-label kind.

## Returns

`Thread`
    A one-element `Thread` named `"pos"` holding `value`'s position, or `-1` if `value`
    isn't one of the atlas's depth labels (same no-throw behavior as
    [`find()`](find.md)).

## Examples

Example 1. Look up a text depth label's position with the `std::string_view` overload.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Thread(pluss::make_note({"jan", "feb", "mar"})));

std::cout << atlas.label().pos("mar");
```

### Output

```text
0  2
Name: pos, ptype: Series
```

Example 2. The `const char*` overload behaves identically — it forwards to the
`std::string_view` overload.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Thread(pluss::make_note({"jan", "feb", "mar"})));

const char* label = "jan";
std::cout << atlas.label().pos(label);
```

### Output

```text
0  0
Name: pos, ptype: Series
```

Example 3. Look up a numeric depth label's position with the `double` overload.

```cpp
auto q1 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0}))},
});
auto q2 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 90.0}))},
});
auto q3 = pluss::table::DataFrame({
    {"sales", pluss::table::Thread(pluss::make_series({95.0, 130.0}))},
});
auto atlas = pluss::table::Atlas({q1, q2, q3}, pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0})));

std::cout << atlas.label().pos(20.0);
```

### Output

```text
0  1
Name: pos, ptype: Series
```

## Notes

- Same no-throw-on-miss behavior as [`find()`](find.md): an unmatched `value` yields
  `-1` in the result rather than raising.
- The result is always a one-element `Thread`, not a bare integer — chain `.at(0)` (or
  similar scalar access) if you need the raw position value.

## See Also

- [`AtlasLabelAccessor::find`](find.md): the multi-value form of this same lookup.
- [`AtlasLabelAccessor::align`](align.md): reorder/subset the bound atlas's frames by
  label, rather than just looking up a position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="intersect.html">
    <span class="label">Previous</span>
    <span class="title">intersect</span>
  </a>
  <a class="next-link" href="range.html">
    <span class="label">Next</span>
    <span class="title">range</span>
  </a>
</div>
