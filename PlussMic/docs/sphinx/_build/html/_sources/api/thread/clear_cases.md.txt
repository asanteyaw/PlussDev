# Thread::clear_cases

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clear_cases() -> Thread
```

## What It Does

Updates object-level structure such as metadata, case labels, or validity state while preserving the underlying values and alignment model. Underscore forms mutate in place.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Return an updated copy after changing structural metadata, cases, or validity state.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_cases();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="chars.html">
    <span class="label">Previous</span>
    <span class="title">chars</span>
  </a>
  <a class="next-link" href="clear_cases_.html">
    <span class="label">Next</span>
    <span class="title">clear_cases_</span>
  </a>
</div>
