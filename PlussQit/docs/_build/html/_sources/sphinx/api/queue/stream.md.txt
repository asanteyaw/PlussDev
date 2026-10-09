<!-- pluss-hand-authored -->

# Queue operator<<

[Back to Queue API Reference](../../queue_api_reference)

## Signature

```cpp
std::ostream& operator<<(std::ostream& os, const Queue& q);
```

## What It Does

Prints `"Queue<Kind>"` followed directly by the held payload's own
`operator<<` output, with the payload's self-identifying type name stripped
from the front of that output first -- so the kind is never printed twice.
This works uniformly across all four kinds even though the payload types
don't all format themselves the same way:
[`Series`'s own `operator<<`](../../series_api_reference.md) deliberately
qualifies itself with a `pluss::` namespace prefix (`"pluss::Series(..."`),
while `Note`, `Timeline`, and `Duration`'s own `operator<<` do not add any
prefix beyond their own bare type name. `Queue`'s `operator<<` checks for
both forms and strips whichever one is actually present.

## Parameters

**os** (`std::ostream&`)

The stream to write to.

**q** (`const Queue&`)

The `Queue` to print.

## Returns

`os`, for chaining.

## Examples

Example 1. All four kinds, each printed through the same `operator<<`.
None of the four outputs repeats its own type name.

```cpp
pluss::Queue series_q(pluss::make_series({1.0, 2.0}));
pluss::Queue note_q(pluss::make_note({"a", "b"}));
pluss::Queue timeline_q(pluss::timeline::make("1999-04-17T21:15:23Z"));
pluss::Queue duration_q(pluss::duration::hours(2));
std::cout << "series: " << series_q << '\n';
std::cout << "note: " << note_q << '\n';
std::cout << "timeline: " << timeline_q << '\n';
std::cout << "duration: " << duration_q << '\n';
```

### Output

```text
series: Queue<Series>(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 2]
note: Queue<Note>(shape=[2], dim=1, numel=2, encoding=utf8)
["a", "b"]
timeline: Queue<Timeline>(shape=[], tz=UTC)
1999-04-17 21:15:23
duration: Queue<Duration>(shape=[], dim=0, numel=1)
0 days 02:00:00.000000000
```

Each line leads with `Queue<Kind>` exactly once, immediately followed by
the payload's own formatted body -- `Series`'s `pluss::` prefix and the
other three's bare type-name prefix are both recognized and removed.

## See Also

- [`kind`](kind.md): the kind name this prints, available on its own
  without the payload's full formatted body.
