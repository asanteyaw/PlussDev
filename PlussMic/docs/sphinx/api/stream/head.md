<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 8c660d74cd56 -->

# Stream::head

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::head(std::int64_t n = 5) const
```

## What It Does

Returns a new stream containing the first `n` rows. `n` is required — there is no
zero-argument overload. Requesting more rows than the stream has clamps to the full
stream rather than throwing; `n <= 0` returns an empty stream rather than throwing.

## Parameters

**n** (`std::int64_t`, default `5`)

The number of leading rows to keep.

## Returns

`Stream`
    A new stream containing at most the first `n` rows.

## Examples

Example 1. Take the first two rows.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));

std::cout << stream.head(2);
```

### Output

```text
0  10
1  20
ptype: Series
```

Example 2. `n` larger than the stream's length clamps to the whole stream, and `n <= 0`
returns an empty stream — neither throws.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({1.0, 2.0, 3.0}));

std::cout << "head(100) size: " << stream.head(100).size() << "\n";
std::cout << "head(0) size: " << stream.head(0).size() << "\n";
std::cout << "head(-1) size: " << stream.head(-1).size();
```

### Output

```text
head(100) size: 3
head(0) size: 0
head(-1) size: 0
```

Example 3. Call `head()` with no argument on a stream of eight values; `n` defaults to 5.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0, 60.0, 70.0, 80.0}));

std::cout << stream.head();
```

### Output

```text
0  10
1  20
2  30
3  40
4  50
ptype: Series
```

With no argument the stream keeps its first five entries, positions 0 to 4.

## Notes

- No zero-argument overload — `n` must be supplied explicitly, unlike some `head`-style
  APIs elsewhere that default to a preview count.
- Clamps at both ends rather than throwing — see Example 2.
- Returns a new `Stream`; the receiver is unmodified.

## See Also

- [`Stream::tail`](tail.md): the mirror operation, taking the last `n` rows.
- [`Stream::slice`](slice.md): an arbitrary `[start, stop)` range instead of a fixed count
  from the front.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_meta.html">
    <span class="label">Previous</span>
    <span class="title">has_meta</span>
  </a>
  <a class="next-link" href="invalid.html">
    <span class="label">Next</span>
    <span class="title">invalid</span>
  </a>
</div>
