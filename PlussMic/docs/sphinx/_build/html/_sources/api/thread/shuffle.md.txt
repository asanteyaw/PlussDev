# Thread::shuffle

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shuffle(std::optional<std::uint64_t> random_state = std::nullopt) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**random_state**
    Optional seed used to make sampling or shuffling reproducible.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `shuffle` to the current thread using explicit `random_state` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.shuffle({});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="show.html">
    <span class="label">Previous</span>
    <span class="title">show</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
