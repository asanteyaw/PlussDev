<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 2cdd4356908e -->

# Stream::map

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
Stream::map(const std::function<Stream(const Stream&)>& fn) const -> Stream
```

## What It Does

Applies a stream-to-stream function to one stream and requires the result to stay defined
and length-preserving.

This is the primitive that `Catalog::transform()` and `Catalog::keys(...).map()` delegate
to.

## Notes

- The function must return a defined stream.
- The function must preserve stream length.

## See Also

- [`Catalog::map`](../catalog/map.md): whole-catalog alias for `transform`.
- [`Catalog::transform`](../catalog/transform.md): catalog-level coordinator built on this.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="log.html">
    <span class="label">Previous</span>
    <span class="title">log</span>
  </a>
  <a class="next-link" href="max.html">
    <span class="label">Next</span>
    <span class="title">max</span>
  </a>
</div>
