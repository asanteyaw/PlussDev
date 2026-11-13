<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 09fc636c2fc0 -->

# Stream::label

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
const std::optional<pluss::table::Slate>& Stream::label() const
```

## What It Does

Returns the stream's row-label axis as a raw `std::optional<pluss::table::Slate>` — engaged
when a label has been set (via a constructor's `cases` argument,
[`with_label()`](with_label.md)/[`with_label_()`](with_label_.md), or
[`lbl().set()`](lbl.md)), and `std::nullopt` otherwise. It is the plain getter counterpart
to [`has_label()`](has_label.md); call `has_label()` first (or check `.has_value()`) before
dereferencing with `.value()`, since an unset label has no fallback value here — that
synthesis only happens on [`lbl().values()`](lbl.md)'s richer surface, not this one.

## Parameters

`None`

## Returns

`const std::optional<pluss::table::Slate>&`
    The label `Slate` if one is set; `std::nullopt` otherwise. Returned by reference — no
    copy is made.

## Examples

Example 1. No label set: `label()` is an engaged-but-empty optional.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.label().has_value();
```

### Output

```text
0
```

Example 2. A label set via the constructor's `cases` argument is returned as-is.

```cpp
auto cases = pluss::table::Slate(pluss::make_note({"a", "b", "c"}));
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}), std::nullopt, cases);

std::cout << stream.label().value();
```

### Output

```text
0  a
1  b
2  c
ptype: Note
```

Example 3. Guard with `has_label()`/`.has_value()` before dereferencing — `.value()` on an
unset label throws `std::bad_optional_access`, unlike `lbl().values()`'s silent fallback.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

if (stream.label().has_value()) {
  std::cout << stream.label().value();
} else {
  std::cout << "no label set";
}
```

### Output

```text
no label set
```

## Notes

- Returns the raw optional by `const&` — no default-value synthesis, unlike
  [`lbl().values()`](lbl.md).
- The returned `Slate` is whatever kind was originally supplied as `cases` — commonly
  `Note` for string labels, but any `Slate` payload kind is accepted.

## See Also

- [`Stream::has_label`](has_label.md): the paired existence check — call it (or
  `.has_value()` on this method's result) before dereferencing.
- [`Stream::lbl`](lbl.md): the fuller label accessor, whose `values()` always returns a
  displayable `Stream` even when no label has been set.
- [`Stream::with_label`](with_label.md): sets the value this method returns.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="kind.html">
    <span class="label">Previous</span>
    <span class="title">kind</span>
  </a>
  <a class="next-link" href="lbl.html">
    <span class="label">Next</span>
    <span class="title">lbl</span>
  </a>
</div>
