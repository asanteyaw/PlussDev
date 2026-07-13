# Thread::cases_

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::cases_(const Thread& labels) -> Thread&
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

## Returns

`Thread&`
    Value produced by this call.

## Examples

Example 1. Apply `cases_` to the current thread using explicit `labels` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.cases_(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cases.html">
    <span class="label">Previous</span>
    <span class="title">cases</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
