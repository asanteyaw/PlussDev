<!-- pluss-hand-authored -->
# TokenBatch::tokens

[Back to TokenBatch API Reference](../../tokenbatch_api_reference)

## Signature

```cpp
const Note& TokenBatch::tokens() const
```

## Access The Flat Ragged Payload

`tokens()` returns the rank-one `Note` containing every emitted token in
row-major source-cell order. It is the value half of `TokenBatch`; the
[`offsets()`](offsets.md) Note supplies the row boundaries.

For normal text operations the payload uses `NoteDType::Text`. The closed
`TokenBatch` contract also permits Bytes so byte-oriented producers can retain
their dtype without converting arbitrary bytes to text. The payload is always
rank one regardless of the source Note's rank because ragged rows cannot be
encoded as a rectangular multidimensional shape. Its length is
`offsets().integers().back()`.

No token content is copied by this accessor. It returns a borrowed
`const Note&` owned by the `TokenBatch` implementation. That reference remains
valid while a handle sharing the implementation remains alive. If independent
lifetime is needed, copy the `Note` value:

```cpp
pluss::Note retained = batch.tokens();
```

The copy is still cheap: `Note` is itself a reference-counted handle, so token
bytes remain shared rather than duplicated.

A defined all-empty batch returns a defined rank-one Text Note with zero
elements. A batch that propagates an undefined input returns an undefined token
Note and canonical offsets `[0]`. Calling `tokens()` on a default-constructed
undefined `TokenBatch` throws `pluss::Error`.

## Example 1: Flat Tokens Preserve Source-Cell Order

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
const auto& tokens = batch.tokens();

std::cout << "dtype=" << pluss::dtype_name(tokens.dtype())
          << " shape=[" << tokens.size(0) << "] values=[";
for (std::int64_t i = 0; i < tokens.numel(); ++i)
  std::cout << (i ? ", " : "") << '"' << tokens.at(i) << '"';
std::cout << "]\n";
```

### Output

```text
dtype=Text shape=[3] values=["red", "green", "blue"]
```

The empty middle row emits no placeholder. Its location survives in the
offsets, not in this flat Note.

## Example 2: Pair Tokens With Their Boundaries

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
const auto bounds = batch.offsets().integers();

for (std::size_t row = 0; row + 1 < bounds.size(); ++row) {
  std::cout << "row " << row << " count="
            << bounds[row + 1] - bounds[row] << '\n';
}
```

### Output

```text
row 0 count=2
row 1 count=0
row 2 count=1
```

Never infer rows from token text; offsets are the authoritative structure.

## Example 3: A Defined Empty Payload

```cpp
const auto batch = pluss::note::make({"", "!!!"}).nlp().tokens();
std::cout << std::boolalpha
          << "batch-defined=" << batch.defined()
          << " tokens-defined=" << batch.tokens().defined()
          << " token-count=" << batch.tokens().numel() << '\n';
```

### Output

```text
batch-defined=true tokens-defined=true token-count=0
```

## Example 4: Propagated Absence

```cpp
const auto batch = pluss::Note::undefined().nlp().tokens();
std::cout << std::boolalpha
          << "batch-defined=" << batch.defined()
          << " tokens-defined=" << batch.tokens().defined() << '\n';
```

### Output

```text
batch-defined=true tokens-defined=false
```

This is not the same state as Example 3.

## Example 5: Retain The Payload Beyond One Handle

```cpp
pluss::Note retained;
{
  const auto batch = pluss::note::make({"red green"}).nlp().tokens();
  retained = batch.tokens();
}
std::cout << retained.at(0) << ' ' << retained.at(1) << '\n';
```

### Output

```text
red green
```

Copying the returned Note handle keeps its storage alive after the local batch
is destroyed.

## Example 6: Undefined Handle Access Is Rejected

```cpp
const pluss::TokenBatch batch;
try {
  (void)batch.tokens();
} catch (const pluss::Error&) {
  std::cout << "default tokens: rejected\n";
}
```

### Output

```text
default tokens: rejected
```

## See Also

- [`offsets`](offsets.md): recover the logical rows of this flat Note.
- [`defined`](defined.md): test whether payload access is legal.
- [`empty`](empty.md): test whether this Note has any token lanes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="offsets.html"><span class="label">Previous</span><span class="title">offsets</span></a>
  <a class="next-link" href="../../tokenbatch_api_reference.html"><span class="label">Next</span><span class="title">TokenBatch</span></a>
</div>
