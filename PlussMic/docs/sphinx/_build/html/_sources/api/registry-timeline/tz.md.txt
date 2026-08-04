<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f9aea3826ee0 -->

# RegistryTimelineAccessor::tz

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Intended to relabel each bound field's timestamp *display* timezone to
`name`, without changing the underlying instant — the same operation as
[`Thread::tl().tz()`](../thread-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table layer.

```{danger} At the Registry/Slot layer, the display-timezone change does not survive being read back
Verified: `tz()` returns without error and accepts the same four timezone
names as the table layer, but every bound field stores its `Timeline`
values independently (one scalar `Timeline` per row, not one shared column),
and every `.tl()` accessor call — including the *next* one you make to read
the result — reconstructs those values from raw epoch-nanoseconds via a
UTC-only conversion path. The practical effect: immediately calling another
`.tl()` method (`fmt()`, `hour()`, a further `tz()` call, anything) on the
result shows the timestamp exactly as if `tz()` had never been called — the
relabeling is silently discarded. See Example 1. This differs from the
`Thread`/`DataFrame`/`Atlas` layer, where `tz()` reliably relabels display.
If you need a working display-timezone change today, do the table-layer
conversion via [`Registry::to_kindle()`](../registry/to_kindle.md) and its
`Catalog`, or track this at the `Thread`/`DataFrame` level instead.
```

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Registry`
    A new registry with every bound field's timestamps nominally relabeled
    to `name` — see the danger callout above for why this doesn't survive
    being read back.

## Examples

Example 1. `tz()` accepts the call and returns without error, but a
follow-up `.tl()` read shows the original, UTC-labeled wall time —
demonstrating the limitation described above.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto shifted = rg.fields({"ts"}).tl().tz("America/New_York");
auto fmted = shifted.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0);
```

### Output

```text
2024-01-31 09:15:30
```

The expected `"America/New_York"`-relative reading would show `04:15:30`
(a `-5:00` offset in January), matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same input —
but it doesn't happen here, per the callout above.

## Notes

- See the danger callout above — this is a confirmed limitation of the
  current `Registry`/`Slot` implementation, not intended behavior.
- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name throws, independent of the display issue above.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`,
  subject to the same limitation.
- [`RegistryTimelineAccessor::localize`](localize.md): changes the
  underlying instant (which *does* persist) rather than just the display
  label.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="time.html">
    <span class="label">Previous</span>
    <span class="title">time</span>
  </a>
  <a class="next-link" href="utc.html">
    <span class="label">Next</span>
    <span class="title">utc</span>
  </a>
</div>
