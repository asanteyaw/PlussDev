<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1b3443edeee3 -->

# Stream::with_meta

[Back to Stream API Reference](../../stream_api_reference)

## Signatures

```cpp
Stream Stream::with_meta(std::string key, std::string value) const
Stream Stream::with_meta(std::initializer_list<std::pair<std::string_view, std::string_view>> items) const
```

## What It Does

Returns a **new** stream with one or more metadata key/value pairs added — the immutable
counterpart to [`with_meta_()`](with_meta_.md). Both overloads **merge** into the existing
metadata map rather than replacing it: pre-existing keys not mentioned in this call are
kept untouched, and a key that already exists is overwritten with the new value.

## Parameters

**key** (`std::string`, single-pair overload)
    The metadata key to set.

**value** (`std::string`, single-pair overload)
    The value to associate with `key`.

**items** (`std::initializer_list<std::pair<std::string_view, std::string_view>>`,
multi-pair overload)
    One or more key/value pairs to set in a single call, e.g.
    `{{"units", "kg"}, {"source", "sensor"}}`.

## Returns

`Stream`
    A new stream, identical payload to the receiver, with `key`/`value` (or every pair in
    `items`) merged into its metadata map.

## Examples

Example 1. Set a single metadata key.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}));

auto out = stream.with_meta("units", "kg");
std::cout << "original has_meta: " << stream.has_meta("units") << "\n";
std::cout << "out has_meta: " << out.has_meta("units") << "\n";
std::cout << "out.meta(\"units\"): " << out.meta("units").value();
```

### Output

```text
original has_meta: 0
out has_meta: 1
out.meta("units"): kg
```

Example 2. Set several keys at once with the initializer-list overload.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta({{"units", "kg"}, {"source", "sensor"}});

std::cout << "count: " << stream.meta().size() << "\n";
std::cout << "units: " << stream.meta("units").value() << "\n";
std::cout << "source: " << stream.meta("source").value();
```

### Output

```text
count: 2
units: kg
source: sensor
```

Example 3. Calling `with_meta()` again merges — it doesn't replace the existing map, and an
overlapping key is overwritten.

```cpp
auto stream = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))
                   .with_meta("units", "kg");

auto merged = stream.with_meta({{"source", "sensor"}});
std::cout << "merged has units (kept): " << merged.has_meta("units") << "\n";

auto overwritten = stream.with_meta("units", "lb");
std::cout << "overwritten units: " << overwritten.meta("units").value();
```

### Output

```text
merged has units (kept): 1
overwritten units: lb
```

## Notes

- Merges into the existing metadata map — a call that mentions only some keys leaves the
  rest of the map intact; see Example 3.
- A key that already exists is overwritten with the new value, not rejected or duplicated.
- Returns a new `Stream`; the receiver's own metadata is unaffected. Use
  [`with_meta_()`](with_meta_.md) to mutate in place instead.

## See Also

- [`Stream::with_meta_`](with_meta_.md): the mutating in-place version of this method.
- [`Stream::has_meta`](has_meta.md) / [`Stream::meta`](meta.md): check for or read back the
  keys this method sets.
- [`Stream::drop_meta`](drop_meta.md): the inverse operation — remove a key instead of
  setting one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_label_.html">
    <span class="label">Previous</span>
    <span class="title">with_label_</span>
  </a>
  <a class="next-link" href="with_meta_.html">
    <span class="label">Next</span>
    <span class="title">with_meta_</span>
  </a>
</div>
