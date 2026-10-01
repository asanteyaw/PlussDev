<!-- pluss-hand-authored -->

# pluss::timeindex::print

[Back to TimeIndex Factory API Reference](../../timeindex_factory_api_reference)

## Signatures

```cpp
std::string pluss::timeindex::print(
    const pluss::TimeIndex& index,
    const pluss::TimelinePrintOptions& options = {})

std::ostream& pluss::timeindex::print(
    std::ostream& stream,
    const pluss::TimeIndex& index,
    const pluss::TimelinePrintOptions& options = {})
```

## Render One Structural Representation

`print()` produces human-readable diagnostic text for the TimeIndex as a
whole. It labels the object as `TimeIndex`, reports its one-dimensional shape
and timezone when requested, and formats visible instants through the same
calendar engine used by Timeline.

The string overload returns the complete representation. The stream overload
writes directly to an existing stream and returns that same stream for
chaining. Neither overload modifies the index. Stream insertion (`stream <<
index`) is the concise default-format equivalent; use `print` when layout or
timestamp formatting must be controlled.

This is presentation, not serialization. The default format shows whole
seconds and truncation can insert an ellipsis. Use `host()` when exact epoch
nanoseconds are the data being exchanged.

## Layout Options

| Field | Meaning | Default |
|---|---|---|
| `format` | Calendar format used for each visible instant | `%Y-%m-%d %H:%M:%S` |
| `edge_items` | Values retained at each side of a truncated axis | `3` |
| `max_cols` | Largest one-dimensional axis shown without truncation | `7` |
| `show_header` | Include the `TimeIndex(...)` header | `true` |
| `show_shape` | Include shape in the header | `true` |
| `show_tz` | Include timezone in the header | `true` |

`max_rows` and `max_slices` are members of the shared option type but do not
affect rank-one TimeIndex output.

## Examples

Example 1. Obtain the default representation as a string.

```cpp
auto index = pluss::timeindex::make({
    "2025-01-01T00:00:00Z", "2025-01-02T00:00:00Z",
    "2025-01-03T00:00:00Z", "2025-01-04T00:00:00Z",
    "2025-01-05T00:00:00Z"});
std::cout << pluss::timeindex::print(index) << "\n";
```

### Output

```text
TimeIndex(shape=[5], tz=UTC)
[ 2025-01-01 00:00:00, 2025-01-02 00:00:00, 2025-01-03 00:00:00, 2025-01-04 00:00:00, 2025-01-05 00:00:00 ]
```

Example 2. Bound a long axis, show only dates, and omit timezone metadata.

```cpp
pluss::TimelinePrintOptions options;
options.format = "%F";
options.max_cols = 3;
options.edge_items = 1;
options.show_tz = false;
std::cout << pluss::timeindex::print(index, options) << "\n";
```

### Output

```text
TimeIndex(shape=[5])
[ 2025-01-01, ..., 2025-01-05 ]
```

Example 3. Write directly to a stream and verify stream identity.

```cpp
std::ostringstream buffer;
auto& returned = pluss::timeindex::print(buffer, index, options);
std::cout << std::boolalpha << (&returned == &buffer) << "\n";
std::cout << buffer.str() << "\n";
```

### Output

```text
true
TimeIndex(shape=[5])
[ 2025-01-01, ..., 2025-01-05 ]
```

Example 4. Remove the structural header for embedding into another report.

```cpp
options.show_header = false;
std::cout << pluss::timeindex::print(index, options) << "\n";
```

### Output

```text
[ 2025-01-01, ..., 2025-01-05 ]
```

## Cost

Formatting currently visits every lane before layout truncation, so runtime is
`O(index.size())` even when only edge values appear. The string overload owns
its returned allocation. The stream overload avoids that final returned string
but still allocates formatted lane text internally.

## See Also

- [`TimelineIndexAccessor` operators](../timeline-index/operators.md)
- [`TimelineIndexAccessor::host`](../timeline-index/host.md)
- [`TimelineIndexAccessor::to_note_iso`](../timeline-index/to_note_iso.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="make.html"><span class="label">Previous</span><span class="title">make</span></a>
  <a class="next-link" href="../../timeindex_factory_api_reference.html"><span class="label">Next</span><span class="title">TimeIndex Factory API Reference</span></a>
</div>
