<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 60e5411aa1bc -->

# RegistryTimelineAccessor::epoch

[Back to RegistryTimelineAccessor API Reference](../../registry_timeline_accessor_api_reference)

## Signature

```cpp
Registry RegistryTimelineAccessor::epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const
```

## What It Does

Extracts each bound field's timestamp as a Unix epoch value in `unit`,
replacing each `Timeline` field with an `Int64` `Series` field.

```{important} The default unit is Nanos, not Seconds
A bare `epoch()` call reports nanoseconds since the Unix epoch, not seconds —
easy to assume otherwise, since seconds is the more familiar epoch unit in
casual use. Pass `TimeUnit::Seconds` explicitly for the more commonly
expected scale.
```

## Parameters

**unit** (`pluss::table::TimeUnit`, defaults to `pluss::table::TimeUnit::Nanos`)
    One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`,
    or `Days`. Passing a span-only unit (`Weeks`, `Months`, `Years`,
    `BusinessDays`) throws.

## Returns

`Registry`
    A new registry with every bound field replaced by an `Int64` `Series`
    field of epoch counts in `unit`; every unbound field unchanged.

## Examples

Example 1. Default call reports nanoseconds — see the important callout
above.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

auto out = rg.fields({"ts"}).tl().epoch();
std::cout << std::get<pluss::Series>(out.fields("ts").at(0)).to_vec<std::int64_t>()[0];
```

### Output

```text
1706692530000000000
```

Example 2. Pass `TimeUnit::Seconds` explicitly for the more commonly
expected scale.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z"), pluss::make_timeline("2024-02-29T23:45:00Z")})},
});

auto out = rg.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Seconds);
auto ts = out.fields("ts");
std::cout << std::get<pluss::Series>(ts.at(0)).to_vec<std::int64_t>()[0] << " "
          << std::get<pluss::Series>(ts.at(1)).to_vec<std::int64_t>()[0];
```

### Output

```text
1706692530 1709250300
```

Example 3. A span-only unit throws.

```cpp
auto rg = pluss::bundle::Registry({
    {"ts", pluss::bundle::Slot({pluss::make_timeline("2024-01-31T09:15:30Z")})},
});

try {
  auto out = rg.fields({"ts"}).tl().epoch(pluss::table::TimeUnit::Weeks);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::tl().epoch: unit is span-only; use Thread::tl().span
```

## Notes

- See the important callout above — the default unit is `Nanos`.
- Only fixed-duration units are accepted; passing `Weeks`, `Months`, `Years`,
  or `BusinessDays` throws. The throw message references `Thread::tl().span`,
  a suggestion that comes from the shared underlying engine — `Registry`'s
  `tl()` accessor has no `span()` method of its own.
- Requires every bound field to be `Timeline`-kind — a non-`Timeline` field
  throws `"Slot::tl: slot kind must be Timeline"`.

## See Also

- [`RegistryTimelineAccessor::fmt`](fmt.md) / [`iso()`](iso.md): render as
  text instead of a raw epoch count.
- [`Registry::tl`](../registry/tl.md): the entry point this accessor is
  reached from.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="day.html">
    <span class="label">Previous</span>
    <span class="title">day</span>
  </a>
  <a class="next-link" href="floor.html">
    <span class="label">Next</span>
    <span class="title">floor</span>
  </a>
</div>
