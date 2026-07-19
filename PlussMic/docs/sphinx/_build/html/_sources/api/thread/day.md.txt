# Thread::day

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::day() -> Thread
```

Extract the day-of-month component from each temporal value.

This is the calendar-field projection for timeline and time-index threads.

Use it when your temporal thread already carries full timestamps but the downstream computation needs just the calendar day.

## Parameters

`None`

## Returns

`Thread`
    Numeric thread containing the day component for each participating temporal entry.

## Notes

`day()` expects temporal payloads such as `Timeline` or `TimeIndex`.

The validity mask is preserved while the calendar field is projected out.

The result is a numeric thread, so you can immediately use it in comparison, grouping, bucketing, or arithmetic pipelines.

## Examples

Example 1. Project day-of-month values from a timeline thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_timeline({
    "2024-01-01T09:10:00Z",
    "2024-02-01T11:00:00Z",
    "2024-03-05T15:30:00Z",
})).day();
```

### Output

```text
0  1
1  1
2  5
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cumsum.html">
    <span class="label">Previous</span>
    <span class="title">cumsum</span>
  </a>
  <a class="next-link" href="defined.html">
    <span class="label">Next</span>
    <span class="title">defined</span>
  </a>
</div>
