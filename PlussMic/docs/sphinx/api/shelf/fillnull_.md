<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 33c099eb92da -->

# Shelf::fillnull_

[Back to Shelf API Reference](../../shelf_api_reference)

## Signatures

```cpp
Shelf& Shelf::fillnull_(double value)
Shelf& Shelf::fillnull_(std::string_view value)
Shelf& Shelf::fillnull_(const pluss::Timeline& value)
Shelf& Shelf::fillnull_(const pluss::Duration& value)
```

## What It Does

The mutating counterpart to [`fillnull()`](fillnull.md): fills `NULL` cells with
`value` and updates `*this` in place, returning a reference to it, instead of
returning a new `Shelf`. Copy-on-write applies — if this shelf's storage is shared
with another `Shelf` (from a copy via the copy constructor or `operator=`), the
mutation clones the storage first, so the other shelf is unaffected. See Example 2.

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

`Shelf&`
    A reference to `*this`, after the fill.

## Examples

Example 1. Fill in place; the return value is a reference to the same object.

```cpp
using namespace pluss::table;

auto readings = Shelf(pluss::make_series({10.0, 20.0, 30.0}),
    pluss::make_series(std::vector<bool>{true, false, true}));
readings.fillnull_(0.0);
std::cout << readings;
```

### Output

```text
0  0
1  20
2  0
ptype: Series
```

Example 2. Copy-on-write: mutating a copy leaves the original shelf untouched.

```cpp
auto original = Shelf(pluss::make_series({1.0, 2.0}),
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

0  99
1  2
ptype: Series
```

`copy` and `original` shared storage right up until the mutating call; `fillnull_()`
detached `copy`'s storage before writing, so `original` is unaffected.

Example 3. Payload-kind mismatches throw, same as the functional form.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b"}));
try {
  notes.fillnull_(0.0);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::fillnull_(double) is only valid for Series payload
```

## Notes

- Copy-on-write: mutating a shelf that shares storage with another (from a copy)
  detaches it first, leaving the other shelf untouched. See Example 2.
- Only the `double` overload mutates storage directly; the others reassign `*this`
  via the functional [`fillnull()`](fillnull.md). See the note above.
- The `double` overload's error message includes the underscore:
  `"Shelf::fillnull_(double) is only valid for Series payload"` — a detail that
  differs from the functional form's own message for the same overload.

## See Also

- [`Shelf::fillnull`](fillnull.md): the functional (non-mutating) form.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillnull.html">
    <span class="label">Previous</span>
    <span class="title">fillnull</span>
  </a>
  <a class="next-link" href="fillposinf.html">
    <span class="label">Next</span>
    <span class="title">fillposinf</span>
  </a>
</div>
