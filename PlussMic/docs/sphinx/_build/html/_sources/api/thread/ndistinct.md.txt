# Thread::ndistinct

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ndistinct(bool dropnull = true) -> ThreadValue
```

## What It Does

Counts distinct observed values in the thread.

## Parameters

**dropnull**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `ndistinct` and inspect the scalar-like thread result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ndistinct(true);
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ncases.html">
    <span class="label">Previous</span>
    <span class="title">ncases</span>
  </a>
  <a class="next-link" href="normalize.html">
    <span class="label">Next</span>
    <span class="title">normalize</span>
  </a>
</div>
