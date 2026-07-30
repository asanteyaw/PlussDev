<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 439cb26e3555 -->

# Stream::meta

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream::meta() const -> const std::unordered_map<std::string, std::string>&
Stream::meta(std::string_view key) const -> std::optional<std::string>
```

## What It Does

Two ways to read the stream's free-form metadata map: the no-argument overload returns the
**entire map** by reference; the `key` overload returns a single value as
`std::optional<std::string>` — engaged if `key` is present, `std::nullopt` otherwise.

## Parameters

**key** (`std::string_view`, only for the single-key overload)
    The metadata key to look up.

## Returns

`const std::unordered_map<std::string, std::string>&` (no-argument overload)
    The full metadata map, by reference — no copy is made.

`std::optional<std::string>` (`key` overload)
    The value for `key` if present; `std::nullopt` otherwise.

## Examples

Example 1. Look up a single key that exists.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta("units", "kg");

std::cout << stream.meta("units").value();
```

### Output

```text
kg
```

Example 2. Looking up an absent key returns `std::nullopt`, not a throw.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

std::cout << stream.meta("units").has_value();
```

### Output

```text
0
```

Example 3. The no-argument overload returns the whole map — useful for iterating every
key at once.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta({{"units", "kg"}, {"source", "sensor"}});

std::cout << "count: " << stream.meta().size();
```

### Output

```text
count: 2
```

## Notes

- The single-key overload returns `std::optional<std::string>`, which has no `operator<<`
  — call `.value()` (after checking `.has_value()` or [`has_meta()`](has_meta.md)) to print
  it directly, as in Example 1.
- The no-argument overload returns `const&` — no copy of the map is made just to read it.
- Requires a defined stream; throws `"Stream: undefined handle"` on a default-constructed
  `Stream()`.

## See Also

- [`Stream::has_meta`](has_meta.md): a cheaper existence check when you don't need the
  value itself.
- [`Stream::with_meta`](with_meta.md) / [`Stream::with_meta_`](with_meta_.md): set the
  values this method reads.
- [`Stream::drop_meta`](drop_meta.md) / [`Stream::drop_meta_`](drop_meta_.md): remove a key.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mean.html">
    <span class="label">Previous</span>
    <span class="title">mean</span>
  </a>
  <a class="next-link" href="min.html">
    <span class="label">Next</span>
    <span class="title">min</span>
  </a>
</div>
