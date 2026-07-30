<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b95c6599fc9e -->

# Stream::with_meta_

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream::with_meta_(std::string key, std::string value) -> Stream&
Stream::with_meta_(std::initializer_list<std::pair<std::string_view, std::string_view>> items) -> Stream&
```

## What It Does

The mutating, in-place counterpart to [`with_meta()`](with_meta.md): merges `key`/`value`
(or every pair in `items`) into the receiver's metadata map directly and returns a
reference to `*this`, rather than allocating a new `Stream`. Same merge semantics as the
immutable version — pre-existing keys not mentioned are kept, and an overlapping key is
overwritten.

## Parameters

**key** (`std::string`, single-pair overload)
    The metadata key to set.

**value** (`std::string`, single-pair overload)
    The value to associate with `key`.

**items** (`std::initializer_list<std::pair<std::string_view, std::string_view>>`,
multi-pair overload)
    One or more key/value pairs to set in a single call.

## Returns

`Stream&`
    A reference to `*this`, with `key`/`value` (or every pair in `items`) merged into its
    metadata map — enabling chained calls on the same object.

## Examples

Example 1. Mutate a stream's metadata in place and confirm it's the same object.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto& ref = stream.with_meta_("units", "kg");
std::cout << "same object: " << (&ref == &stream) << "\n";
std::cout << "has_meta: " << stream.has_meta("units");
```

### Output

```text
same object: 1
has_meta: 1
```

Example 2. The multi-pair overload merges in place too.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

stream.with_meta_("units", "kg");
stream.with_meta_({{"source", "sensor"}, {"units", "lb"}});

std::cout << "count: " << stream.meta().size() << "\n";
std::cout << "units (overwritten): " << stream.meta("units").value();
```

### Output

```text
count: 2
units (overwritten): lb
```

## Notes

- Returns `*this` by reference, unlike `with_meta()`'s fresh `Stream` — mutates the
  receiver directly.
- Same merge-not-replace semantics as [`with_meta()`](with_meta.md); see that page's
  Example 3 for the merge/overwrite behavior in detail.
- Requires a non-`const` `Stream` to call, since it mutates the receiver.

## See Also

- [`Stream::with_meta`](with_meta.md): the immutable version, returning a new `Stream`
  instead of mutating in place.
- [`Stream::drop_meta_`](drop_meta_.md): the mutating counterpart for removing a key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_meta.html">
    <span class="label">Previous</span>
    <span class="title">with_meta</span>
  </a>
</div>
