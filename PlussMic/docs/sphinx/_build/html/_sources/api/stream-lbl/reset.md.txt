<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b81c2fd04388 -->

# StreamLabelAccessor::reset

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signature

```cpp
Stream StreamLabelAccessor::reset() const
```

## What It Does

Clears the bound stream's row label back to the unset state, returning a
new stream. After `reset()`, [`has()`](has.md) reports `false` again, even
though [`values()`](values.md) still returns a usable synthesized positional
range.

## Parameters

`None`

## Returns

`Stream`
    A new stream with no row label set; every other aspect of the stream
    (values, mask) is unchanged.

## Examples

Example 1. Set a label, then clear it back to unset.

```cpp
using pluss::table::Slate;

auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));
auto labeled = stream.lbl().set(Slate(pluss::make_note({"a", "b", "c"})));
std::cout << "labeled has(): " << std::boolalpha << labeled.lbl().has() << "\n";

auto back = labeled.lbl().reset();
std::cout << "after reset has(): " << std::boolalpha << back.lbl().has() << "\n";
std::cout << back;
```

### Output

```text
labeled has(): true
after reset has(): false
========================================================================================================================
kindle::Stream  kind=Series  dtype=Series[float64]  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | 1                                                                                                        
1            | 2                                                                                                        
2            | 3                                                                                                        
========================================================================================================================
```

After `reset()`, the stream's display falls back to positional `0..n-1`
row labels — the same synthesized display [`values()`](values.md) would
produce — but `label=no` in the header and `has() == false` both confirm no
real label is attached.

## Notes

- `reset()` clears the label; it doesn't merely hide it — [`has()`](has.md)
  reports `false` afterward, unlike a label that's merely all-default
  values.
- Returns a new stream — the original `labeled` stream is unaffected.

## See Also

- [`StreamLabelAccessor::set`](set.md): the inverse operation, attaching a
  new label.
- [`StreamLabelAccessor::has`](has.md): confirms the label was actually
  cleared.
- [`Stream::clear_label`](../stream/clear_label.md) /
  [`clear_label_`](../stream/clear_label_.md): equivalent operations reached
  outside this accessor.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="range.html">
    <span class="label">Previous</span>
    <span class="title">range</span>
  </a>
  <a class="next-link" href="set.html">
    <span class="label">Next</span>
    <span class="title">set</span>
  </a>
</div>
