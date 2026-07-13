# Thread::se

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::se(ErrorTarget target = ErrorTarget::Mean, const StatisticOptions& options = {}) -> ThreadValue
```

## What It Does

Returns the standard error summary for a numeric thread.

## Parameters

**target**
    Target payload family chosen by this cast overload.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.se({}, {});
```

### Output

```text
10.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="scan.html">
    <span class="label">Previous</span>
    <span class="title">scan</span>
  </a>
  <a class="next-link" href="select.html">
    <span class="label">Next</span>
    <span class="title">select</span>
  </a>
</div>
