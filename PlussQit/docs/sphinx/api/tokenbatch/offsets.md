<!-- pluss-hand-authored -->
# TokenBatch::offsets

[Back to TokenBatch API Reference](../../tokenbatch_api_reference)

## Signature

```cpp
const Note& TokenBatch::offsets() const
```

## Recover Every Ragged Row Boundary

`offsets()` returns the rank-one Int64 `Note` that maps the flattened
[`tokens()`](tokens.md) payload back to logical source cells. If the operation
processed `N` cells, the Note contains `N + 1` cumulative boundaries. Cell
`r` owns the half-open token interval
`[offsets[r], offsets[r + 1])`.

The representation maintains four invariants:

- the Note has `NoteDType::Int64` and rank one
- the first offset is `0`
- offsets are nondecreasing, so every interval is valid
- the final offset equals `tokens().numel()` for a present token payload

Adjacent equal values represent an empty source cell. This preserves every
row boundary without sentinel strings or padding. The source's multidimensional
shape is not stored in `TokenBatch`; cells are represented in the source
Note's row-major flat order. Retain the source shape separately if the ragged
rows must later be mapped back to coordinates.

For an undefined input propagated through a token-producing operation, the
canonical offsets Note is `[0]` and `tokens()` is undefined. For a present
zero-cell input, offsets is also `[0]`, but `tokens()` is a defined empty Note.
Inspect `tokens().defined()` to distinguish those states.

The returned `const Note&` is borrowed from the batch. Access is constant-time
and allocates nothing. Copying the returned Note creates a cheap owning handle
to the same immutable storage. Calling `offsets()` on a default-constructed
undefined `TokenBatch` throws `pluss::Error`.

## Example 1: Inspect The Boundary Note

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
const auto& offsets = batch.offsets();

std::cout << "dtype=" << pluss::dtype_name(offsets.dtype())
          << " shape=[" << offsets.size(0) << "] values=[";
for (std::size_t i = 0; i < offsets.integers().size(); ++i)
  std::cout << (i ? ", " : "") << offsets.integers()[i];
std::cout << "]\n";
```

### Output

```text
dtype=Int64 shape=[4] values=[0, 2, 2, 3]
```

Four boundaries encode three source cells. The repeated `2` marks the empty
middle cell.

## Example 2: Reconstruct Rows Without Copying

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
const auto bounds = batch.offsets().integers();

for (std::size_t row = 0; row + 1 < bounds.size(); ++row) {
  std::cout << "row " << row << ": [";
  for (std::int64_t i = bounds[row]; i < bounds[row + 1]; ++i)
    std::cout << (i == bounds[row] ? "" : ", ")
              << '"' << batch.tokens().at(i) << '"';
  std::cout << "]\n";
}
```

### Output

```text
row 0: ["red", "green"]
row 1: []
row 2: ["blue"]
```

Iteration is linear in the number of rows plus tokens. No nested container is
materialized.

## Example 3: Every Row May Be Empty

```cpp
const auto batch = pluss::note::make({"", "!!!"}).nlp().tokens();
for (const auto value : batch.offsets().integers())
  std::cout << value << ' ';
std::cout << '\n';
```

### Output

```text
0 0 0 
```

The final offset is zero because the flat token count is zero, while the three
boundaries still preserve two rows.

## Example 4: Propagated Absence Uses The Canonical Boundary

```cpp
const auto batch = pluss::Note::undefined().nlp().tokens();
std::cout << std::boolalpha
          << "tokens-defined=" << batch.tokens().defined()
          << " offsets=[" << batch.offsets().integers().front() << "]\n";
```

### Output

```text
tokens-defined=false offsets=[0]
```

## Example 5: Undefined Handle Access Is Rejected

```cpp
const pluss::TokenBatch batch;
try {
  (void)batch.offsets();
} catch (const pluss::Error&) {
  std::cout << "default offsets: rejected\n";
}
```

### Output

```text
default offsets: rejected
```

## See Also

- [`tokens`](tokens.md): the flat payload indexed by these boundaries.
- [`empty`](empty.md): test the final token cardinality in constant time.
- [`defined`](defined.md): guard payload access on optional handles.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="empty.html"><span class="label">Previous</span><span class="title">empty</span></a>
  <a class="next-link" href="tokens.html"><span class="label">Next</span><span class="title">tokens</span></a>
</div>
