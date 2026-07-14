# Thread::sum

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::sum() -> ThreadValue
```

## What It Does

Returns the sum of a numeric thread.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `sum` and inspect the resulting summary value or summary object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sum();
```

### Output

```text
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sub.html">
    <span class="label">Previous</span>
    <span class="title">sub</span>
  </a>
  <a class="next-link" href="tail.html">
    <span class="label">Next</span>
    <span class="title">tail</span>
  </a>
</div>
