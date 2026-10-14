<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33c099eb92da -->

# Slate::fillnull_

[Back to Slate API Reference](../../slate_api_reference)

## Signatures

```cpp
Slate& Slate::fillnull_(double value)
Slate& Slate::fillnull_(std::string_view value)
Slate& Slate::fillnull_(const pluss::Timeline& value)
Slate& Slate::fillnull_(const pluss::Duration& value)
```

## What It Does

The mutating counterpart to [`fillnull()`](fillnull.md): fills `NULL` cells with
`value` and updates `*this` in place, returning a reference to it, instead of
returning a new `Slate`. Copy-on-write applies — if this slate's storage is shared
with another `Slate` (from a copy via the copy constructor or `operator=`), the
mutation clones the storage first, so the other slate is unaffected. See Example 2.

```{note} Only the double overload is genuinely in-place
The `double` overload mutates the underlying `Series` storage directly through the
copy-on-write path. The `std::string_view`, `Timeline`, and `Duration` overloads are
implemented as `*this = fillnull(value);` — they call the functional
[`fillnull()`](fillnull.md) and reassign the result to `*this`. The end result and
copy-on-write behavior are the same either way; only the internal mechanism differs.
```

## Parameters

**value**
    Same as [`fillnull()`](fillnull.md) — the replacement for `NULL` cells,
    selecting the overload (and required payload kind) by its C++ type.

## Returns

`Slate&`
    A reference to `*this`, after the fill.

## Examples

Example 1. Fill in place; the return value is a reference to the same object.

```cpp
using namespace pluss::table;

auto readings = Slate(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
readings.fillnull_(0.0);
std::cout << readings;
```

### Output

```text
0  10
1  0
2  30
ptype: Series
```

Example 2. Copy-on-write: mutating a copy leaves the original slate untouched.

```cpp
auto original = Slate(pluss::make_series({1.0, 2.0}),
    pluss::make_series(std::vector<bool>{true, false}));
auto copy = original;

copy.fillnull_(99.0);
std::cout << original << "\n";
std::cout << copy;
```

### Output

```text
0  1
1  NULL
ptype: Series

0  1
1  99
ptype: Series
```

`copy` and `original` shared storage right up until the mutating call; `fillnull_()`
detached `copy`'s storage before writing, so `original` is unaffected.

Example 3. Payload-kind mismatches throw, same as the functional form.

```cpp
auto notes = Slate(pluss::make_note({"a", "b"}));
try {
  notes.fillnull_(0.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::fillnull_(double) is only valid for Series payload
```

## Notes

- Copy-on-write: mutating a slate that shares storage with another (from a copy)
  detaches it first, leaving the other slate untouched. See Example 2.
- Only the `double` overload mutates storage directly; the others reassign `*this`
  via the functional [`fillnull()`](fillnull.md). See the note above.
- The `double` overload's error message includes the underscore:
  `"Slate::fillnull_(double) is only valid for Series payload"` — a detail that
  differs from the functional form's own message for the same overload.

## See Also

- [`Slate::fillnull`](fillnull.md): the functional (non-mutating) form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="filter.html">
    <span class="label">Next</span>
    <span class="title">filter</span>
  </a>
</div>
