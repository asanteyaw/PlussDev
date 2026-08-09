<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e58a71d4010c -->

# AxelTimelineAccessor::tz

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::tz(std::string_view name) const
```

## What It Does

Relabels each bound field's timestamp *display* timezone to `name`, in
every registry frame, without changing the underlying instant — the
same operation as
[`Thread::tl().tz()`](../thread-timeline/tz.md)/
[`DataFrame::tl().tz()`](../dataframe-timeline/tz.md) at the table
layer. Applies
[`RegistryTimelineAccessor::tz`](../registry-timeline/tz.md) to every
frame via [`Axel::map`](../axel/map.md).

The relabeling survives being read back through any further `.tl()`
call (`fmt()`, `hour()`, another `tz()`, anything) — see Example 1.

## Parameters

**name** (`std::string_view`)
    The target timezone. Only `"UTC"` (also the empty string),
    `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"`
    are recognized (case-insensitive); any other name throws.

## Returns

`Axel`
    A new axel with every bound field's timestamps, in every frame,
    relabeled to `name`. Reading the result back through any further
    `.tl()` call reflects the new display timezone.

## Examples

Example 1. `tz()` relabels the display timezone, and the relabeling is
still in effect on the very next `.tl()` read.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto shifted = axel.fields({"ts"}).tl().tz("America/New_York");
auto fmted = shifted.fields({"ts"}).tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.registry(0).fields("ts").at(0)).at(0);
```

### Output

```text
2024-01-31 04:15:30
```

`04:15:30` is the `"America/New_York"`-relative reading of the same
instant (a `-5:00` offset in January) — matching what
[`Thread::tl().tz()`](../thread-timeline/tz.md) produces on the same
input.

## Notes

- Only four timezone names are recognized: `"UTC"` (or empty string),
  `"America/Edmonton"`, `"America/New_York"`, and `"Europe/London"` —
  any other name throws.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.
- Internally, each `Registry` frame stores each row's `Timeline` value
  independently (one scalar `Timeline` per row, not one shared column).
  `tz()` relabels every value, and that relabeling round-trips
  correctly through the `kindle::Stream` boundary every further
  `.tl()` call goes through. If every value does not already share the
  same display timezone going into a `.tl()` call, the call throws
  rather than picking one arbitrarily — combine to a common zone with
  `tz()`/`localize()` first if needed.

## See Also

- [`AxelTimelineAccessor::utc`](utc.md): shorthand for `tz("UTC")`.
- [`AxelTimelineAccessor::localize`](localize.md): changes the
  underlying instant (which *does* persist) rather than just the
  display label.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

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
