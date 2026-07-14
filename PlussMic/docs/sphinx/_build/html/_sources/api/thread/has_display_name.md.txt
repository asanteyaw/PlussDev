# Thread::has_display_name

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::has_display_name() -> bool
```

## What It Does

Reports whether the thread currently carries an explicit display name.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has_display_name();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_caselbl.html">
    <span class="label">Previous</span>
    <span class="title">has_caselbl</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
