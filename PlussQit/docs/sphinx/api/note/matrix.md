<!-- pluss-hand-authored -->
# Note::matrix

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
std::vector<Note> Note::matrix() const
```

## Project Into Rows And Columns

`matrix()` decomposes a scalar, vector, or matrix into row Notes. A rank-two
receiver produces one rank-one `Note` per row. A rank-one receiver is returned
as the sole row, and a rank-zero receiver is returned as the sole scalar row.
The elements therefore remain in the Note family: dtype, shape, checked typed
access, and ownership are not discarded merely to expose row structure.

The promotion table is exact:

| Receiver | Result |
|---|---|
| rank 0 | one rank-zero Note |
| rank 1, shape `[n]` | one Note with shape `[n]` |
| rank 2, shape `[r, c]` | `r` Notes, each with shape `[c]` |
| rank 3 | rejected |

Only the outer `std::vector` and compact per-row metadata are allocated. Text
rows retain the original packed byte arena; Bool, Int64, and Float64 rows retain
the original aligned lane buffer and advance an offset. Thus numeric row
construction is `O(rows)`, does not copy lanes, and each row remains valid after
the source Note is destroyed. Use `host()` when independent owning standard
containers are required instead.

Empty geometry remains exact because every row is itself a Note. Shape `[2, 0]`
becomes two Notes of shape `[0]`; shape `[0]` becomes one Note of shape `[0]`;
shape `[0, 3]` produces no rows, while the source still records both extents.

## Example 1: Promote A Scalar

```cpp
const auto rows = pluss::note::make("alpha").matrix();
std::cout << "rows=" << rows.size() << " value=" << rows[0].scalar() << '\n';
```

### Output

```text
rows=1 value=alpha
```

## Example 2: Promote A Vector To One Row

```cpp
const auto rows = pluss::note::make({"a", "b"}).matrix();
std::cout << "rows=" << rows.size() << " shape=" << rows[0].size(0)
          << " second=" << rows[0].at(1) << '\n';
```

### Output

```text
rows=1 shape=2 second=b
```

## Example 3: Preserve A Matrix

```cpp
const auto grid = pluss::note::make(
    std::vector<std::string>{
        "r0c0", "r0c1", "r0c2", "r1c0", "r1c1", "r1c2"},
    std::array<std::int64_t, 2>{2, 3}).matrix();

std::cout << "rows=" << grid.size()
          << " row-shape=" << grid[0].size(0)
          << " last=" << grid[1].at(2) << '\n';
```

### Output

```text
rows=2 row-shape=3 last=r1c2
```

## Example 4: Distinguish Two Empty Shapes

```cpp
const auto empty_matrix = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 2>{2, 0}).matrix();
const auto empty_vector = pluss::note::make(
    std::vector<std::string>{},
    std::array<std::int64_t, 1>{0}).matrix();

std::cout << "empty rows=" << empty_matrix.size()
          << " widths=" << empty_matrix[0].numel() << ','
          << empty_matrix[1].numel() << '\n'
          << "empty vector rows=" << empty_vector.size()
          << " width=" << empty_vector[0].numel() << '\n';
```

### Output

```text
empty rows=2 widths=0,0
empty vector rows=1 width=0
```

## Example 5: Rank Three And Absence Reject

```cpp
try {
  (void)pluss::note::make(
      std::vector<std::string>{"x"},
      std::array<std::int64_t, 3>{1, 1, 1}).matrix();
} catch (const pluss::Error&) {
  std::cout << "matrix rank3: rejected\n";
}

try { (void)pluss::Note::undefined().matrix(); }
catch (const pluss::Error&) { std::cout << "matrix undefined: rejected\n"; }
```

### Output

```text
matrix rank3: rejected
matrix undefined: rejected
```

## See Also

- [`vector`](vector.md): flatten any rank to one borrowed sequence.
- [`tensor3`](tensor3.md): extract exact three-dimensional nesting.
- [`host`](host.md): copy lanes into owning standard-library storage.
- [`reshape`](reshape.md): change Note geometry rather than only its C++ projection.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lstrip.html"><span class="label">Previous</span><span class="title">lstrip</span></a>
  <a class="next-link" href="narrow.html"><span class="label">Next</span><span class="title">narrow</span></a>
</div>
