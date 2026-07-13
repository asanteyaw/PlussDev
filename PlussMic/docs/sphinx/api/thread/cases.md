# Thread::cases

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::cases() -> Thread

Thread::cases(const Thread& labels) -> Thread
```

## What It Does

Fetches one value, member, or slice from the current object without changing the source object.

## Parameters

**labels**
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `cases` to the current thread values and inspect the result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.cases();
```

### Output

```text
[10.0, 20.0, 30.0]
```
Example 2. Apply `cases` to the current thread using explicit `labels` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto other = pluss::table::Thread(pluss::make_series({15.0, 10.0, 35.0}));

auto out = thread.cases(other);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="casefold.html">
    <span class="label">Previous</span>
    <span class="title">casefold</span>
  </a>
  <a class="next-link" href="cases_.html">
    <span class="label">Next</span>
    <span class="title">cases_</span>
  </a>
</div>
