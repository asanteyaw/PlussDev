# `pluss::ai::nlp::Workflow` Introduction

## What A Workflow Is

A `Workflow` is a value: a language, and an ordered list of stages. It holds
no documents and no results between calls. Each call takes input, runs the
stages over it, and returns new documents or a report. Two workflows built
from the same stages and language behave identically, which is what makes
saving and reloading one reproducible.

## Calls And Their Inputs

Four calls share the same input shapes, a single string, a list of strings, a
`pluss::Note` of texts, or a list of already-tokenized documents:

- [`infer`](Workflow/infer.md) returns annotated documents.
- [`learn`](Workflow/learn.md) trains the trainable stages from texts or
  documents, with labels for a classifier.
- [`assess`](Workflow/assess.md) runs the pipeline and summarizes the result
  as a [`WorkflowReport`](WorkflowReport.md).
- [`save`](Workflow/save.md) and [`load`](Workflow/load.md) persist the stage
  list and every stage's learned state.

[`append_stage`](Workflow/append_stage.md) is the only call that changes the
pipeline itself.

## Order Matters

Stages run in list order over each document, and a later stage can rely on
everything an earlier stage wrote. Put tokenization first, sentence detection
before anything that needs sentences, and tagging before syntax.

## Related Pages

- [`Workflow`](Workflow.md) -- the full method table.
- [`WorkflowReport`](WorkflowReport.md) -- what `assess` returns.
