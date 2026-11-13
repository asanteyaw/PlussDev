<!-- pluss-hand-authored -->

# SlateLabelAccessor::slice

[Back to SlateLabelAccessor API Reference](../../slate_label_accessor_api_reference)

## Signatures

```cpp
Slate SlateLabelAccessor::slice(std::int64_t start, std::int64_t stop) const
Slate SlateLabelAccessor::slice(std::string_view start, std::string_view stop) const
```

## What It Does

Two different slicing conventions:

- **`slice(int64_t, int64_t)`** is a plain **positional** slice of the case
  labels — `stop` is *exclusive*, standard half-open `[start, stop)` indexing.
- **`slice(string_view, string_view)`** is a **label-value** slice: `start`
  matches the case labels' *first* occurrence of that value, `stop` matches
  the *last* occurrence of its value — and the returned range is inclusive of
  both ends. If either value never occurs, or `start`'s position ends up
  after `stop`'s, it throws.

Both overloads return the sliced case-label **values** as a bare `Slate` —
not the bound slate's own data.

## Parameters

**start** / **stop** (`std::int64_t`, positional overload)

Positions bounding the slice; `stop` is exclusive.

**start** / **stop** (`std::string_view`, label-value overload)

Case-label values bounding the slice; the range runs from `start`'s first
occurrence through `stop`'s **last** occurrence, inclusive.

## Returns

`Slate`
    The sliced case-label values, plain default integer index.

## Examples

Example 1. Positional slice — exclusive of `stop`.

```cpp
using namespace pluss::table;

auto tagged = Slate(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Slate(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().slice(1, 3);
std::cout << out;
```

### Output

```text
0  west
1  east

ptype: Note
```

Positions `1` and `2` only — position `3` (`"north"`) is excluded, since
`stop` is exclusive here.

Example 2. Label-value slice — inclusive of `stop`'s *last* occurrence, which
can include repeats not immediately obvious.

```cpp
auto out2 = tagged.label().slice("east", "east");
std::cout << out2;
```

### Output

```text
0  east
1  west
2  east

ptype: Note
```

`start = "east"` matches the *first* `"east"` (row `0`); `stop = "east"`
matches the *last* `"east"` (row `2`) — not the first one again. The result
spans rows `0` through `2` inclusive, which includes `"west"` at row `1`
even though it isn't `"east"`.

Example 3. A `start` or `stop` value that doesn't occur throws, and so does
a `start` that lands after `stop`.

```cpp
try {
  auto bad = tagged.label().slice("missing", "east");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
try {
  auto bad2 = tagged.label().slice("east", "missing");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
try {
  auto bad3 = tagged.label().slice("north", "east");
} catch (const std::exception& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
Slate::label().slice: start label not found
Slate::label().slice: stop label not found
Slate::label().slice: start label occurs after stop label
```

`"missing"` never occurs as either bound, so each throws its own not-found
message depending on which argument it was passed as. `"north"` (row `3`) and
`"east"`'s last occurrence (row `2`) both exist, but `"north"`'s position
comes *after* `"east"`'s last occurrence, so the range is backwards and
throws instead of silently returning nothing.

## Notes

- The label-value overload's `stop` bound is the **last**, not first,
  occurrence of that value — worth checking carefully when the slice value
  itself repeats, as in Example 2.
- Both overloads return values only, not the bound slate's own data — pair
  with [`pos()`](pos.md)/[`find()`](find.md) and `Slate::take` if the
  corresponding data rows are needed instead.

## See Also

- [`SlateLabelAccessor::pos`](pos.md),
  [`SlateLabelAccessor::find`](find.md): locate positions rather than slice
  a range.
- [`SlateLabelAccessor::take`](take.md): select arbitrary positions instead
  of a contiguous range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
  <a class="next-link" href="sort.html">
    <span class="label">Next</span>
    <span class="title">sort</span>
  </a>
</div>
