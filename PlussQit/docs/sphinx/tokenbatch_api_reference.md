<!-- pluss-hand-authored -->
# TokenBatch API Reference

[Back to PlussQit API Reference](../api_reference.md)

`TokenBatch` is the closed ragged result returned by tokenizers, extractors,
aligners, and other `Note::nlp()` or `Note::regex()` operations whose cells can
produce different numbers of strings. A rectangular `Note` cannot represent
those variable-length rows without padding; `TokenBatch` instead stores one
flat Text or Bytes `Note` plus one Int64 boundary `Note`.

## The Ragged Layout

For `N` logical source cells, [`offsets()`](api/tokenbatch/offsets.md) contains
`N + 1` cumulative boundaries. Source cell `r` owns the half-open token range

```text
[offsets[r], offsets[r + 1])
```

inside [`tokens()`](api/tokenbatch/tokens.md). Equal adjacent boundaries encode
an empty row. The first boundary is always zero, boundaries never decrease,
and the final boundary equals the number of flat tokens. This is a compact,
padding-free representation: token text and row structure each occur exactly
once.

`TokenBatch` is a cheap reference-counted handle. Copying it does not copy token
bytes or offsets. The `tokens()` and `offsets()` accessors return borrowed
`const Note&` references; copy either returned `Note` handle when it must
outlive the particular `TokenBatch` handle used to obtain it.

## State Queries

[`defined()`](api/tokenbatch/defined.md) asks whether the handle owns a ragged
result. [`empty()`](api/tokenbatch/empty.md) asks whether that result contains
zero flat tokens. Those are independent questions: a defined result can be
empty because every source cell emitted zero tokens, and a default-constructed
undefined handle is also empty.

```{toctree}
:hidden:
:maxdepth: 1

api/tokenbatch/defined
api/tokenbatch/empty
api/tokenbatch/offsets
api/tokenbatch/tokens
```

Every displayed result on these pages is emitted by
`examples/CodeN/tokenbatch_codec_reference_demo.cpp`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="note_nlp_accessor_api_reference.html"><span class="label">Previous</span><span class="title">Note NLP Accessor</span></a>
  <a class="next-link" href="api/tokenbatch/defined.html"><span class="label">Next</span><span class="title">defined</span></a>
</div>
