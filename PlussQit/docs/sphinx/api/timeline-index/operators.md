<!-- pluss-hand-authored -->

# Timeline::TimelineIndexAccessor operators

[Back to Timeline Index Accessor API Reference](../../timeline_index_accessor_api_reference)

## Signature

```cpp
std::ostream& operator<<(std::ostream& stream, const Timeline::TimelineIndexAccessor& index)
```

## Stream Insertion

Timeline::TimelineIndexAccessor deliberately has one domain-specific operator: stream insertion.
Arithmetic between temporal values belongs to `Timeline` and `Duration`;
index search, union, and alignment are named operations whose policies cannot
be expressed clearly by punctuation.

`operator<<` writes the accessor's default structural representation. It
includes the one-dimensional shape, timezone, and readable timestamp values,
returns the original stream, and does not mutate or transfer ownership from
the index.

The default representation prints whole seconds. It is diagnostic text, not a
round-trip serialization and not proof of nanosecond equality. Use `host()` for
exact epoch nanoseconds or the underlying Timeline's formatting facilities for
controlled fractional output.

## Examples

Example 1. Write an index directly.

```cpp
auto index = pluss::timeline::make({
    "2025-01-01T00:00:00Z", "2025-01-02T00:00:00Z",
    "2025-01-03T00:00:00Z", "2025-01-04T00:00:00Z",
    "2025-01-05T00:00:00Z"}).index();
std::cout << index << "\n";
```

### Output

```text
Timeline::TimelineIndexAccessor(shape=[5], tz=UTC)
[ 2025-01-01 00:00:00, 2025-01-02 00:00:00, 2025-01-03 00:00:00, 2025-01-04 00:00:00, 2025-01-05 00:00:00 ]
```

Example 2. Stream chaining works because the operator returns the same stream.

```cpp
std::ostringstream out;
auto& returned = (out << index);
returned << "\ncount=" << index.size();
std::cout << std::boolalpha << (&returned == &out) << "\n";
```

### Output

```text
true
```

Example 3. Render the underlying Timeline when layout options are needed;
stream insertion always uses defaults.

```cpp
pluss::TimelinePrintOptions options;
options.format = "%F";
options.max_cols = 3;
options.edge_items = 1;
options.show_tz = false;
std::cout << pluss::timeline::print(index.timeline(), options) << "\n";
```

### Output

```text
Timeline(shape=[5])
[ 2025-01-01, ..., 2025-01-05 ]
```

## Cost

Rendering formats every lane before applying display truncation, so cost is
linear in `size()` even when an ellipsis is shown. Stream insertion avoids the
returned `std::string` object used by the string-returning `print` overload but
still creates formatted lane text internally.

## See Also

- [`pluss::timeline::print`](../timeline_factory/print.md)
- [`Timeline::TimelineIndexAccessor::host`](host.md)
- [`Timeline::TimelineIndexAccessor::timeline`](timeline.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html"><span class="label">Previous</span><span class="title">min</span></a>
  <a class="next-link" href="resample_agg.html"><span class="label">Next</span><span class="title">resample_agg</span></a>
</div>
