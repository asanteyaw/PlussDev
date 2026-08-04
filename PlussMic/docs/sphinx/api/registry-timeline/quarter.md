<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 441b6d34cf70 -->

# RegistryTimelineAccessor::quarter

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::quarter() const
```

## What It Does

Extracts the calendar quarter (`1`-`4`) of each bound field's timestamp,
replacing each `Timeline` field with an `Int64` `Series` field.

## Parameters

`None`

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of quarter numbers; every unbound field unchanged.

## Examples

Example 1. Extract the quarter from a July and a March timestamp.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-07-06T14:00:00Z"), pluss::make_timeline("2023-03-01T00:00:00Z")})},
});

auto out = rg.fields({"ts"}).tl().quarter();
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
3 1
```

July falls in Q3 (months 7-9); March falls in Q1 (months 1-3).

## Notes

- `1` covers January-March, `2` April-June, `3` July-September, `4`
  October-December.
- Reads local calendar fields relative to whatever timezone the field's
  `Timeline` currently carries (`"UTC"` unless changed via
  [`tz()`](tz.md)/[`localize()`](localize.md)).
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::month`](month.md): the finer-grained component
  `quarter()` is derived from.
- [`RegistryTimelineAccessor::period`](period.md): group timestamps into
  arbitrary frequency-based periods, not just quarters.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="period.html">
    <span class="label">Previous</span>
    <span class="title">period</span>
  </a>
  <a class="next-link" href="round.html">
    <span class="label">Next</span>
    <span class="title">round</span>
  </a>
</div>
