# Thread::with_display_name

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::with_display_name(std::string name) -> Thread
```

## What It Does

Attaches a display name to the thread without changing the underlying values.

## Parameters

**name**
    Name used to identify the target object, field, feature, or slot for this overload.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `with_display_name` to the current thread using explicit `name` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.with_display_name("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="window.html">
    <span class="label">Previous</span>
    <span class="title">window</span>
  </a>
  <a class="next-link" href="year.html">
    <span class="label">Next</span>
    <span class="title">year</span>
  </a>
</div>
