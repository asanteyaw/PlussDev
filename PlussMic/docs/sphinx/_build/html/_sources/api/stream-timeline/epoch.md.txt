<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 37107c90f520 -->

# StreamTimelineAccessor::epoch

[Back to StreamTimelineAccessor API Reference](../../stream_timeline_accessor_api_reference)

## Signature

```cpp
Stream StreamTimelineAccessor::epoch(pluss::table::TimeUnit unit = pluss::table::TimeUnit::Nanos) const
```

## What It Does

Extracts the bound stream's timestamp as a Unix epoch value in `unit`,
replacing it with an `Int64` `Series` stream.

```{important} StreamTimelineAccessor requires the bound stream to be Timeline-kind
Same precondition as every method on this accessor — see
[`StreamTimelineAccessor::isnat`](isnat.md)'s own important callout for the
full explanation and the exact throw message.
```

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

`Stream`
    A new `Int64` `Series` stream of epoch counts in `unit`.

## Examples

Example 1. Default call reports nanoseconds — see the important callout
above.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
auto out = stream.tl().epoch();
std::cout << out.as_series().to_vec<std::int64_t>()[0];
```

### Output

```text
1706692530000000000
```

Example 2. Pass `TimeUnit::Seconds` explicitly for the more commonly
expected scale.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z", "2024-02-29T23:45:00Z"}));
auto out = stream.tl().epoch(pluss::table::TimeUnit::Seconds);
auto v = out.as_series().to_vec<std::int64_t>();
std::cout << v[0] << " " << v[1];
```

### Output

```text
1706692530 1709250300
```

Example 3 (edge case). A span-only unit throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_timeline({"2024-01-31T09:15:30Z"}));
try {
  auto out = stream.tl().epoch(pluss::table::TimeUnit::Weeks);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Slate::tl().epoch: unit is span-only; use Slate::tl().span
```

## Notes

- See the important callouts above — the default unit is `Nanos`.
- Only fixed-duration units are accepted; passing `Weeks`, `Months`, `Years`,
  or `BusinessDays` throws. The throw message references `Slate::tl().span`,
  a suggestion that comes from the shared underlying engine — `Stream`'s
  `tl()` accessor has no `span()` method of its own.

## See Also

- [`StreamTimelineAccessor::fmt`](fmt.md) / [`iso()`](iso.md): render as text
  instead of a raw epoch count.
- [`Stream::tl`](../stream/tl.md): the entry point this accessor is reached from.
- [`CatalogTimelineAccessor::epoch`](../catalog-timeline/epoch.md): the
  whole-catalog equivalent.

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
