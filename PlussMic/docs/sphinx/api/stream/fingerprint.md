<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 5245a91e3b16 -->

# Stream::fingerprint

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::uint64_t Stream::fingerprint() const
```

## What It Does

Returns a 64-bit hash of the stream's contents: its kind and dtype, its length, which entries are present, every present value, and its labels. Equal contents give equal fingerprints however the stream was built, and different contents give different ones, up to the usual 64-bit collision probability. Values under a missing entry do not count, and the metadata is not part of the content.

The cost is one pass over the data. Use it to answer "did this data change?" or "are these two the same data?", for example to decide which downstream reports to rebuild. For a cheap token that notices edits without reading the data, see [`stamp()`](stamp.md).

## Parameters

`None`

## Returns

`std::uint64_t`
    A hash of the contents.

## Examples

Example 1. Equal contents have equal fingerprints, even when separately built; a changed value changes the fingerprint.

```cpp
auto a = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto rebuilt = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto changed = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 31.0}));
std::cout << "rebuilt fingerprint equal: " << (rebuilt.fingerprint() == a.fingerprint()) << "\n";
std::cout << "changed fingerprint equal: " << (changed.fingerprint() == a.fingerprint()) << "\n";
std::cout << "with metadata fingerprint equal: " << (a.with_meta("units", "kg").fingerprint() == a.fingerprint());
```

### Output

```text
rebuilt fingerprint equal: 1
changed fingerprint equal: 0
with metadata fingerprint equal: 1
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="freq.html">
    <span class="label">Next</span>
    <span class="title">freq</span>
  </a>
</div>
