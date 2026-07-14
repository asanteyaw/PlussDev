# Thread::rsub

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::rsub(const Thread& other) -> Thread

Thread::rsub(double other) -> Thread
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

Example 1. Apply `rsub` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.rsub(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `rsub` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.rsub(10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rstrip.html">
    <span class="label">Previous</span>
    <span class="title">rstrip</span>
  </a>
  <a class="next-link" href="rtruediv.html">
    <span class="label">Next</span>
    <span class="title">rtruediv</span>
  </a>
</div>
