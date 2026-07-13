# Thread::filter

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::filter(const pluss::Series& bool_mask) -> Thread
```

## What It Does

Keeps only the entries where the supplied boolean mask is true.

## Parameters

**bool_mask**
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `filter` to the current thread using explicit `bool_mask` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = thread.filter(mask);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fillna_.html">
    <span class="label">Previous</span>
    <span class="title">fillna_</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
