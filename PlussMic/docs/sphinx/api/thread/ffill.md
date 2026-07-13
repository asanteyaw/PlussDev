# Thread::ffill

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ffill(std::int64_t limit = -1) -> Thread
```

## What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

## Parameters

**limit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `ffill` to the current thread using explicit `limit` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ffill();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="extract.html">
    <span class="label">Previous</span>
    <span class="title">extract</span>
  </a>
  <a class="next-link" href="fillna.html">
    <span class="label">Next</span>
    <span class="title">fillna</span>
  </a>
</div>
