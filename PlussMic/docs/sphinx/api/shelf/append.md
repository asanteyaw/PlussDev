<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 23e356d1f482 -->

# Shelf::append

[Back to Shelf API Reference](../../shelf_api_reference)

## Signature

```cpp
Shelf Shelf::append(const Shelf& other) const
```

## What It Does

Concatenates `other` onto the end of `*this`, returning a new, longer shelf. This
is a strict operation: `*this` and `other` must have the same [`kind()`](kind.md),
and for `Series`-kind shelves, the same concrete scalar dtype and platform too —
`append()` never coerces or promotes types to make two mismatched shelves fit
together.

## Parameters

**other** (`const Shelf&`)

The shelf to append. Must match `*this` exactly in payload kind (and, for
`Series`, scalar dtype and platform).

## Returns

`Shelf`
    A new shelf, length `this->size() + other.size()`, holding `*this`'s entries
    followed by `other`'s.

## Examples

Example 1. Append two compatible `Series`-kind shelves.

```cpp
using namespace pluss::table;

auto first = Shelf(pluss::make_series({10.0, 20.0}));
auto second = Shelf(pluss::make_series({30.0, 40.0}));
auto out = first.append(second);
std::cout << out;
```

### Output

```text
0  10
1  20
2  30
3  40

ptype: Series
```

Example 2. Appending shelves of different payload kinds throws.

```cpp
auto notes = Shelf(pluss::make_note({"a", "b"}));
try {
  auto bad = first.append(notes);
} catch (const pluss::Error& e) {
  std::cout << e.message();
}
```

### Output

```text
Shelf::append: payload kinds must match
```

## Notes

- Requires identical `kind()` between `*this` and `other`; throws
  `"Shelf::append: payload kinds must match"` otherwise.
- For `Series`-kind shelves, also requires identical concrete scalar dtype
  (`"Shelf::append: Series dtypes must match"`) and platform
  (`"Shelf::append: Series platforms must match"`).
- Validity masks combine correctly even when only one side has one — a side with
  no explicit mask is treated as fully valid.
- Also requires both shelves to agree on whether they carry case labels at all
  (`"Shelf::append: both Shelves must either have case labels or both be
  unlabeled"`); if both do, the labels are appended too.

## See Also

- [`Shelf::take`](take.md): combine with `append()` for more elaborate
  reassembly.
- [`Shelf::cast`](cast.md) / [`Shelf::astype`](astype.md): convert dtype first
  if two `Series`-kind shelves have mismatched scalar types.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="any.html">
    <span class="label">Previous</span>
    <span class="title">any</span>
  </a>
  <a class="next-link" href="arrange.html">
    <span class="label">Next</span>
    <span class="title">arrange</span>
  </a>
</div>
