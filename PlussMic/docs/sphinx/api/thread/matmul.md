# Thread::matmul

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::matmul(const Thread& other) -> ThreadValue
```

## What It Does

Computes the dot-product-style matrix multiplication between two numeric threads.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `matmul` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.matmul(other);
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="matches.html">
    <span class="label">Previous</span>
    <span class="title">matches</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
