<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: d88a07f091ea -->

# RegistryTimelineAccessor::utc

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::utc() const
```

## What It Does

Shorthand for [`tz("UTC")`](tz.md) — relabel each bound field's display
timezone back to `"UTC"` without changing the underlying instant.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field relabeled to `"UTC"` display.

## Examples

Example 1. Calling `utc()` after `tz()` relabels back to UTC, producing the
same displayed value as never having called `tz()` at all.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto ny = rg.fields({"ts"}).tl().tz("America/New_York");
auto out = ny.fields({"ts"}).tl().utc();
auto fmted = out.fields("ts").tl().fmt("%Y-%m-%d %H:%M:%S");
std::cout << std::get<pluss::Note>(fmted.select("ts").at(0)).at(0);
```

### Output

```text
2024-01-31 09:15:30
```

## Notes

- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::tz`](tz.md): the general form this method is
  a fixed `"UTC"` shorthand for.
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
