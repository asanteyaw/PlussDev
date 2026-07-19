# Thread::div

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::div(const Thread& other) -> Thread

Thread::div(double other) -> Thread
```

Divide a numeric thread by one scalar or by another numeric thread.

`div(...)` is the named thread-level division surface behind `/`-style arithmetic when you want a method in a chain or you want the overload family documented explicitly.

## Parameters

**other**

Right-hand divisor.

- `double`: broadcast one numeric divisor across the whole thread.
- `const Thread&`: divide element by element against another numeric thread of compatible size.

## Returns

`Thread`
    Numeric result thread with elementwise division applied.

## See Also

- <a href="truediv.html"><code>Thread::truediv</code></a>: explicit true-division surface.
- <a href="mul.html"><code>Thread::mul</code></a>: same typed arithmetic model for multiplication.
- <a href="align.html"><code>Thread::align</code></a>: align threads first when you need label-aware preparation before arithmetic.

## Notes

Scalar division broadcasts the divisor across the full thread length.

Thread-to-thread division is positional unless you explicitly align the threads beforehand.

This method is numeric. Use note, timeline, or timeindex accessors for non-numeric payload work.

## Examples

Example 1. Broadcast one scalar divisor across a numeric thread.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = values.div(10.0);
```

### Output

```text
0  1
1  2
2  3
ptype: Series
```

Example 2. Divide one numeric thread by another thread element by element.

```cpp
using namespace pluss::table;

auto left = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto right = Thread(pluss::make_series({2.0, 4.0, 5.0}));

auto out = left.div(right);
```

### Output

```text
0  5
1  5
2  6
ptype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
