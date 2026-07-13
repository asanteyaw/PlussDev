# Thread::show

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::show(const display::DisplayOptions& options = {}) -> void

Thread::show(std::ostream& os, const display::DisplayOptions& options = {}) -> void
```

## What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

## Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**os**
    Destination output stream used instead of the default display or print target.

## Returns

`void`
    Performs the side effect and returns no value.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Write the output to a caller-provided stream.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

thread.show();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shift.html">
    <span class="label">Previous</span>
    <span class="title">shift</span>
  </a>
  <a class="next-link" href="size.html">
    <span class="label">Next</span>
    <span class="title">size</span>
  </a>
</div>
