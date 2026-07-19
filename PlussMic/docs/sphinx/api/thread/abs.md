# Thread::abs

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::abs() -> Thread
```

Return the elementwise absolute value of a numeric thread.

`Thread::abs()` is the sign-stripping numeric helper for one-dimensional data. Use it when magnitude matters more than direction, such as absolute error, distance-from-target, or symmetric-threshold preprocessing.

## Parameters

`None`

## Returns

`Thread`
    A numeric thread containing the absolute value of each element.

## See Also

- <a href="clip.html"><code>Thread::clip</code></a>: cap values into a range instead of removing sign.
- <a href="pow.html"><code>Thread::pow</code></a>: apply power transformations after magnitude cleanup when needed.

## Notes

This method is for numeric threads. Text and temporal payloads are not valid inputs.

The result is non-mutating. The current thread is left unchanged.

## Examples

Example 1. Take the absolute value of a numeric thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({-2.5, 0.0, 3.5}));

auto out = scores.abs();
```

### Output

```text
0  2.5
1  0
2  3.5
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="next-link" href="add.html">
    <span class="label">Next</span>
    <span class="title">add</span>
  </a>
</div>
