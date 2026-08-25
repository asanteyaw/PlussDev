# PlussAI `procedure`

`procedure` is the cross-framework execution and persistence layer in PlussAI.

It coordinates staged execution over artifacts from families such as:

- `classic`
- `nets`
- `nlp`
- `lymph`
- `ice`

## Public Positioning

Teach `procedure` as the family for:

- artifact-driven execution graphs
- reusable staged workflows
- persistence and restore contracts
- family-aware runtime routing

It is not a modeling family by itself. It is the coordinating layer that lets
multiple modeling families participate in one executable procedure.

## Include Surface

```cpp
#include <pluss/ai/procedure.h>
```

## Public Namespace

```cpp
namespace proc = pluss::ai::procedure;
```

## Relationship To Other Families

- persists and restores family artifacts
- records compatibility and execution metadata
- routes work across `classic`, `nlp`, `lymph`, `nets`, and `ice`

## Documentation Status

This handwritten branch is the new public entrypoint for `procedure`.
