<!-- pluss-hand-authored -->

# AxelTimelineAccessor::utc

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabels each bound field's
display timezone back to `"UTC"`, in every registry frame, without
changing the underlying instant. Applies
[`RegistryTimelineAccessor::utc`](../registry-timeline/utc.md) to every
frame via [`Axel::map`](../axel/map.md).

## Parameters

`None`

## Returns

`Axel`
    A new axel with every bound field, in every frame, relabeled to
    `"UTC"` display. Reading the result back through any further
    `.tl()` call reflects the `"UTC"` relabeling.

## Examples

Example 1. `tz("America/New_York")` relabels the display timezone
(visible on the first `fmt()` read below); a follow-up `utc()`
relabels it back to `"UTC"`, and that too is visible on the next read —
the instant itself never changes across either call.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto ny = axel.fields({"ts"}).tl().tz("America/New_York");
std::cout << "NY-relabeled: "
          << std::get<pluss::Note>(ny.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S").registry(0).fields("ts").at(0)).at(0) << "\n";

auto out = ny.fields({"ts"}).tl().utc();
std::cout << "UTC-relabeled: "
          << std::get<pluss::Note>(out.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S").registry(0).fields("ts").at(0)).at(0);
```

### Output

```text
NY-relabeled: 2024-01-31 04:15:30
UTC-relabeled: 2024-01-31 09:15:30
```

## Notes

- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.
- Internally, each `Registry` frame stores each row's `Timeline` value
  independently (one scalar `Timeline` per row, not one shared column);
  the resulting per-row tz labels round-trip correctly through the
  `kindle::Stream` boundary every further `.tl()` call goes through,
  the same as [`tz()`](tz.md).

## See Also

- [`AxelTimelineAccessor::tz`](tz.md): the general form this method is
  a fixed `"UTC"` shorthand for.
- [`AxelTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tz.html">
    <span class="label">Previous</span>
    <span class="title">tz</span>
  </a>
  <a class="next-link" href="valid.html">
    <span class="label">Next</span>
    <span class="title">valid</span>
  </a>
</div>
