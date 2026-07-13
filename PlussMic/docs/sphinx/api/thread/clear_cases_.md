# Thread::clear_cases_

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::clear_cases_() -> Thread&
```

## What It Does

Updates object-level structure such as metadata, case labels, or validity state while preserving the underlying values and alignment model. Underscore forms mutate in place.

## Parameters

`None`

## Returns

`Thread&`
    Value produced by this call.

## Examples

Example 1. Mutate the current object in place, then inspect the updated state.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.clear_cases_();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clear_cases.html">
    <span class="label">Previous</span>
    <span class="title">clear_cases</span>
  </a>
  <a class="next-link" href="clear_display_name.html">
    <span class="label">Next</span>
    <span class="title">clear_display_name</span>
  </a>
</div>
