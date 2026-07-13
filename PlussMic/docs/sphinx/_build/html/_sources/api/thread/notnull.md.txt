# Thread::notnull

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::notnull() -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `notnull` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.notnull();
```

### Output

```text
[true, false, true]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note.html">
    <span class="label">Previous</span>
    <span class="title">note</span>
  </a>
  <a class="next-link" href="numeric_binary.html">
    <span class="label">Next</span>
    <span class="title">numeric_binary</span>
  </a>
</div>
