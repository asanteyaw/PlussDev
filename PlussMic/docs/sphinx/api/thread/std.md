# Thread::std

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::std(bool unbiased = true) -> ThreadValue

Thread::std(const StatisticOptions& options) -> ThreadValue
```

## What It Does

Returns the standard deviation of a numeric thread.

## Parameters

**unbiased**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`ThreadValue`
    Value produced by this call.

## Examples

Example 1. Compute `std` and inspect the resulting summary value or summary object.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

### Output

```text
20.0
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.std(true);
```

### Output

```text
20.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="starts.html">
    <span class="label">Previous</span>
    <span class="title">starts</span>
  </a>
  <a class="next-link" href="strip.html">
    <span class="label">Next</span>
    <span class="title">strip</span>
  </a>
</div>
