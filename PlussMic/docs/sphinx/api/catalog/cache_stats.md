<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e0aa03f65177 -->

# Catalog::cache_stats

[Back to Catalog API Reference](../../catalog_api_reference)

## Signature

```cpp
static std::pair<std::int64_t, std::int64_t> Catalog::cache_stats()
```

## What It Does

Returns how often joins were served from the join cache, as `(hits, misses)`. A hit is a join that found the hashed right-hand side of an earlier join on the same data and keys; a miss is a join that had to build it. The counters run since the process started or since [`clear_cache`](clear_cache.md) was last called, and every catalog shares them. Whether a particular join is cached is up to the library, so read the counters as a description of the cache, not as a count of joins. The cache never changes a join's result.

## Parameters

`None`

## Returns

`std::pair<std::int64_t, std::int64_t>`
    `(hits, misses)`.

## Examples

Example 1. The second join on the same right-hand side is a hit.

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
Catalog::clear_cache();
(void)left.join(right, spec);
auto first = Catalog::cache_stats();
(void)left.join(right, spec);
auto second = Catalog::cache_stats();
std::cout << "after the first join:  hits " << first.first << ", misses " << first.second << "\n";
std::cout << "after the second join: hits " << second.first << ", misses " << second.second << "\n";
```

### Output

```text
after the first join:  hits 0, misses 1
after the second join: hits 1, misses 1
```

The first join builds the right-hand side (one miss). The second join on the same data and keys reuses it (one hit). The joined result is the same either way.

Example 2. After `clear_cache` both counters are zero.

```cpp
using pluss::kindle::Catalog;

Catalog::clear_cache();
auto stats = Catalog::cache_stats();
std::cout << "hits " << stats.first << ", misses " << stats.second << "\n";
```

### Output

```text
hits 1, misses 1
```

Clearing resets the counters together with the cached sides.

## Output

```text
hits 1, misses 1
```

Text keys are not cached, so repeating the join leaves both counters at zero. The counters describe the cache, not the number of joins.

## Output

```text
hits 1, misses 1
```

Both sides are tiny, so the cache is not used and both counters stay at zero. The counters describe the cache, not the number of joins.

## Notes

x

## See Also

x

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bfill.html">
    <span class="label">Previous</span>
    <span class="title">bfill</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
