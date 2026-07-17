# Thread::has

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::has(const char* value) -> Thread

Thread::has(const Thread& values) -> Thread

Thread::has(double value) -> Thread

Thread::has(std::string_view value) -> Thread
```

## What It Does

Uses the public contract shown above to inspect, transform, summarize, or combine the current object without changing the rest of its alignment model unexpectedly.

## Parameters

**value**
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillnull(...)`, or `replace(...)`.

**values**
    Value collection supplied to this overload. The ordering must match the corresponding names or positions expected by the method.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `has` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `has` to the current thread using explicit `values` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.has(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Apply `has` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has(10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `has` to the current thread using explicit `value` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.has("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="gt.html">
    <span class="label">Previous</span>
    <span class="title">gt</span>
  </a>
  <a class="next-link" href="has_display_name.html">
    <span class="label">Next</span>
    <span class="title">has_display_name</span>
  </a>
</div>
