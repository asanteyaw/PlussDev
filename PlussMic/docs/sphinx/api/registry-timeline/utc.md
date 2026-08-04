<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d88a07f091ea -->

# RegistryTimelineAccessor::utc

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::utc() const
```

## What It Does

Intended as a shorthand for [`tz("UTC")`](tz.md) — relabel each bound
field's display timezone back to `"UTC"` without changing the underlying
instant.

```{danger} Subject to the same display-label limitation as tz()
Since [`tz()`](tz.md) itself doesn't make a display-timezone relabel survive
being read back at the `Registry`/`Slot` layer (see its page for the full
explanation), `utc()` is effectively unobservable here too — a field's
timestamps already read back as UTC by default on every `.tl()` call,
`utc()` included, regardless of any prior `tz()`/`localize()` call.
```

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field nominally relabeled to `"UTC"`
    display — see the danger callout above.

## Examples

Example 1. Calling `utc()` after `tz()` produces the same displayed values
as not calling `tz()` at all, since neither call's relabeling persists past
the next `.tl()` read.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto ny = rg.fields({"ts"}).tl().tz("America/New_York");
auto out = ny.fields({"ts"}).tl().utc();
auto fmted = out.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.at(0)).at(0);
```

### Output

```text
2024-01-31 09:15:30
```

## Notes

- See the danger callout above and [`tz()`](tz.md)'s page for the full
  explanation of why this doesn't behave the way the equivalent
  `Thread`/`DataFrame`/`Atlas` method does.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::tz`](tz.md): the general form this method is
  a fixed `"UTC"` shorthand for, with the full explanation of the
  round-trip limitation.
- [`RegistryTimelineAccessor::localize`](localize.md): the operation that
  actually changes the underlying instant.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

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
