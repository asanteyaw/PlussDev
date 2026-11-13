<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: fcf1cc7f27b8 -->

# Stream::drop_meta

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream Stream::drop_meta(std::string_view key) const
```

## What It Does

Returns a new stream with `key` removed from its metadata map, leaving every other key and
the payload itself untouched — the immutable counterpart to
[`drop_meta_()`](drop_meta_.md). Calling it with a key that doesn't exist is a harmless
no-op: it returns an equivalent stream rather than throwing.

## Parameters

**key** (`std::string_view`)
    The metadata key to remove.

## Returns

`Stream`
    A new stream, identical payload to the receiver, with `key` absent from its metadata
    map (whether or not it was present beforehand).

## Examples

Example 1. Remove one key, leaving the rest of the metadata map intact.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta({{"units", "kg"}, {"source", "sensor"}});

auto out = stream.drop_meta("units");
std::cout << "out has units: " << out.has_meta("units") << "\n";
std::cout << "out has source: " << out.has_meta("source") << "\n";
std::cout << "stream (receiver) has units: " << stream.has_meta("units");
```

### Output

```text
out has units: 0
out has source: 1
stream (receiver) has units: 1
```

Example 2. Dropping a key that was never set is a harmless no-op.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.drop_meta("nope").meta().size();
```

### Output

```text
0
```

## Notes

- Safe to call with a nonexistent key — never throws; see Example 2.
- Removes exactly one key; the payload, row-label axis, and every other metadata key are
  unaffected.
- Returns a new `Stream`; the receiver's own metadata is unaffected. Use
  [`drop_meta_()`](drop_meta_.md) to mutate in place instead.

## See Also

- [`Stream::drop_meta_`](drop_meta_.md): the mutating in-place version of this method.
- [`Stream::with_meta`](with_meta.md): the inverse operation — set a key instead of
  removing one.
- [`Stream::has_meta`](has_meta.md): check whether a key is present before or after
  dropping it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="distinct.html">
    <span class="label">Previous</span>
    <span class="title">distinct</span>
  </a>
  <a class="next-link" href="drop_meta_.html">
    <span class="label">Next</span>
    <span class="title">drop_meta_</span>
  </a>
</div>
