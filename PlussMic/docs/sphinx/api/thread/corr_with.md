# Thread::corr_with

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::corr_with(const Thread& other, ThreadCorrMethod method = ThreadCorrMethod::Pearson, std::int64_t min_count = 1) -> ThreadValue
```

Compute one correlation value and return it as a scalar-like thread value.

## Parameters

**other**

Right-hand numeric thread.

**method**

Correlation method to use.

**min_count**

Minimum number of valid paired observations required before a value is returned.

## Returns

`ThreadValue`
    Scalar-like holder of the computed correlation.

## Examples

Example 1. Return one correlation result in a scalar-like thread value.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.corr_with(other, ThreadCorrMethod::Pearson, 1);
```

### Output

```text
0.943119
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr.html">
    <span class="label">Previous</span>
    <span class="title">corr</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
