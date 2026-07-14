# Thread::has_caselbl

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::has_caselbl() -> bool
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_caselbl();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has.html">
    <span class="label">Previous</span>
    <span class="title">has</span>
  </a>
  <a class="next-link" href="has_display_name.html">
    <span class="label">Next</span>
    <span class="title">has_display_name</span>
  </a>
</div>
