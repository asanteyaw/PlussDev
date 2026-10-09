# `pluss::ai::nlp::Doc` Introduction

## What `Doc` Is

`Doc` is the one type every [`Workflow`](../../workflow/Workflow.md)
call produces or consumes: raw text plus whatever annotation layers the
pipeline's own stages added. A `Doc` fresh out of
[`RuleTokenizerStage`](../../tasks/RuleTokenizerStage.md) alone has only
`tokens` filled in; the same `Doc` type, after a full pipeline
(tokenize, sentencize, lemmatize, tag, parse, recognize entities,
classify), carries every one of those layers at once -- `Doc` does not
grow new fields per stage; every stage just fills in fields that already
exist but started empty.

## Lifecycle

A `Doc` is built once per input text, by
[`Workflow::infer`](../../workflow/Workflow/infer.md) or
[`Workflow::learn`](../../workflow/Workflow/learn.md), and is otherwise
immutable from a caller's point of view -- nothing in `core` mutates an
already-returned `Doc` further. Its [`lexicon`](../Lexicon.md) already
points at a fresh, empty `Lexicon` the moment the `Doc` itself is
default-constructed (it is never null unless a caller clears the field
by hand); tokenization fills that `Lexicon` in as a side effect of
observing each token's surface form, not by creating it in the first
place.

## Dependency Structure Needs A Syntax Stage First

`children_of`/`lineage_of`/`branch_of`/`left_edge_of`/`right_edge_of`
all read `Token::chief` (each token's head index) and `Token::role`
(its dependency label). Tokenization alone makes every token its own
head (`chief` equals its own index, `role` stays empty), so no token
has any dependents until [`SyntaxStage`](../../tasks/SyntaxStage.md)
assigns real heads and roles. Calling any of the five on a `Doc` that
never went through a syntax stage returns an empty or degenerate result
rather than throwing; see each method's own page for the exact
no-syntax behavior.

## Related Pages

- [`Token`](../Token.md) -- what fills `tokens`, and the `chief`/`role`
  fields the dependency-structure methods read.
- [`Lexicon`](../Lexicon.md) -- what `lexicon` points to, and how it
  accumulates as `tokens` grows.
- [`Workflow`](../../workflow/Workflow.md) -- the only type that
  ordinarily produces a `Doc`.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="branch_of.html">
    <span class="label">Next</span>
    <span class="title">branch_of</span>
  </a>
</div>
<!-- apiref:nav:end -->
