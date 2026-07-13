# Thread::corr

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::corr(const Thread& other, ThreadCorrMethod method = ThreadCorrMethod::Pearson) -> double
```

Compute the correlation between two numeric threads.

## Parameters

**other**

Right-hand numeric thread to correlate with the current thread.

**method**

Correlation method.

- `Pearson`: ordinary linear correlation.
- `Spearman`: rank correlation.
- `KendallTau`: rank-concordance correlation.

## Returns

`double`
    Scalar correlation value.

## Notes

Both threads must be numeric and pairwise compatible after dropping invalid or missing pairs.

## Examples

Example 1. Compute Pearson correlation between two numeric threads.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.corr(other, ThreadCorrMethod::Pearson);
```

### Output

```text
0.943119
```

Example 2. Switch to Spearman correlation when you care about rank association.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.corr(other, ThreadCorrMethod::Spearman);
```

### Output

```text
0.948683
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="contains.html">
    <span class="label">Previous</span>
    <span class="title">contains</span>
  </a>
  <a class="next-link" href="corr_with.html">
    <span class="label">Next</span>
    <span class="title">corr_with</span>
  </a>
</div>
