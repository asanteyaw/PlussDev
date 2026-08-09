<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e233cc82a48a -->

# AxelTimelineAccessor::index

[Back to AxelTimelineAccessor API Reference](../../axel_timeline_accessor_api_reference)

## Signature

```cpp
Axel AxelTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts every bound field from `Timeline`-kind to `TimeIndex`-kind, in
every registry frame. Applies
[`RegistryTimelineAccessor::index`](../registry-timeline/index.md) to
every frame via [`Axel::map`](../axel/map.md).

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in a bound field must be
    non-decreasing (and non-`NaT`), or the call throws. When `false`,
    the conversion happens without checking order or `NaT`.

## Returns

`Axel`
    A new axel with every bound field, in every frame, converted from
    `Timeline`-kind to `TimeIndex`-kind; every unbound field unchanged.

## Examples

Example 1. Convert a sorted `Timeline` field to `TimeIndex`.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-01T00:00:00Z"), pluss::make_timeline("2024-01-02T00:00:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

auto out = axel.fields({"ts"}).tl().index();
std::cout << out.registry(0).fields("ts").to_json();
```

### Output

```text
{
  "_type": "Slot",
  "defined": true,
  "size": 2,
  "values": [
    "TimeIndex[size=1]",
    "TimeIndex[size=1]"
  ]
}
```

Example 2. An out-of-order field throws under the default
`require_sorted = true`.

```cpp
auto r0 = pluss::bundle::Registry({{"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-02T00:00:00Z"), pluss::make_timeline("2024-01-01T00:00:00Z")})}});
auto axel = pluss::bundle::Axel({r0});

try {
  auto out = axel.fields({"ts"}).tl().index();
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
TimeIndex must be sorted (non-decreasing)
```

## Notes

- `require_sorted = false` skips the check entirely — use it only when
  a field's order doesn't matter for what comes next.
- Requires every bound field, in every frame, to be `Timeline`-kind — a
  non-`Timeline` field throws `"Slot::tl: slot kind must be Timeline"`.
- Each converted value becomes its own single-element `TimeIndex`
  (`"TimeIndex[size=1]"` per row, as seen in Example 1's `to_json()`
  output) — a `Registry` field stores one independent value per row
  rather than one shared column, so `index()` on an `N`-row `Timeline`
  field produces `N` separate one-element `TimeIndex` values, not one
  `N`-element `TimeIndex`.

## See Also

- [`Axel::from_kindle`](../axel/from_kindle.md): the round-trip partner
  that reconstructs `TimeIndex` values from a `kindle::Archive`.
- [`Axel::tl`](../axel/tl.md): the entry point this accessor is reached
  from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="isnat.html">
    <span class="label">Next</span>
    <span class="title">isnat</span>
  </a>
</div>
