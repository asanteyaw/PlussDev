# Thread::sort_values

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::sort_values(bool ascending = true, bool nulls_last = true) -> Thread
```

## What It Does

Reorders thread entries by their values while keeping the payload type unchanged.

## Parameters

**ascending**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**nulls_last**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `sort_values` to the current thread using explicit `ascending, nulls_last` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.sort_values(true, true);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="split.html">
    <span class="label">Next</span>
    <span class="title">split</span>
  </a>
</div>
