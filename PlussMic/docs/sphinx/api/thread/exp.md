# Thread::exp

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::exp() -> Thread
```

Apply the natural exponential function elementwise to a numeric thread.

`exp()` is the thread-level `e^x` surface for series payloads. It is useful in probability, growth, soft transforms, and any workflow where you need exponential recovery after a log-domain transformation.

## Parameters

`None`

## Returns

`Thread`
    Numeric thread containing `exp(x)` for each valid entry.

## See Also

- <a href="log.html"><code>Thread::log</code></a>: inverse natural-log transform.
- <a href="pow.html"><code>Thread::pow</code></a>: more general exponentiation surface.

## Notes

`exp()` is only valid for numeric series payloads.

Integer-like numeric series are promoted internally to a floating type before the exponential is computed.

## Examples

Example 1. Exponentiate a small numeric thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_series({0.0, 1.0, 2.0})).exp();
```

### Output

```text
0  1
1  2.718281828
2  7.389056099
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="extract.html">
    <span class="label">Next</span>
    <span class="title">extract</span>
  </a>
</div>
