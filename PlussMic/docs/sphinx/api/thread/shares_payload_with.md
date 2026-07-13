# Thread::shares_payload_with

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::shares_payload_with(const Thread& other) -> bool
```

## What It Does

Reports whether two threads point at the same underlying payload storage.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.shares_payload_with(other);
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="shift.html">
    <span class="label">Next</span>
    <span class="title">shift</span>
  </a>
</div>
