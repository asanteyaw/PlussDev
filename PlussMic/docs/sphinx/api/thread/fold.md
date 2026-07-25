# Thread::fold

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::fold(const std::function<Thread(const Thread&)>& fn, ResultShape shape = ResultShape::Reduce) -> Thread
```

## What It Does

Reduces the thread through a folding-style aggregation route.

## Parameters

**fn**
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

**shape**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `fold` to the current thread using explicit `fn, shape` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.fold(other, {});
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flow.html">
    <span class="label">Previous</span>
    <span class="title">flow</span>
  </a>
  <a class="next-link" href="fullmatch.html">
    <span class="label">Next</span>
    <span class="title">fullmatch</span>
  </a>
</div>
