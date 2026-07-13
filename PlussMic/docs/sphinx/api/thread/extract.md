# Thread::extract

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::extract(std::string_view pattern, pluss::RangeView<std::string_view> names = {}) -> DataFrame
```

## What It Does

Extracts text submatches or text-derived pieces from note-backed values.

## Parameters

**pattern**
    Pattern string used for matching or extraction.

**names**
    Collection of feature, slot, or stream names supplied in positional order.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.extract("Acc", "demo");
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="exp.html">
    <span class="label">Previous</span>
    <span class="title">exp</span>
  </a>
  <a class="next-link" href="ffill.html">
    <span class="label">Next</span>
    <span class="title">ffill</span>
  </a>
</div>
