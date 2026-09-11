<!-- pluss-hand-authored -->
# Note::unsqueeze

[Back to Note API Reference](../../note_api_reference)

## Signature

```cpp
Note Note::unsqueeze(std::int64_t dim) const
```

## Insert A Size-1 Axis

`unsqueeze(dim)` returns a `Note` with a new axis of length 1 inserted at
position `dim`, raising the rank by one. Reach for it to give a flat list a
leading batch axis, to line two `Note`s up for a shape-sensitive operation,
or to make a `1 × n` row / `n × 1` column out of a plain `n`.

Nothing about the text changes: cells are shared with the source, the
[bytes-vs-UTF-8](is_bytes.md) mode carries over, and only the shape metadata
is rewritten. `expand_dims(dim)` is an **exact synonym** and this page
covers both.

### The `dim` contract

- `dim` may be `0 .. dim()` — inserting at `dim()` appends the new axis at
  the end. A negative `dim` counts from the back with room for the new axis
  (`-1` inserts as the new last axis).
- The result must stay within rank 3; inserting a fourth axis raises
  `inserting an axis would exceed Note max rank 3`.
- An out-of-range `dim` raises `dim out of range`.

## Example 1: A Leading Axis

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.unsqueeze(0)) << "\n";
```

### Output

```text
Note(shape=[1, 6], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c", "d", "e", "f"]
]
```

## Example 2: A Trailing Axis

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.unsqueeze(1)) << "\n";
```

### Output

```text
Note(shape=[6, 1], dim=2, numel=6, encoding=utf8)
[
  ["a"], 
  ["b"], 
  ["c"], 
  ["d"], 
  ["e"], 
  ["f"]
]
```

## Example 3: A Negative Position

```cpp
const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.unsqueeze(-1)) << "\n";   // same as unsqueeze(1)
```

### Output

```text
Note(shape=[6, 1], dim=2, numel=6, encoding=utf8)
[
  ["a"], 
  ["b"], 
  ["c"], 
  ["d"], 
  ["e"], 
  ["f"]
]
```

## Example 4: Scalar To Rank 1, And The Synonym

```cpp
std::cout << pluss::note::print(pluss::make_note("solo").unsqueeze(0)) << "\n";

const auto v = pluss::make_note({"a", "b", "c", "d", "e", "f"});
std::cout << pluss::note::print(v.expand_dims(0)) << "\n";   // identical to unsqueeze(0)
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["solo"]
Note(shape=[1, 6], dim=2, numel=6, encoding=utf8)
[
  ["a", "b", "c", "d", "e", "f"]
]
```

## Example 5: Exceeding The Rank Limit

```cpp
try {
  const auto t = pluss::make_note({{"a", "b"}}).reshape({1, 1, 2});   // rank 3
  (void)t.unsqueeze(0);
} catch (const pluss::Error& e) {
  const std::string m = e.what();
  std::cout << m.substr(0, m.find('\n')) << "\n";
}
```

### Output

```text
inserting an axis would exceed Note max rank 3
```

## See Also

- [`Note::squeeze`](squeeze.md): the inverse — remove a size-1 axis.
- [`Note::reshape`](reshape.md): restate the whole shape when more than one
  axis changes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unquote.html">
    <span class="label">Previous</span>
    <span class="title">unquote</span>
  </a>
  <a class="next-link" href="upper.html">
    <span class="label">Next</span>
    <span class="title">upper</span>
  </a>
</div>
