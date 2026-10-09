# `pluss::ai::nlp::Workflow` Concepts

[Back to `pluss::ai::nlp::Workflow`](../Workflow.md)

## A Pipeline, Not A Dependency Solver

Stages run exactly in vector order. The workflow does not inspect stage types,
reorder them, or insert missing prerequisites. That makes custom stages
predictable, but puts composition responsibility at construction time. A
surface-form matcher can follow tokenization directly; a lemma matcher must
follow lemmatization; syntax produces richer structure when tagging precedes
it.

## Rule State And Learned State

Configuration and learned parameters both belong to stage objects. `learn()`
offers the complete corpus to each stage in order. Rule stages use the default
no-op learning contract, while trainable stages update their own state. The
workflow does not clear a stage before relearning; consult the stage's page for
whether a new call replaces or continues its existing model state.

Training overloads that start with raw text first run the current workflow to
build documents. This is convenient for document classification. For token
tagging or entity learning, pass prepared `Doc` objects containing the gold
annotations expected by the stage.

## Failure Atomicity

Validation that belongs to the workflow, such as an empty corpus or mismatched
label count, happens before stages are asked to learn. Stage-specific failures
may occur after an earlier stage has already updated itself; `learn()` is not a
transaction across heterogeneous stages. Validate stage options before fitting
and persist a known-good workflow when rollback is required.

Inference also stops at the first failing stage and returns no partial `Doc`.
The caller's original input is unaffected because each raw-text overload builds
a new document internally.

## Persistence Identity

The persisted stage name is a compatibility key. Built-in stages are restored
through the built-in registry. A custom stage must keep its `name()` stable,
register a zero-argument factory before loading, and validate its own state
lines. The artifact checksum detects truncation or accidental corruption; it
does not provide encryption or authenticity.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="next-link" href="append_stage.html">
    <span class="label">Next</span>
    <span class="title">append_stage</span>
  </a>
</div>
<!-- apiref:nav:end -->
