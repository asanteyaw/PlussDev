# Thread::cast

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::cast(ThreadCastTarget target, const ThreadCastOptions& options = {}) -> Thread

Thread::cast(ThreadCastTarget target, std::optional<pluss::ScalarType> series_dtype) -> Thread
```

Cast the current thread into another payload family or numeric dtype.

`Thread::cast(...)` is the direct typed-column casting surface behind many table-level conversions.

## Parameters

**target**

Requested target payload family such as `Series`, `Note`, `Timeline`, or `TimeIndex`.

**options**

Structured cast policy.

Use this when you need to control details such as:
- numeric dtype selection,
- invalid parse handling,
- other target-specific cast behavior.

**series_dtype**

Optional scalar dtype used when the target is `ThreadCastTarget::Series`.

## Returns

`Thread`
    Cast thread with the requested target family or dtype.

## See Also

- <a href="as_series.html"><code>Thread::as_series</code></a>, <a href="as_note.html"><code>Thread::as_note</code></a>: direct typed access without changing the payload.
- <a href="../dataframe/cast.html"><code>DataFrame::cast</code></a>: cast selected frame features in one call.

## Examples

Example 1. Cast note-backed numeric text into a numeric series by naming the target dtype directly.

```cpp
using namespace pluss::table;
using pluss::ScalarType;

auto out = Thread(pluss::make_note({"1.0", "2.0", "3.0"}))
               .cast(ThreadCastTarget::Series, ScalarType::Float64);
```

### Output

```text
0  1
1  2
2  3
dtype: Series
```

Example 2. Use `ThreadCastOptions` when invalid text should become null instead of throwing.

```cpp
using namespace pluss::table;
using pluss::ScalarType;

ThreadCastOptions options;
options.series_dtype = ScalarType::Float64;
options.invalid = ParseInvalid::Null;

auto out = Thread(pluss::make_note({"4.0", "oops", "6.0"}))
               .cast(ThreadCastTarget::Series, options);
```

### Output

```text
0  4
1  NA
2  6
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cases_.html">
    <span class="label">Previous</span>
    <span class="title">cases_</span>
  </a>
  <a class="next-link" href="chars.html">
    <span class="label">Next</span>
    <span class="title">chars</span>
  </a>
</div>
