# PlussTils API Reference

[Back to PlussTils](index.md)

PlussTils is organized as a catalog, not a dependency ladder. Start with the
family that owns the job you need to perform.

| Family | Use it for | Public entry point | Reference |
|---|---|---|---|
| Core Utilities | Compact language helpers, output, environment access, conversion, and callable dispatch | `<pluss/tils.h>` | [Core Utilities](api/core.md) |
| Loaders | Paths, sampling, splitting, batching, and transfer policy | `pluss::loader` | [Loaders](api/loaders.md) |
| PlussBI | Declarative analytical boards and browser sessions | `<pluss/bi/bi.h>` | [PlussBI](api/bi.md) |
| PlussView | The built-in renderer underneath `.show()` on `DataFrame`, `Atlas`, and plots -- call `.show()`, not this | `<pluss/tils/view/window/canvas.h>` etc. | [PlussView](api/view.md) |

The umbrella `<pluss/tils.h>` is convenient when an application deliberately
uses several shelves. Focused headers keep dependencies and compile time more
explicit when only one family is needed.

```{toctree}
:maxdepth: 2
:hidden:

api/core
api/loaders
api/bi
api/view
```
