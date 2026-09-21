<!-- pluss-hand-authored -->
# TokenBatch::defined

[Back to TokenBatch API Reference](../../tokenbatch_api_reference)

## Signature

```cpp
bool TokenBatch::defined() const noexcept
```

## Ask Whether A Ragged Result Exists

`defined()` reports whether this handle owns a `TokenBatch` implementation. It
is a handle-state query, not a token-count query and not a statement about the
state of the nested token `Note`.

A default-constructed `TokenBatch` has no implementation and returns `false`.
Every batch returned by a successful NLP or regex operation returns `true`,
including a batch with zero tokens. Propagating an undefined input also returns
a defined batch envelope: its `tokens()` Note is undefined and its canonical
offsets are `[0]`. That representation keeps "the operation ran, but its input
was absent" distinct from "no `TokenBatch` object was ever produced."

The method is constant-time, performs no allocation, never inspects token
bytes, and is `noexcept`. Use it before `tokens()` or `offsets()` when a batch
may have been default constructed; those accessors reject an undefined handle.
Use `empty()` instead when the question is whether there are any flat tokens.

## Example 1: A Default Handle Is Undefined

```cpp
const pluss::TokenBatch batch;
std::cout << std::boolalpha
          << "defined=" << batch.defined()
          << " empty=" << batch.empty() << '\n';
```

### Output

```text
defined=false empty=true
```

Undefined handles are safely queryable; only payload access is invalid.

## Example 2: A Populated Result Is Defined

```cpp
const auto batch =
    pluss::note::make({"red green", "", "blue"}).nlp().tokens();
std::cout << std::boolalpha
          << "defined=" << batch.defined()
          << " empty=" << batch.empty() << '\n';
```

### Output

```text
defined=true empty=false
```

The empty middle source cell does not make the whole batch empty because the
other cells emitted tokens.

## Example 3: Defined Does Not Mean Nonempty

```cpp
const auto batch = pluss::note::make({"", "!!!"}).nlp().tokens();
std::cout << std::boolalpha
          << "defined=" << batch.defined()
          << " empty=" << batch.empty()
          << " tokens-defined=" << batch.tokens().defined() << '\n';
```

### Output

```text
defined=true empty=true tokens-defined=true
```

This is a valid two-row result. Both rows are empty, represented by offsets
`[0, 0, 0]`.

## Example 4: Propagated Absence Has A Defined Envelope

```cpp
const auto batch = pluss::Note::undefined().nlp().tokens();
std::cout << std::boolalpha
          << "batch-defined=" << batch.defined()
          << " tokens-defined=" << batch.tokens().defined()
          << " empty=" << batch.empty() << '\n';
```

### Output

```text
batch-defined=true tokens-defined=false empty=true
```

Check the nested `tokens().defined()` when absence must remain distinguishable
from a present input that simply emitted no tokens.

## See Also

- [`empty`](empty.md): test whether the flat token payload has zero elements.
- [`tokens`](tokens.md): access the flattened payload after checking state.
- [`offsets`](offsets.md): recover each source cell's token range.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="../../tokenbatch_api_reference.html"><span class="label">Previous</span><span class="title">TokenBatch</span></a>
  <a class="next-link" href="empty.html"><span class="label">Next</span><span class="title">empty</span></a>
</div>
