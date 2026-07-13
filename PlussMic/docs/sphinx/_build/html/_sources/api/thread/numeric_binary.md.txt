# Thread::numeric_binary

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::numeric_binary(const Thread& other, ThreadBinaryMathOp op) -> Thread

Thread::numeric_binary(double other, ThreadBinaryMathOp op, bool reverse = false) -> Thread
```

## What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

**reverse**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `numeric_binary` to the current thread using explicit `other, op` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

### Output

```text
[20.0, 30.0, 40.0]
```
Example 2. Apply `numeric_binary` to the current thread using explicit `other, op, reverse` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.numeric_binary(10.0, pluss::table::ThreadBinaryMathOp::Add, false);
```

### Output

```text
[20.0, 30.0, 40.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="pad.html">
    <span class="label">Next</span>
    <span class="title">pad</span>
  </a>
</div>
