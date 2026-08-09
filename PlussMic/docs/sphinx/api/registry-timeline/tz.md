<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: f9aea3826ee0 -->

# RegistryTimelineAccessor::tz

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Relabels each bound field's timestamp *display* timezone to `name`, without
changing the underlying instant — the same operation as
[`Thread::tl().tz()`](../thread-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table layer.

The relabeling survives being read back through any further `.tl()` call
(`fmt()`, `hour()`, another `tz()`, anything) — see Example 1.

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Registry`
    A new registry with every bound field's timestamps relabeled to `name`.
    Reading the result back through any further `.tl()` call reflects the
    new display timezone.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabeling is
still in effect on the very next `.tl()` read.

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
2024-01-31 04:15:30
```

`04:15:30` is the `"America/New_York"`-relative reading of the same instant
(a `-5:00` offset in January) — matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same input.

## Notes

- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name throws.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.
- Internally, each bound field stores its `Timeline` values independently
  (one scalar `Timeline` per row, not one shared column). `tz()` relabels
  every row's `Timeline`, and that relabeling round-trips correctly through
  the `kindle::Stream` boundary every further `.tl()` call goes through —
  unlike an earlier version of this code path, which silently discarded the
  label on every subsequent `.tl()` read. If every row does not already
  share the same display timezone going into a `.tl()` call, the call
  throws rather than picking one arbitrarily — combine to a common zone
  with `tz()`/`localize()` first if needed.

## See Also

- [`RegistryTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
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
