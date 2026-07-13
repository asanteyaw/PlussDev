# Thread::compare

[Back to Thread API Reference](../../thread_api_reference)

## Signatures

```cpp
Thread::compare(const char* other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(const pluss::Note& other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(const pluss::Series& other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(const pluss::TimeIndex& other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(const pluss::Timeline& other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(const Thread& other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(double other, ThreadBinaryCompareOp op) -> Thread

Thread::compare(std::string_view other, ThreadBinaryCompareOp op) -> Thread
```

Compare the current thread against a scalar, a native pluss payload, or another thread.

This is the typed comparison surface behind scalar broadcasting and thread-to-thread comparison.

## Parameters

**other**

Right-hand comparison target.

Supported forms include:
- native text scalars such as `"BOLA"` or `std::string_view`,
- numeric scalars such as `20.0`,
- pluss payloads such as `Series`, `Note`, `Timeline`, and `TimeIndex`,
- another `Thread`.

**op**

Comparison operator.

- `Eq`: equal
- `Ne`: not equal
- `Lt`: less than
- `Le`: less than or equal
- `Gt`: greater than
- `Ge`: greater than or equal

## Returns

`Thread`
    Boolean-style result thread.

## Notes

Scalar-like inputs broadcast across the full thread length.

Thread-to-thread comparison requires compatible sizes after any supported broadcast or alignment rules are applied.

## Examples

Example 1. Broadcast one numeric scalar across a numeric thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));

auto out = scores.compare(20.0, ThreadBinaryCompareOp::Ge);
```

### Output

```text
0  false
1  true
2  true
3  true
dtype: Series
```

Example 2. Compare a note-backed thread against one text literal.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({"Ada", "BOLA", "cHeN"}));

auto out = names.compare("BOLA", ThreadBinaryCompareOp::Eq);
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

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Thread(pluss::make_series({10.0, 10.0, 35.0, 45.0}));

auto out = scores.compare(other, ThreadBinaryCompareOp::Ne);
```

### Output

```text
0  false
1  true
2  true
3  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="clip.html">
    <span class="label">Previous</span>
    <span class="title">clip</span>
  </a>
  <a class="next-link" href="compare_rows.html">
    <span class="label">Next</span>
    <span class="title">compare_rows</span>
  </a>
</div>
