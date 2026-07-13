# Thread::btw

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::btw(const Thread& lo, const Thread& hi, bool include_left, bool include_right) -> Thread

Thread::btw(const Thread& lo, const Thread& hi, bool inclusive = true) -> Thread

Thread::btw(double a, double b, bool include_left, bool include_right) -> Thread

Thread::btw(double a, double b, bool inclusive = false) -> Thread

Thread::btw(std::string_view lo, std::string_view hi, bool include_left, bool include_right) -> Thread

Thread::btw(std::string_view lo, std::string_view hi, bool inclusive = true) -> Thread
```

Return whether each element falls between two bounds.

`Thread::btw(...)` is the interval-membership surface on `Thread`. It supports scalar numeric bounds, scalar temporal/string bounds, and rowwise lower/upper threads. This makes it useful for grading bands, date windows, threshold rules, and per-row tolerance envelopes.

## Parameters

**a**

Lower scalar numeric bound for the numeric-scalar overloads.

**b**

Upper scalar numeric bound for the numeric-scalar overloads.

**lo**

Lower bound.

- In scalar temporal/text overloads, this is one lower literal bound.
- In rowwise overloads, this is the lower-bound thread used element by element.

**hi**

Upper bound.

- In scalar temporal/text overloads, this is one upper literal bound.
- In rowwise overloads, this is the upper-bound thread used element by element.

**inclusive**

Shared boundary-closure shorthand.

- `true`: both boundaries are closed.
- `false`: both boundaries are open.

Use this when both sides should share the same closure rule.

**include_left**

Whether the lower boundary is included.

- `true`: the lower boundary is closed.
- `false`: the lower boundary is open.

**include_right**

Whether the upper boundary is included.

- `true`: the upper boundary is closed.
- `false`: the upper boundary is open.

## Returns

`Thread`
    A boolean thread containing one interval-membership result per element.

## See Also

- <a href="#ge"><code>Thread::ge</code></a> and <a href="#le"><code>Thread::le</code></a>: spell out the two comparisons manually when you want the lower and upper tests separately.
- <a href="../dataframe/filter.html"><code>DataFrame::filter</code></a>: use the boolean result as a row mask inside a frame workflow.

## Notes

The overload family supports three bound styles:

- numeric scalar bounds,
- temporal/text scalar bounds,
- rowwise lower/upper threads.

When you use the two-boolean form, `include_left` and `include_right` independently control whether each boundary is closed or open.

The one-boolean `inclusive` shorthand differs by overload family in the current API. The examples below show the actual behavior for each family, which is why this method deserves more careful reading than simpler comparison helpers.

## Examples

Example 1. Use shared inclusion for scalar numeric bounds.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.btw(20.0, 40.0, true);
```

### Output

```text
0  false
1  true
2  true
3  true
4  false
dtype: Series
```

Example 2. Use asymmetric scalar numeric boundaries.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

auto out = scores.btw(20.0, 40.0, false, true);
```

### Output

```text
0  false
1  false
2  true
3  true
4  false
dtype: Series
```

Example 3. Use shared inclusion for scalar temporal bounds.

```cpp
using namespace pluss::table;

auto dates = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
    "2024-01-03T00:00:00Z",
}));

auto out = dates.btw("2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", true);
```

### Output

```text
0  true
1  true
2  false
dtype: Series
```

Example 4. Use asymmetric scalar temporal boundaries.

```cpp
using namespace pluss::table;

auto dates = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z",
    "2024-01-02T00:00:00Z",
    "2024-01-03T00:00:00Z",
}));

auto out = dates.btw("2024-01-01T00:00:00Z", "2024-01-03T00:00:00Z", false, true);
```

### Output

```text
0  false
1  true
2  true
dtype: Series
```

Example 5. Use rowwise lower and upper threads with shared inclusion.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto lower = Thread(pluss::make_series({10.0, 20.0, 35.0, 35.0, 45.0}));
auto upper = Thread(pluss::make_series({15.0, 25.0, 35.0, 45.0, 50.0}));

auto out = scores.btw(lower, upper, true);
```

### Output

```text
0  true
1  true
2  false
3  true
4  true
dtype: Series
```

Example 6. Use rowwise lower and upper threads with asymmetric boundaries.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto lower = Thread(pluss::make_series({10.0, 20.0, 35.0, 35.0, 45.0}));
auto upper = Thread(pluss::make_series({15.0, 25.0, 35.0, 45.0, 50.0}));

auto out = scores.btw(lower, upper, false, true);
```

### Output

```text
0  false
1  false
2  false
3  true
4  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bounded.html">
    <span class="label">Previous</span>
    <span class="title">bounded</span>
  </a>
  <a class="next-link" href="bucketize.html">
    <span class="label">Next</span>
    <span class="title">bucketize</span>
  </a>
</div>
