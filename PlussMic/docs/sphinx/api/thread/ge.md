# Thread::ge

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::ge(const char* other) -> Thread

Thread::ge(const Thread& other) -> Thread

Thread::ge(double other) -> Thread

Thread::ge(std::string_view other) -> Thread
```

## What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `ge` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ge("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `ge` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.ge(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 3. Apply `ge` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ge(10.0);
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 4. Apply `ge` to the current thread using explicit `other` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.ge("demo");
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="fullmatch.html">
    <span class="label">Previous</span>
    <span class="title">fullmatch</span>
  </a>
  <a class="next-link" href="gt.html">
    <span class="label">Next</span>
    <span class="title">gt</span>
  </a>
</div>
