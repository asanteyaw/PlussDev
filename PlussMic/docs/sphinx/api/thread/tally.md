# Thread::tally

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::tally(std::int64_t top_n = 0, bool dropnull = true, bool render = false) -> DataFrameView
```

## What It Does

Use this when you want a summary view instead of carrying the full input forward unchanged.

## Parameters

**top_n**
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

**dropnull**
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tally(2, true, true);
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
