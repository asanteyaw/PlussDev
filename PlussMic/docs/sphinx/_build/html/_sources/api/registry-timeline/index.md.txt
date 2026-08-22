<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a28b0b406e3e -->

# RegistryTimelineAccessor::index

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::index(bool require_sorted = true) const
```

## What It Does

Converts each bound field from `Timeline`-kind to `TimeIndex`-kind.

## Parameters

**require_sorted** (`bool`, defaults to `true`)
    When `true` (the default), every value in the field must be
    non-decreasing (and non-`NaT`), or the call throws. When `false`, the
    conversion happens without checking order or `NaT`.

## Returns

`Registry`
    A new registry with every bound field converted from `Timeline`-kind to
    `TimeIndex`-kind; every unbound field unchanged.

## Examples

Example 1. Convert a sorted `Timeline` field to `TimeIndex`.

```cpp
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-01T00:00:00Z"), pluss::make_timeline("2024-01-02T00:00:00Z")})},
});

auto out = rg.fields({"ts"}).tl().index();
std::cout << out.fields("ts").to_json();
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
auto rg = pluss::scribble::Registry({
    {"ts", pluss::scribble::Slot({pluss::make_timeline("2024-01-02T00:00:00Z"), pluss::make_timeline("2024-01-01T00:00:00Z")})},
});

try {
  auto out = rg.fields({"ts"}).tl().index();
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

- `require_sorted = false` skips the check entirely — use it only when the
  field's order doesn't matter for what comes next.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryFieldAxisAccessor::timeindex`](../registry-fieldaxis/timeindex.md):
  select fields already at `TimeIndex`-kind.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

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
