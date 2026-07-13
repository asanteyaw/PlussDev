# Thread::weigh

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::weigh(pluss::RangeView<double> weights, WindowStatistic statistic = WindowStatistic::Mean, std::int64_t min_count = 1) -> Thread
```

## What It Does

Computes a weighted trailing window statistic over the thread using an explicit weight vector.

## Parameters

**weights**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**statistic**
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

**min_count**
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

## Returns

`Thread`
    Value produced by this call.

## Examples

Example 1. Apply `weigh` to the current thread using explicit `weights, statistic, min_count` arguments.

```cpp
auto thread = pluss::table::Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = thread.weigh({}, {}, 2);
```

### Output

```text
[10.0, 20.0, 30.0]
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="window.html">
    <span class="label">Next</span>
    <span class="title">window</span>
  </a>
</div>
