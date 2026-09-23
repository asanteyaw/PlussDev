<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d67ced34419d -->

# SlotTimelineAccessor::isnat

[Back to SlotTimelineAccessor API Reference](../../slot_timeline_accessor_api_reference)

## Signature

```cpp
Slot SlotTimelineAccessor::isnat() const
```

## What It Does

Flags whether each value in the bound slot is `NaT` (missing/"Not a
Time"), replacing each `Timeline` value with a `Bool` `Series`.

```{important} SlotTimelineAccessor requires the bound slot to be Timeline-kind
Every method on this accessor — not just `isnat()` — routes through
`Slot::tl()`. If the slot isn't `Timeline`-kind (`Series`, `Note`,
`TimeIndex`, `Mixed`, `Null`, or `Empty`), the call throws
`"Slot::tl: slot kind must be Timeline"`.
```

`isnat()`'s own result is never itself `NaT` — it always answers `true`
or `false`.

## Parameters

`None`

## Returns

`Slot`
    A new `Series[bool]` slot: `true` where the value is `NaT`, `false`
    otherwise.

## Examples

Example 1. Flag a `NaT` timestamp alongside a valid one. An empty string
passed to [`pluss::make_timeline`](../slot/tl.md) produces `NaT` rather
than throwing.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_timeline(""), pluss::make_timeline("2024-01-01T00:00:00Z")});
auto out = slot.tl().isnat();
std::cout << std::boolalpha
          << std::get<pluss::Series>(out.at(0)).to_vec<bool>()[0] << " "
          << std::get<pluss::Series>(out.at(1)).to_vec<bool>()[0];
```

### Output

```text
true false
```

Example 2. A non-`Timeline` slot throws — see the important callout
above.

```cpp
auto slot = pluss::scribble::Slot({pluss::make_series(1)});
try {
  auto out = slot.tl().isnat();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slot::tl: slot kind must be Timeline
```

## Notes

- See the important callout above.
- Unlike almost every other method on this accessor, `isnat()`'s output
  is never itself masked — it directly answers the missingness question.

## See Also

- [`SlotTimelineAccessor::valid`](valid.md): the logical inverse.
- [`Slot::tl`](../slot/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="index.html">
    <span class="label">Previous</span>
    <span class="title">index</span>
  </a>
  <a class="next-link" href="iso.html">
    <span class="label">Next</span>
    <span class="title">iso</span>
  </a>
</div>
