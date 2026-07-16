# Thread::split

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::split(std::string_view delimiter, std::int64_t limit = -1, std::string prefix = "part") -> DataFrame

Thread::split(std::string_view pattern, const TextOptions& options) -> DataFrame
```

## What It Does

Splits note-backed text values into pieces using the requested delimiter or rule.

## Parameters

**delimiter**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**limit**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**prefix**
    Text prefix inserted ahead of the existing label or value.

**pattern**
    Pattern string used for matching or extraction.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.split("demo", 2, "demo");
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.split("Acc", {});
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sort_values.html">
    <span class="label">Previous</span>
    <span class="title">sort_values</span>
  </a>
  <a class="next-link" href="starts.html">
    <span class="label">Next</span>
    <span class="title">starts</span>
  </a>
</div>
