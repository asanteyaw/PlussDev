<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 6262b3852fe0 -->

# StreamLabelAccessor::range

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signature

```cpp
Stream StreamLabelAccessor::range() const
```

## What It Does

Returns the bound stream **unchanged in its values**, but with its row
label reset to the default positional range (`0..n-1`) — overwriting
whatever label (custom or none) was there before. `range()` is the
"reset to the default label" counterpart to [`reset()`](reset.md)'s
"clear to no label at all": both discard any existing custom label, but
`range()` leaves `has()` reporting `true` (a real positional label is now
set), whereas `reset()` leaves `has()` reporting `false`.

```{important} range() keeps your data — only the label changes
It's easy to misread `range()` as producing a small `Int64` stream of
`0..n-1` values. It does not: the returned stream's *values* are exactly
the bound stream's original values, untouched. Only the *label* is
replaced with the default `0..n-1` positional range. Contrast this with
[`values()`](values.md), which does the opposite — it discards the values
and returns the label's own content as the new stream's values.
```

## Parameters

`None`

## Returns

`Stream`
    A new stream with the same values as the bound stream, and its row
    label replaced by the default positional range `0..n-1`.

## Examples

Example 1. `range()` on an unlabeled 3-position stream — the values
(`10`/`20`/`30`) pass through unchanged; only the label goes from unset to
the default positional range.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto out = stream.lbl().range();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

The `0`/`1`/`2` you see is the **label** column, not the values —
`label=yes` in the header confirms a real label is now attached, and the
`value` column (`10`/`20`/`30`) is untouched from the original stream.

Example 2. `range()` overwrites an *existing custom* label, not just an
absent one.

```cpp
using pluss::table::Slate;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Slate(pluss::make_note({"x", "y", "z"})));
auto out = labeled.lbl().range();
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=yes  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 10                                                                                                       
1            | 20                                                                                                       
2            | 30                                                                                                       
========================================================================================================================
```

`labeled` carried the custom label `["x", "y", "z"]`; `range()` discards it
entirely and replaces it with the default `0`/`1`/`2` positional range —
the same result as Example 1, regardless of what label state the bound
stream started in.

## Notes

- See the important callout above — `range()` changes the label, not the
  values.
- `range()` always produces the same default positional label, regardless
  of whether the bound stream started unlabeled or with a custom label
  (Example 2).
- Length matches the bound stream's own length.

## See Also

- [`StreamLabelAccessor::values`](values.md): the inverse framing — turns
  the label into the stream's values, rather than replacing the label.
- [`StreamLabelAccessor::reset`](reset.md): also discards any existing
  label, but clears to the unset state (`has() == false`) instead of the
  default positional range.
- [`StreamLabelAccessor::set`](set.md): attach an arbitrary label instead of
  the positional default.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="reset.html">
    <span class="label">Next</span>
    <span class="title">reset</span>
  </a>
</div>
