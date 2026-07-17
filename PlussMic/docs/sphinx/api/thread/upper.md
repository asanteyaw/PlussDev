# Thread::upper

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::upper() -> Thread
```

## What It Does

Converts note-backed text values to uppercase.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `upper` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.upper();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="value.html">
    <span class="label">Next</span>
    <span class="title">value</span>
  </a>
</div>
