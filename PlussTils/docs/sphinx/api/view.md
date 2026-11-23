# PlussView

[Back to PlussTils API Reference](../api_reference.md) · [PlussView guide](../manual/view.md)

PlussView is the window that opens when you call `.show()`. It has no API of its own: you call `.show()` on the object you want to look at, and its own reference documents the call.

| To look at | Call | Reference |
|---|---|---|
| A table | `DataFrame::show()` | PlussMic, `dataframe/show` |
| A stack of tables | `Atlas::show()` | PlussMic, `atlas/show` |
| A plot | `Studio::show()` or `flash::show()` | PlussPl, `studio/show` and `flash/show` |

What the window offers (views, formats, themes, sorting, resizing columns and Save As) is in the [PlussView guide](../manual/view.md).
