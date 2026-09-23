<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 374f8658b90b -->

# CatalogTimelineAccessor::epoch

[Back to CatalogTimelineAccessor API Reference](../../catalog_timeline_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimelineAccessor::epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const
```

## What It Does

Extracts each bound key's timestamp as a Unix epoch value in `unit`, replacing
each `Timeline`- or `TimeIndex`-kind key with an `Int64` `Series` key.

```{important} CatalogTimelineAccessor requires every bound key to be Timeline- or TimeIndex-kind
Same precondition as every method on this accessor — see
[`CatalogTimelineAccessor::isnat`](isnat.md)'s own important callout for the full
explanation and the exact throw message.
```

```{important} The default unit is Nanos, not Seconds
A bare `epoch()` call reports nanoseconds since the Unix epoch, not seconds —
easy to assume otherwise, since seconds is the more familiar epoch unit in
casual use. Pass `TimeUnit::Seconds` explicitly for the more commonly expected
scale.
```

## Parameters

**unit** (`pluss::table::TimeUnit`, defaults to `pluss::table::TimeUnit::Nanos`)
    One of the fixed-duration units: `Nanos`, `Micros`, `Millis`, `Seconds`, or
    `Days`. Passing a span-only unit (`Weeks`, `Months`, `Years`,
    `BusinessDays`) throws.

## Returns

`Catalog`
    A new catalog with every bound key replaced by an `Int64` `Series` key of
    epoch counts in `unit`.

## Examples

Example 1. Default call reports nanoseconds — see the important callout above.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
auto out = ct.tl().epoch();
std::cout << out.keys("ts").as_series().to_vec<std::int64_t>()[0];
```

### Output

```text
1706692530000000000
```

Example 2. Pass `TimeUnit::Seconds` explicitly for the more commonly expected
scale.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"})),
});
auto out = ct.tl().epoch(pluss::table::TimeUnit::Seconds);
auto v = out.keys("ts").as_series().to_vec<std::int64_t>();
std::cout << v[0] << " " << v[1];
```

### Output

```text
1706692530 1709250300
```

Example 3. A span-only unit throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "ts"_k = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"})),
});
try {
  auto out = ct.tl().epoch(pluss::table::TimeUnit::Weeks);
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

- See the important callouts above — the default unit is `Nanos`.
- Only fixed-duration units are accepted; passing `Weeks`, `Months`, `Years`, or
  `BusinessDays` throws. The throw message references `Thread::tl().span`, a
  suggestion that comes from the shared underlying engine — `Catalog`'s `tl()`
  accessor has no `span()` method of its own.

## See Also

- [`CatalogTimelineAccessor::fmt`](fmt.md) / [`iso()`](iso.md): render as text
  instead of a raw epoch count.
- [`Catalog::tl`](../catalog/tl.md): the entry point this accessor is reached from.
- [`RegistryTimelineAccessor::epoch`](../registry-timeline/epoch.md): the
  scribble-model equivalent.

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
