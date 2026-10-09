<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: ab508cf350d3 -->

# Stream::has_label

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::has_label() const -> bool
```

## What It Does

Reports whether this stream carries a **row-label axis** — an optional
`pluss::table::Slate` of per-row labels, set via the constructor's `cases` parameter or
via [`with_label()`](with_label.md)/[`Stream::lbl()`](lbl.md)`.set(...)`. Most streams
don't carry one; `Stream(data)` alone leaves it unset, and `has_label()` returns `false`.

```{important} Two related but distinct surfaces: label()/has_label() and lbl()
`has_label()`/[`label()`](label.md) are a plain getter pair for the raw optional label
`Slate` — cheap, no-frills, no default-value synthesis. [`lbl()`](lbl.md) returns a
`StreamLabelAccessor` with a richer surface (`values()`, `set()`, `range()`, `reset()`,
`align()`) — notably, `lbl().values()` always returns *something* displayable, synthesizing
a positional `0..n-1` range when no label has actually been set. `has_label()` still
reports `false` in that synthesized case; only an explicitly set label counts.
```

## Parameters

`None`

## Returns

`bool`
    `true` if a row-label `Slate` has been explicitly set (via a constructor's `cases`
    argument, [`with_label()`](with_label.md)/[`with_label_()`](with_label_.md), or
    [`lbl().set()`](lbl.md)); `false` otherwise, including right after
    [`lbl().reset()`](lbl.md) or [`clear_label()`](clear_label.md).

## Examples

Example 1. A plain stream built without a `cases` argument has no label.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.has_label();
```

### Output

```text
0
```

Example 2. Passing `cases` to the constructor sets the label immediately.

```cpp
auto cases = pluss::table::Slate(pluss::make_note({"a", "b", "c"}));
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}), std::nullopt, cases);

std::cout << stream.has_label();
```

### Output

```text
1
```

Example 3. `has_label()` stays `false` even though [`lbl().values()`](lbl.md) already
returns a displayable (synthesized) range — the two report different things.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << "has_label: " << stream.has_label() << "\n";
std::cout << stream.lbl().values();
```

### Output

```text
has_label: 0
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[int64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 0                                                                                                        
1            | 1                                                                                                        
2            | 2                                                                                                        
========================================================================================================================
```

## Notes

- Cheap and side-effect-free; safe to call before deciding whether to fall back to
  [`lbl().range()`](lbl.md) for a synthesized default.
- Mirrors [`has_meta()`](has_meta.md)'s existence-check shape, but for the label axis
  instead of the free-form metadata map.

## See Also

- [`Stream::label`](label.md): the paired getter — returns the actual optional label
  `Slate` this method is only checking for.
- [`Stream::lbl`](lbl.md): the fuller label accessor (`values()`, `set()`, `range()`,
  `reset()`, `align()`).
- [`Stream::with_label`](with_label.md) / [`Stream::clear_label`](clear_label.md): set or
  clear the label this method reports on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="from_slate.html">
    <span class="label">Previous</span>
    <span class="title">from_slate</span>
  </a>
  <a class="next-link" href="has_meta.html">
    <span class="label">Next</span>
    <span class="title">has_meta</span>
  </a>
</div>
