# Thread::tail

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::tail(std::int64_t n = 5, bool render = false) -> ThreadView
```

## What It Does

Returns the last `n` entries of the thread.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`ThreadView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.tail(2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
