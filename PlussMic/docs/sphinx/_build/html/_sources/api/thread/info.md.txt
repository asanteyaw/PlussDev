# Thread::info

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::info() -> void

Thread::info(std::ostream& os) -> void
```

## What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

## Parameters

**os**
    Destination output stream used instead of the default display or print target.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info();
```

### Output

```text
The call completes and returns the contract described under Output.
```
Example 2. Write the output to a caller-provided stream.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.info({});
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="indicate.html">
    <span class="label">Previous</span>
    <span class="title">indicate</span>
  </a>
  <a class="next-link" href="info_string.html">
    <span class="label">Next</span>
    <span class="title">info_string</span>
  </a>
</div>
