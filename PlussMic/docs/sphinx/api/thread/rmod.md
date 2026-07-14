# Thread::rmod

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rmod(const Thread& other) -> Thread

Thread::rmod(double other) -> Thread
```

## What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `rmod` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.rmod(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `rmod` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rmod(10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rfloordiv.html">
    <span class="label">Previous</span>
    <span class="title">rfloordiv</span>
  </a>
  <a class="next-link" href="rmul.html">
    <span class="label">Next</span>
    <span class="title">rmul</span>
  </a>
</div>
