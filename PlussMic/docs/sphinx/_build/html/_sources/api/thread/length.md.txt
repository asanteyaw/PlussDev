# Thread::length

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::length() -> Thread
```

## What It Does

Returns the element count of the current thread.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `length` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.length();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="le.html">
    <span class="label">Previous</span>
    <span class="title">le</span>
  </a>
  <a class="next-link" href="log.html">
    <span class="label">Next</span>
    <span class="title">log</span>
  </a>
</div>
