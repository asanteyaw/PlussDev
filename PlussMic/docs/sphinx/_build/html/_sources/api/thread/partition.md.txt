# Thread::partition

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::partition(std::string_view separator, std::string prefix = "part") -> DataFrame
```

## What It Does

Splits note-backed text values around a separator or boundary rule.

## Parameters

**separator**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**prefix**
    Text prefix inserted ahead of the existing label or value.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.partition("demo", "demo");
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="pad.html">
    <span class="label">Previous</span>
    <span class="title">pad</span>
  </a>
  <a class="next-link" href="percentiles.html">
    <span class="label">Next</span>
    <span class="title">percentiles</span>
  </a>
</div>
