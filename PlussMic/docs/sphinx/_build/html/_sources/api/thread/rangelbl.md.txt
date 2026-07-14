# Thread::rangelbl

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::rangelbl() -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `rangelbl` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rangelbl();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rand.html">
    <span class="label">Previous</span>
    <span class="title">rand</span>
  </a>
  <a class="next-link" href="rangelbl_.html">
    <span class="label">Next</span>
    <span class="title">rangelbl_</span>
  </a>
</div>
