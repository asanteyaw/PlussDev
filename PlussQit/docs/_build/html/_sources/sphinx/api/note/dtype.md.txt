<!-- pluss-hand-authored -->
# Note::dtype

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
NoteDType Note::dtype() const
```

## Identify The Stored Lane Type

`dtype()` reports how every cell in a defined `Note` is physically represented.
A Note is homogeneous: one call describes the complete object, regardless of
rank, shape, or element count. The five values are `NoteDType::Text`,
`NoteDType::Bytes`, `NoteDType::Bool`, `NoteDType::Int64`, and
`NoteDType::Float64`.

Use `dtype()` when code must branch across several lane types, serialize a
schema, or preserve a type while constructing another Note. Use `is_text()`,
`is_bool()`, and the other predicates when one specific type is the only
accepted case; they are shorter and, unlike `dtype()`, safely return `false`
for an undefined handle.

The query is constant-time and allocation-free. It does not inspect cells or
materialize host storage. An empty but defined Note still has a dtype. An
undefined Note does not, so calling `dtype()` on it throws `pluss::Error`; test
`defined()` first when absence is possible.

## Example 1: Inventory Every Closed Dtype

```cpp
const auto text = pluss::note::make("alpha");
const auto bytes = pluss::note::bytes(std::string_view{"A\0B", 3});
const auto flags = pluss::note::make(std::vector<bool>{true, false});
const auto counts = pluss::note::make(std::vector<std::int64_t>{2, 5});
const auto scores = pluss::note::make(std::vector<double>{0.25, 0.75});

std::cout << pluss::coden::dtype_name(text.dtype()) << ' '
          << pluss::coden::dtype_name(bytes.dtype()) << ' '
          << pluss::coden::dtype_name(flags.dtype()) << ' '
          << pluss::coden::dtype_name(counts.dtype()) << ' '
          << pluss::coden::dtype_name(scores.dtype()) << '\n';
```

### Output

```text
Text Bytes Bool Int64 Float64
```

`dtype_name()` is useful for diagnostics; control flow should compare the enum
values directly rather than comparing these names.

## Example 2: Shape Does Not Affect Dtype

```cpp
const auto grid = pluss::note::make(
    std::vector<bool>{true, false, false, true},
    std::array<std::int64_t, 2>{2, 2});
std::cout << "dtype=" << pluss::coden::dtype_name(grid.dtype())
          << " dim=" << grid.dim()
          << " shape=[" << grid.size(0) << ", " << grid.size(1) << "]\n";
```

### Output

```text
dtype=Bool dim=2 shape=[2, 2]
```

## Example 3: Undefined Has No Dtype

```cpp
try {
  (void)pluss::Note::undefined().dtype();
} catch (const pluss::Error& error) {
  const std::string message(error.what());
  std::cout << message.substr(0, message.find('\n')) << '\n';
}
```

### Output

```text
Note::dtype(): undefined Note
```

## See Also

- [`Note::defined`](defined.md): distinguish absence from every defined dtype.
- [`Note::host`](host.md): cross into owning host storage while retaining dtype
  metadata.
- [`Note::is_text`](is_text.md), [`Note::is_bool`](is_bool.md),
  [`Note::is_int64`](is_int64.md), and [`Note::is_float64`](is_float64.md):
  single-type predicates.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dim.html"><span class="label">Previous</span><span class="title">dim</span></a>
  <a class="next-link" href="empty.html"><span class="label">Next</span><span class="title">empty</span></a>
</div>
