# PlussAI `nlp`

`nlp` is the public namespace for PlussAI's text and language-processing framework.

- Use `pluss::ai::nlp` in public examples and user code.
- Internally, this family is implemented by `pluss::ai::nodeb`.

## What Lives Here

- `Doc` and document-oriented workflow types
- tokenization and task helpers
- lexical vectorizers and hashing/vector mapping
- embedding and encoding utilities
- text runtimes, registries, resources, and NLP persistence helpers

## Public Positioning

Teach `nlp` as the direct public family for:

- document and text workflows
- vectorization and embedding preparation
- NLP-specific training/inference pipelines
- text artifacts that may later feed `classic`, `lymph`, or `ice`

Do not teach `nodeb` in public prose. That name is internal.

## Include Surface

```cpp
#include <pluss/ai/nlp.h>
```

## Public Namespace

```cpp
namespace nlp = pluss::ai::nlp;
```

## Relationship To Other Families

- `nlp` can produce `classic`-compatible batches and sparse features
- `ice` contains higher-level NLP model surfaces that can consume `nlp` artifacts
- `procedure` can persist and route NLP workflows and documents

## Documentation Status

The generated NodeB-facing doc story is retired. This handwritten branch is now the
public API entrypoint for the NLP family.
