<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 19bacb69958c -->

# Axel::concat_depth

[Back to Axel API Reference](../../axel_api_reference)

## Signature

```cpp
Axel Axel::concat_depth(const Axel& other) const
```

## What It Does

Concatenates `other`'s registry frames after this axel's own frames
along the depth axis, returning a new `Axel` with
`depth() + other.depth()` frames.

```{danger} Labels are dropped entirely unless BOTH sides have them
If exactly one of the two axels carries [`labels()`](labels.md), the
result has **no labels at all** — the surviving side's labels are
silently discarded rather than padded or partially kept. Labels only
carry through when both `this` and `other` have them, in which case
they're concatenated with [`Slot::concat`](../slot/concat.md). See
Example 2.
```

## Parameters

**other** (`const Axel&`)
    The axel whose frames are appended after this one's. Must be
    defined.

## Returns

`Axel`
    A new axel holding this axel's frames followed by `other`'s frames.

## Examples

Example 1. Concatenate two two-frame axels into one four-frame axel.

```cpp
auto a = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
});
auto b = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
});
auto out = a.concat_depth(b);
std::cout << out.depth();
```

### Output

```text
4
```

Example 2. Labels only survive when *both* sides have them — here `a`
is labeled and `b` is not, so the concatenated result has no labels.

```cpp
auto a = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
}, pluss::scribble::Slot({pluss::make_note("l0"), pluss::make_note("l1")}));
auto b = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
});
auto out = a.concat_depth(b);
std::cout << std::boolalpha << out.label().has_value();
```

### Output

```text
false
```

Example 3. When both sides are labeled, the labels concatenate too.

```cpp
auto a = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(0)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(1)})}}),
}, pluss::scribble::Slot({pluss::make_note("l0"), pluss::make_note("l1")}));
auto b = pluss::scribble::Axel({
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(2)})}}),
    pluss::scribble::Registry({{"ids", pluss::scribble::Slot({pluss::make_series(3)})}}),
}, pluss::scribble::Slot({pluss::make_note("l2"), pluss::make_note("l3")}));
auto out = a.concat_depth(b);
std::cout << "has_value=" << std::boolalpha << out.label().has_value() << " size=" << out.label()->size();
```

### Output

```text
has_value=true size=4
```

## Notes

- See the danger callout above — an asymmetric label presence between
  `this` and `other` is not an error, but it silently produces an
  unlabeled result. If your workflow depends on labels surviving a
  `concat_depth()` call, ensure both sides are labeled first (or neither
  is).
- `other` must be `defined()` — an undefined `other` throws
  `"Axel::concat_depth: other axel undefined"`. This axel itself must
  also be `defined()`, throwing `"Axel: undefined handle"` otherwise.
- Registry frames are not merged or validated against each other for
  schema compatibility here — `concat_depth()` only concatenates the
  depth-axis list; each frame keeps whatever fields it already had.

## See Also

- [`Axel::labels`](labels.md): the depth-axis label slot this method
  can silently drop.
- [`Axel::slice`](slice.md): the inverse-flavored operation, selecting a
  sub-range of frames instead of appending more.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cast.html">
    <span class="label">Previous</span>
    <span class="title">cast</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
