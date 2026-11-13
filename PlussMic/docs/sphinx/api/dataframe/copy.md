<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b2d2ce92b989 -->

# DataFrame::copy

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame DataFrame::copy() const
```

## What It Does

Returns a copy-on-write duplicate of the frame: `copy()` is implemented as simply
`return *this;`, so the new handle initially shares the exact same underlying storage
as the source. This is safe and cheap because every mutating operation on a
`DataFrame` — the trailing-underscore methods like [`title_()`](title_.md),
[`rename_()`](rename_.md), and [`set_()`](set_.md) — detaches from shared storage
first if it isn't uniquely owned, transparently cloning before writing. In practice,
this means `copy()` behaves exactly like an independent deep copy from the caller's
point of view: mutating the copy never affects the original, and vice versa. You
never need to reason about the sharing directly — it's an implementation detail that
only exists to make `copy()` (and ordinary handle copies in general) cheap when no
mutation ever happens.

## Parameters

`None`

## Returns

`DataFrame`
    A new handle to the frame's data. Feature order, case labels, title, and every
    cell value are preserved exactly.

## Examples

Example 1. Copy a frame before downstream experimentation.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"math",    pluss::make_series({80.0, 90.0, 70.0})},
    {"science", pluss::make_series({75.0, 88.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.copy();
std::cout << out;
```

### Output

```text
====================
     | math  science
--------------------
s1   | 80    75     
s2   | 90    88     
s3   | 70    92     
--------------------
[3 cs x 2 feat]
```

Example 2. The copy is independent under mutation, even though it started out sharing
storage with the source.

```cpp
auto renamed = scores.copy();
renamed.rename_("math", "mathematics");

std::cout << scores.features()[0] << "\n";    // still "math"
std::cout << renamed.features()[0] << "\n";   // "mathematics"
```

### Output

```text
math
mathematics
```

`renamed.rename_(...)` mutates `renamed` in place — internally, this triggers a
copy-on-write clone the moment it detects the underlying storage is still shared with
`scores`, so `scores` itself is completely unaffected.

## Notes

- `copy()` preserves feature order, case labels, title, and case-axis metadata exactly
  alongside every cell value — nothing is normalized or reset.
- The overwhelming majority of `DataFrame`'s own methods are `const` and already
  return a new frame rather than mutating in place, so `copy()` is mainly useful right
  before calling one of the trailing-underscore in-place mutators
  ([`rename_()`](rename_.md), [`title_()`](title_.md), [`set_()`](set_.md),
  [`put_()`](put_.md), [`fillnull_()`](fillnull_.md)) on a value you want to keep a
  pristine original of.
- `copy()` always returns a defined frame if called on a defined frame — see
  [`defined()`](defined.md).

## See Also

- [`DataFrame::equals`](equals.md): verify a copy still matches its source exactly.
- [`DataFrame::rename_`](rename_.md), [`DataFrame::title_`](title_.md): in-place
  mutators where taking a `copy()` first is most useful.
- [`DataFrame::defined`](defined.md): confirm a handle is usable at all.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conform.html">
    <span class="label">Previous</span>
    <span class="title">conform</span>
  </a>
  <a class="next-link" href="corr.html">
    <span class="label">Next</span>
    <span class="title">corr</span>
  </a>
</div>
