<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 488794090792 -->

# Stream::drop_meta_

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::drop_meta_(std::string_view key) -> Stream&
```

## What It Does

The mutating, in-place counterpart to [`drop_meta()`](drop_meta.md): removes `key` from the
receiver's metadata map directly and returns a reference to `*this`, rather than allocating
a new `Stream`. Like the immutable version, dropping a nonexistent key is a harmless no-op.

## Parameters

**key** (`std::string_view`)
    The metadata key to remove.

## Returns

`Stream&`
    A reference to `*this`, with `key` now absent from its metadata map — enabling chained
    calls on the same object.

## Examples

Example 1. Mutate a stream's metadata in place and confirm it's the same object.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta("units", "kg");

auto& ref = stream.drop_meta_("units");
std::cout << "same object: " << (&ref == &stream) << "\n";
std::cout << "has_meta after drop_meta_: " << stream.has_meta("units");
```

### Output

```text
same object: 1
has_meta after drop_meta_: 0
```

## Notes

- Returns `*this` by reference, unlike `drop_meta()`'s fresh `Stream` — mutates the
  receiver directly.
- Never throws, whether or not `key` was present beforehand.
- Requires a non-`const` `Stream` to call, since it mutates the receiver.

## See Also

- [`Stream::drop_meta`](drop_meta.md): the immutable version, returning a new `Stream`
  instead of mutating in place.
- [`Stream::with_meta_`](with_meta_.md): the mutating counterpart for setting a key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="drop_meta.html">
    <span class="label">Previous</span>
    <span class="title">drop_meta</span>
  </a>
  <a class="next-link" href="dropnull.html">
    <span class="label">Next</span>
    <span class="title">dropnull</span>
  </a>
</div>
