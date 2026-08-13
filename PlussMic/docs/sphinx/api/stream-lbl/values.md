<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 62349bfebda1 -->

# StreamLabelAccessor::values

[Back to StreamLabelAccessor API Reference](../../stream_lbl_accessor_api_reference)

## Signature

```cpp
Stream StreamLabelAccessor::values() const
```

## What It Does

Returns the bound stream's row labels *as data* — the label's own content
becomes the **values** of a new, unlabeled `Stream`. If no label has
actually been set, `values()` synthesizes a positional `0..n-1` range as
that content instead of throwing or returning empty.

```{important} values() always returns something — has() is the accurate existence check
`values()` never fails and never signals "no label" through its return
value; the synthesized positional range looks identical in shape to a real
extracted label. If you need to know whether a label was *actually* set on
the *original* stream, call [`has()`](has.md) on it — don't try to infer it
from `values()`'s result, whose own `has()` is unrelated (see Example 2).
```

```{important} values() is the inverse framing of range()
[`range()`](range.md) keeps the original *values* and replaces the
*label*. `values()` does the opposite: it discards the original values and
returns the *label's own content* as the new stream's values. The two are
complementary extraction directions on the same label/value pair.
```

## Parameters

`None`

## Returns

`Stream`
    The current row labels, or a synthesized `0..n-1` positional range if
    none has been set.

## Examples

Example 1. No label set — `values()` synthesizes a positional range, while
`has()` correctly reports `false`.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << "has(): " << std::boolalpha << stream.lbl().has() << "\n";
std::cout << stream.lbl().values();
```

### Output

```text
has(): false
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

`values()` returns the positions `[0, 1, 2]` as an `Int64` `Series` stream —
the display's own row labels (`label=no`) confirm the *source* stream itself
still carries no real label, even though `values()` handed back a usable
result.

Example 2. With a genuine `Note`-kind label set, `values()` extracts that
label's own content and kind — not a synthesized range, and not the bound
stream's numeric values.

```cpp
using pluss::table::Thread;

auto labeled = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .lbl().set(Thread(pluss::make_note({"x", "y", "z"})));

std::cout << "result has(): " << std::boolalpha << labeled.lbl().values().lbl().has() << "\n";
std::cout << labeled.lbl().values();
```

### Output

```text
result has(): false
========================================================================================================================
kindle::Stream  kind=Note  dtype=Note  records=3
label=no  mask=no  meta=0
------------------------------------------------------------------------------------------------------------------------
label        | value                                                                                                    
------------------------------------------------------------------------------------------------------------------------
0            | x                                                                                                        
1            | y                                                                                                        
2            | z                                                                                                        
========================================================================================================================
```

The original stream's `Note`-kind label `["x", "y", "z"]` becomes the
returned stream's `Note`-kind *values* — the original numeric values
(`10`/`20`/`30`) are gone entirely. The returned stream is itself unlabeled
(`has() == false`, displayed with default positional row labels
`0`/`1`/`2`) — `values()` never attaches a label to what it returns.

## Notes

- See the important callouts above.
- The synthesized (no-label) case always produces `Int64` positions; a real
  label's own dtype/kind is preserved as-is (Example 2).
- The `Stream` returned by `values()` never itself carries a label — its
  own `.lbl().has()` is always `false`, regardless of whether the *source*
  stream had one.

## See Also

- [`StreamLabelAccessor::has`](has.md): the accurate existence check this
  method's own callout points to.
- [`StreamLabelAccessor::range`](range.md): the inverse extraction —
  preserves the original values and replaces the label instead.
- [`Stream::lbl`](../stream/lbl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="set.html">
    <span class="label">Previous</span>
    <span class="title">set</span>
  </a>
</div>
