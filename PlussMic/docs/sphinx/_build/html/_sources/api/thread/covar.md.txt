# Thread::covar

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::covar(const Thread& other, const StatisticOptions& options = {}) -> ThreadValue
```

Compute the covariance between two numeric threads.

## Parameters

**other**

Right-hand numeric thread.

**options**

Statistic policy options such as denominator correction and minimum valid pair count.

## Returns

`ThreadValue`
    Scalar-like covariance result.

## Examples

Example 1. Compute covariance between two numeric threads.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.covar(other);
```

### Output

```text
216.667
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count.html">
    <span class="label">Previous</span>
    <span class="title">count</span>
  </a>
  <a class="next-link" href="day.html">
    <span class="label">Next</span>
    <span class="title">day</span>
  </a>
</div>
