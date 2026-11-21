<!-- pluss-hand-authored -->

# TimeSeriesAccessor::sort

[Back to TimeSeriesAccessor API Reference](../../shelf_ts_accessor_api_reference)

## Signature

```cpp
Shelf TimeSeriesAccessor::sort(const Shelf& axis, bool ascending = true, bool nulls_last = true) const
```

## What It Does

Reorders the values of the bound shelf by a time axis. `axis` must have exactly one instant per value. The
result is labelled by the sorted axis, so every value keeps its own instant.

## Parameters

**axis** (`const Shelf&`)

The `Timeline` shelf that gives the order. Its length must equal the length of the bound shelf.

**ascending** (`bool`, default `true`)

`true` orders from the earliest instant, `false` from the latest.

**nulls_last** (`bool`, default `true`)

Where `NULL` instants go: at the end when `true`, at the start when `false`.

## Returns

`Shelf`
    A new shelf with the values in axis order, labelled by the sorted axis.

## Examples

Example 1. Put values recorded out of order into time order.

```cpp
using namespace pluss::table;

auto axis = Shelf(pluss::make_timeline({"2024-12-25T00:00:00Z", "2024-03-14T09:30:00Z", "2024-06-01T00:00:00Z"}));
auto values = Shelf(pluss::make_series({30.0, 10.0, 20.0}));

std::cout << values.ts().sort(axis);
```

### Output

```text
2024-03-14 09:30:00  10
2024-06-01 00:00:00  20
2024-12-25 00:00:00  30
ptype: Series
```

Example 2. Newest first.

```cpp
std::cout << values.ts().sort(axis, false);
```

### Output

```text
2024-12-25 00:00:00  30
2024-06-01 00:00:00  20
2024-03-14 09:30:00  10
ptype: Series
```

Example 3. An axis of the wrong length is refused.

```cpp
auto short_axis = Shelf(pluss::make_timeline({"2024-03-14T09:30:00Z", "2024-12-25T00:00:00Z"}));

try {
  auto out = values.ts().sort(short_axis);
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
TimeSeriesAccessor::sort: axis length must match values
```

## See Also

- [`Shelf::ts`](../shelf/ts.md): enters the accessor.
- [`TimeSeriesAccessor::calendar`](calendar.md): break an axis into calendar components.
