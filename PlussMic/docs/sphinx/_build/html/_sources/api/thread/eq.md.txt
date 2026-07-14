# Thread::eq

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::eq(const char* other) -> Thread

Thread::eq(const Thread& other) -> Thread

Thread::eq(double other) -> Thread

Thread::eq(std::string_view other) -> Thread
```

Compare the current thread for equality against a scalar literal or another thread.

`eq(...)` is the direct typed equality surface for thread work. It is the method form behind many mask-building operations on one extracted column.

## Parameters

**other**

Right-hand equality target.

Supported overload families include:

- numeric scalars such as `20.0`,
- text scalars such as `"Bola.txt"`,
- another `Thread` for elementwise equality.

## Returns

`Thread`
    Boolean-style result thread with one equality decision per entry.

## See Also

- <a href="compare.html"><code>Thread::compare</code></a>: generalized comparison surface when you want to choose the operator at runtime.
- <a href="../dataframe/eq.html"><code>DataFrame::eq</code></a>: frame-level equality across several features at once.

## Notes

Numeric scalar equality broadcasts one value across the entire thread.

Text scalar equality is the direct route for note-backed threads.

Thread-to-thread equality is positional unless you explicitly align the threads first.

## Examples

Example 1. Broadcast one numeric scalar across a numeric thread.

```cpp
using namespace pluss::table;

auto values = Thread(pluss::make_series({10.0, 20.0, 30.0}));

auto out = values.eq(20.0);
```

### Output

```text
0  false
1  true
2  false
dtype: Series
```

Example 2. Broadcast one text scalar across a note-backed thread.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({"Ada.csv", "Bola.txt", "Chen.csv"}));

auto out = names.eq("Bola.txt");
```

### Output

```text
0  false
1  true
2  false
dtype: Series
```

Example 3. Compare two numeric threads element by element.

```cpp
using namespace pluss::table;

auto left = Thread(pluss::make_series({10.0, 20.0, 30.0}));
auto right = Thread(pluss::make_series({10.0, 0.0, 30.0}));

auto out = left.eq(right);
```

### Output

```text
0  true
1  false
2  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ends.html">
    <span class="label">Previous</span>
    <span class="title">ends</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
