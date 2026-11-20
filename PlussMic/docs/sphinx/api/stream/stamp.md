<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: bd286dc1ef2e -->

# Stream::stamp

[Back to Stream API Reference](../../stream_api_reference)

## Signature

```cpp
std::uint64_t Stream::stamp() const
```

## What It Does

Returns a cache token: a 64-bit number that identifies this stored data as of now, not what it contains. It changes whenever the contents can have changed (a different handle, since edits clone, or an in-place edit of the shared storage), so a copy of the stream has the same stamp while a stream rebuilt from equal values has its own.

It costs O(1): the data is never read. It is meant for caches that must notice edits cheaply, such as the join cache, which keys its reusable hashed right-hand sides on stamps. It is valid only while a copy of the stream is kept alive, because it folds in the handle's address. To ask whether two streams hold the same data, use [`fingerprint()`](fingerprint.md).

## Parameters

`None`

## Returns

`std::uint64_t`
    A change token. Compare two stamps for equality; the number has no other meaning.

## Examples

Example 1. A copy shares the stamp; a stream rebuilt from the same values does not.

```cpp
auto a = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
auto copy = a;
auto rebuilt = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));
std::cout << "copy stamp equal: " << (copy.stamp() == a.stamp()) << "\n";
std::cout << "rebuilt stamp equal: " << (rebuilt.stamp() == a.stamp());
```

### Output

```text
copy stamp equal: 1
rebuilt stamp equal: 0
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="slice.html">
    <span class="label">Previous</span>
    <span class="title">slice</span>
  </a>
  <a class="next-link" href="std.html">
    <span class="label">Next</span>
    <span class="title">std</span>
  </a>
</div>
