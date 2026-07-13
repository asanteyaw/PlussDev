# Thread::indicate

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::indicate(std::string_view delimiter = "|", std::string prefix = "") -> DataFrame
```

## What It Does

Returns a boolean indicator thread showing which entries satisfy the requested predicate.

## Parameters

**delimiter**
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

auto out = thread.indicate("demo", "demo");
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hour.html">
    <span class="label">Previous</span>
    <span class="title">hour</span>
  </a>
  <a class="next-link" href="info.html">
    <span class="label">Next</span>
    <span class="title">info</span>
  </a>
</div>
