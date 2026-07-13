# Thread::value

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::value() -> ThreadValue
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

`None`

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `value` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.value();
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="upper.html">
    <span class="label">Previous</span>
    <span class="title">upper</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
