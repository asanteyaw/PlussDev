# Thread::unique

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::unique() -> bool
```

## What It Does

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.unique();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ts.html">
    <span class="label">Previous</span>
    <span class="title">ts</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
