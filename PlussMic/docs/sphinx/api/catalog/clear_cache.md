<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2098d6486125 -->

# Catalog::clear_cache

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
static void Catalog::clear_cache()
```

## What It Does

Empties the join cache of hashed right-hand sides, which every catalog shares. Clearing never changes a join's result; it only means the next join on the same data starts cold (and counts as a miss in [`cache_stats`](cache_stats.md)). Use it to start a measurement from zero, or to release the memory a large right-hand side occupied.

## Parameters

`None`

## Returns

`void`

## Examples

Example 1. Clear the cache between measurements.

```cpp
using namespace pluss::kindle::literals;
using pluss::kindle::Catalog;
using pluss::kindle::Stream;

const std::int64_t n = 20000;
std::vector<std::int64_t> left_ids(n), right_ids(n / 2);
std::vector<double> left_values(n), right_values(n / 2);
for (std::int64_t i = 0; i < n; ++i) { left_ids[i] = i % (n / 2 + 100); left_values[i] = static_cast<double>(i); }
for (std::int64_t i = 0; i < n / 2; ++i) { right_ids[i] = i; right_values[i] = 0.5 * static_cast<double>(i); }
Catalog left({"id"_k = Stream(pluss::make_series(left_ids)), "a"_k = Stream(pluss::make_series(left_values))});
Catalog right({"id"_k = Stream(pluss::make_series(right_ids)), "b"_k = Stream(pluss::make_series(right_values))});
pluss::kindle::CatalogJoinSpec spec;
spec.how = pluss::relational::JoinHow::Left;
spec.left_keys = {"id"};
spec.right_keys = {"id"};
(void)left.join(right, spec);
Catalog::clear_cache();
auto after_clear = Catalog::cache_stats();
(void)left.join(right, spec);
auto rebuilt = Catalog::cache_stats();
std::cout << "after clear:  hits " << after_clear.first << ", misses " << after_clear.second << "\n";
std::cout << "next join:    hits " << rebuilt.first << ", misses " << rebuilt.second << "\n";
```

### Output

```text
after clear:  hits 0, misses 1
next join:    hits 0, misses 2
```

Clearing resets the counters and drops the cached side, so the next join is a miss again rather than a hit.

## Notes

x

## See Also

x

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clean.html">
    <span class="label">Previous</span>
    <span class="title">clean</span>
  </a>
  <a class="next-link" href="eloc.html">
    <span class="label">Next</span>
    <span class="title">eloc</span>
  </a>
</div>
