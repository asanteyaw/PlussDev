:orphan:

# Timeline::to_timeindex

[Back to Timeline API Reference](../../timeline_api_reference)

## Signature

```cpp
TimeIndex Timeline::to_timeindex(bool require_sorted = true) const
```

## Promote A Timeline Into An Ordered Index

`to_timeindex()` promotes a one-dimensional `Timeline` from general temporal
storage into a [`TimeIndex`](../../timeindex_api_reference): a temporary
compatibility object intended
for ordered lookup, boundary searches, and time-based indexing. Promotion does
not parse, sort, deduplicate, or change any instant. It validates that the
existing values satisfy the stronger index contract and then returns a
`TimeIndex` carrying those values and the same timezone.

Use this method when temporal values are about to become coordinates rather
than ordinary data. Keep them as a `Timeline` when rank greater than one,
`NaT`, or arbitrary ordering is meaningful. Promotion is therefore an explicit
claim about structure, not a cosmetic type conversion.

## Parameter

| Parameter | Meaning | Default |
| --- | --- | --- |
| `require_sorted` | Require nondecreasing epoch order during promotion | `true` |

Setting `require_sorted` to `false` skips only the monotonicity check. It does
not sort the values, and it does not relax the one-dimensional or no-`NaT`
requirements. An unchecked, unsorted `TimeIndex` can be inspected or sorted,
but ordered search operations do not have meaningful results until its order
has been established.

## Promotion Contract

| Property | Required behavior |
| --- | --- |
| Rank | Exactly one; a scalar `[]` and matrix `[m, n]` are rejected |
| Length | Any nonnegative length, including an empty vector |
| Values | Every position must be a defined instant; `NaT` is rejected |
| Ordering | Nondecreasing by default; equal adjacent instants are allowed |
| Shape | The vector shape `[n]` is retained |
| Timezone | Retained exactly |
| Precision | Exact signed 64-bit epoch nanoseconds are retained |

Nondecreasing order permits duplicates because ordering and uniqueness answer
different questions. Call
[`TimelineIndexAccessor::is_unique`](../timeline-index/is_unique.md)
when uniqueness is also required by the application.

## Example: Sorted Duplicates Are Valid

```cpp
const auto times = pluss::timeline::make({
    "2024-01-01T00:00:00Z",
    "2024-01-01T00:00:00Z",
    "2024-01-03T00:00:00Z",
});
const auto index = times.to_timeindex();

std::cout << "size=" << index.size()
          << " increasing=" << std::boolalpha
          << index.is_monotonic_increasing()
          << " unique=" << index.is_unique() << '\n';
```

### Output

```text
size=3 increasing=true unique=false
```

The conversion preserves both copies of 1 January. It checks `previous <=
current`, not `previous < current`.

## Example: An Empty Axis Is A Valid Ordered Index

```cpp
const auto index = pluss::timeline::epoch({0}).to_timeindex();

std::cout << "size=" << index.size()
          << " increasing=" << std::boolalpha
          << index.is_monotonic_increasing() << '\n';
```

### Output

```text
size=0 increasing=true
```

An empty vector has no pair that violates monotonic order. Notice that the
factory receives shape `{0}`: a scalar has shape `{}` and cannot be promoted.

## Example: Sortedness Is Checked By Default

```cpp
const auto times = pluss::timeline::make({
    "2024-01-03T00:00:00Z",
    "2024-01-01T00:00:00Z",
});

try {
  (void)times.to_timeindex();
} catch (const pluss::Error&) {
  std::cout << "rejected\n";
}

const auto unchecked = times.to_timeindex(false);
std::cout << "increasing=" << std::boolalpha
          << unchecked.is_monotonic_increasing() << '\n';
```

### Output

```text
rejected
increasing=false
```

The second call is not a sorting shortcut. It preserves `[3 January, 1
January]` exactly and records no promise that binary-search operations can
repair that order. Prefer [`Timeline::sort()`](sort.md) before promotion when
an ordered index is the intended result.

## Example: Strided Storage Is Materialized Safely

```cpp
std::array<std::int64_t, 5> backing{
    1'704'067'200'000'000'000LL, 99,
    1'704'153'600'000'000'000LL, 88,
    1'704'240'000'000'000'000LL,
};
const std::array<std::int64_t, 1> shape{3};
const std::array<std::int64_t, 1> strides{2};

const auto times = pluss::Timeline::from_buffer(
    backing.data(), shape, strides, {}, "America/Edmonton");
const auto index = times.to_timeindex();

std::cout << pluss::timeline::print(index.timeline()) << '\n';
```

### Output

```text
Timeline(shape=[3], tz=America/Edmonton)
[ 2023-12-31 17:00:00, 2024-01-01 17:00:00, 2024-01-02 17:00:00 ]
```

The source reads every second entry of `backing`. Promotion copies those
logical values into packed storage because index scans and binary searches
require contiguous ticks. A contiguous receiver can share its reference-counted
tick storage with the new index instead; in both cases, object lifetimes remain
safe and the timezone is retained.

## Example: Rank And `NaT` Remain Hard Invariants

```cpp
const auto with_nat = pluss::timeline::make({"2024-01-01", ""});
const auto scalar = pluss::timeline::date("2024-01-01");
const auto matrix = pluss::timeline::date(
    std::vector<std::string>{"2024-01-01"}, {1, 1});

for (const auto* value : {&with_nat, &scalar, &matrix}) {
  try {
    (void)value->to_timeindex(false);
  } catch (const pluss::Error&) {
    std::cout << "rejected\n";
  }
}
```

### Output

```text
rejected
rejected
rejected
```

`false` cannot turn an absent instant into an index key or collapse an
unsupported rank. Validation completes before a `TimeIndex` is returned.

## Cost And Ownership

Promotion is linear in the number of elements because it validates every value
and, by default, every adjacent ordering relation. A contiguous receiver shares
its immutable, reference-counted tick storage with the result; a strided
receiver incurs one linear materialization. The timezone string is retained by
value. Neither path mutates the receiver.

## See Also

- [`Timeline::sort`](sort.md): establish nondecreasing order before promotion.
- [`Timeline::unique`](unique.md): remove repeated instants when uniqueness is required.
- [Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference.md): canonical ordered lookup and index operations.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="to_note_iso.html"><span class="label">Previous</span><span class="title">to_note_iso</span></a>
  <a class="next-link" href="tz.html"><span class="label">Next</span><span class="title">tz</span></a>
</div>
<!-- pluss-hand-authored -->
