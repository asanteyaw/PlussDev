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

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` are
    recognized (case-insensitive); any other name throws.

## Returns

`Registry`
    A new registry with every bound field's timestamps relabeled to `name`.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabel survives a
follow-up `.tl()` read.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto shifted = rg.fields({"ts"}).tl().tz("America/New_York");
auto fmted = shifted.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.select("ts").at(0)).at(0);
```

### Output

```text
2024-01-31 04:15:30
```

The `"America/New_York"`-relative reading shows `04:15:30` (a `-5:00`
offset in January), matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same input.

## Notes

- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` — any
  other name throws.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`RegistryTimelineAccessor::localize`](localize.md): changes the
  underlying instant (which also persists) rather than just the display
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
