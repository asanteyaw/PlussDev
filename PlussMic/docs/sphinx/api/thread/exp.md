# Thread::exp

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::exp() -> Thread
```

## What It Does

Applies the elementwise exponential to a numeric thread.

## Parameters

`None`

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `exp` elementwise and inspect the returned result.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.exp();
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
