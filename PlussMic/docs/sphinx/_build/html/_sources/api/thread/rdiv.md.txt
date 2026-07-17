# Thread::rdiv

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rdiv(const Thread& other) -> Thread

Thread::rdiv(double other) -> Thread
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

Example 1. Apply `rdiv` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.rdiv(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `rdiv` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rdiv(10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rank.html">
    <span class="label">Previous</span>
    <span class="title">rank</span>
  </a>
  <a class="next-link" href="repeated.html">
    <span class="label">Next</span>
    <span class="title">repeated</span>
  </a>
</div>
