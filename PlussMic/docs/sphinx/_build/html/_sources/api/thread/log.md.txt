# Thread::log

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::log() -> Thread
```

## What It Does

Applies the elementwise natural logarithm to a numeric thread.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `log` elementwise and inspect the returned result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.log();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="length.html">
    <span class="label">Previous</span>
    <span class="title">length</span>
  </a>
  <a class="next-link" href="lower.html">
    <span class="label">Next</span>
    <span class="title">lower</span>
  </a>
</div>
