<!-- pluss-hand-authored -->
# TokenBatch::empty

[Back to TokenBatch API Reference](../../tokenbatch_api_reference)

## Signature

```cpp
bool TokenBatch::empty() const noexcept
```

## Ask Whether The Batch Contains Any Tokens

`empty()` returns `true` when the flattened token payload has zero elements.
It answers a payload question across the entire ragged batch; it does not ask
whether any individual source row is empty.

The result is also `true` for a default-constructed undefined handle and for
the canonical batch produced by propagating an undefined input. Consequently,
`empty()` deliberately cannot distinguish these three situations:

| State | `defined()` | `tokens().defined()` | `empty()` |
|---|---:|---:|---:|
| Default handle | `false` | inaccessible | `true` |
| Present input, zero emitted tokens | `true` | `true` | `true` |
| Propagated undefined input | `true` | `false` | `true` |
| At least one emitted token | `true` | `true` | `false` |

Use the three queries together when those states matter. If only work
avoidance matters, `empty()` alone is enough: a `true` result means there is no
token lane to process.

The method is constant-time, allocation-free, and `noexcept`. It does not scan
the offsets or token text.

## Example 1: Empty Rows Do Not Imply An Empty Batch

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
std::cout << std::boolalpha << "empty=" << batch.empty() << '\n';
```

### Output

```text
empty=false
```

The second row is empty, but the batch contains three tokens in total.

## Example 2: Every Row Can Be Empty Without Losing Row Count

```cpp
const auto batch = pluss::note::make({"", "!!!"}).nlp().tokens();
std::cout << std::boolalpha
          << "defined=" << batch.defined()
          << " empty=" << batch.empty()
          << " offsets=[";
const auto offsets = batch.offsets().integers();
for (std::size_t i = 0; i < offsets.size(); ++i)
  std::cout << (i ? ", " : "") << offsets[i];
std::cout << "]\n";
```

### Output

```text
defined=true empty=true offsets=[0, 0, 0]
```

Three boundaries still describe two logical rows. Equal adjacent boundaries
are the ragged representation of a row with no tokens.

## Example 3: An Undefined Handle Is Safe To Test

```cpp
const pluss::TokenBatch batch;
if (batch.empty())
  std::cout << "nothing to visit\n";
```

### Output

```text
nothing to visit
```

No exception is thrown. Payload access would still require `defined()`.

## Example 4: Preserve Absence When It Matters

```cpp
const auto batch = pluss::Note::undefined().nlp().tokens();
std::cout << std::boolalpha
          << "empty=" << batch.empty()
          << " source-present=" << batch.tokens().defined() << '\n';
```

### Output

```text
empty=true source-present=false
```

This differs from Example 2 even though both batches are empty.

## See Also

- [`defined`](defined.md): distinguish a default handle from an owned result.
- [`offsets`](offsets.md): identify which individual rows are empty.
- [`tokens`](tokens.md): access the payload whose cardinality this method tests.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html"><span class="label">Previous</span><span class="title">defined</span></a>
  <a class="next-link" href="offsets.html"><span class="label">Next</span><span class="title">offsets</span></a>
</div>
