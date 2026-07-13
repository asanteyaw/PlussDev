# Thread::titlecase

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::titlecase() -> Thread
```

## What It Does

Converts note-backed text values into title case.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `titlecase` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.titlecase();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tally.html">
    <span class="label">Previous</span>
    <span class="title">tally</span>
  </a>
  <a class="next-link" href="tl.html">
    <span class="label">Next</span>
    <span class="title">tl</span>
  </a>
</div>
