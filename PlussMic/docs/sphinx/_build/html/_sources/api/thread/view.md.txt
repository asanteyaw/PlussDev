# Thread::view

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::view(std::int64_t start, std::int64_t stop, bool render = false) -> ThreadView
```

## What It Does

Returns a smaller preview cut from the current object so you can inspect rows or depth items without carrying the full payload forward.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`ThreadView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.view(0, 2, true);
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="weigh.html">
    <span class="label">Next</span>
    <span class="title">weigh</span>
  </a>
</div>
