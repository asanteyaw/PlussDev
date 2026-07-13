# Thread::kind

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::kind() -> ThreadKind
```

## What It Does

Returns which payload kind this thread currently stores, such as Series, Note, Timeline, or TimeIndex.

## Parameters

`None`

## Returns

`ThreadKind`
    Value produced by this call.

## Examples

Example 1. Inspect the `kind` property of the current object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.kind();
```

### Output

```text
Series
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="isnull.html">
    <span class="label">Previous</span>
    <span class="title">isnull</span>
  </a>
  <a class="next-link" href="kurtosis.html">
    <span class="label">Next</span>
    <span class="title">kurtosis</span>
  </a>
</div>
