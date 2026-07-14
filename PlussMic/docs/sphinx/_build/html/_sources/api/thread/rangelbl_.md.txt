# Thread::rangelbl_

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::rangelbl_() -> Thread&
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Thread&`
    Value produced by this call.

## Examples

Example 1. Apply `rangelbl_` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rangelbl_();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rangelbl.html">
    <span class="label">Previous</span>
    <span class="title">rangelbl</span>
  </a>
  <a class="next-link" href="rank.html">
    <span class="label">Next</span>
    <span class="title">rank</span>
  </a>
</div>
