# `pluss::tils::env` API Reference

[Back to Core Utilities](../core.md)

The environment family reads process configuration without exposing borrowed
`char*` storage. Text accessors copy the value into an owning `std::string`;
typed readers trim surrounding ASCII whitespace and either parse the complete
token or return the caller's fallback.

These functions read environment state only. They do not modify variables,
cache previous reads, consult configuration files, or reinterpret an empty
present value as an absent variable.

## Choosing a reader

| Operation | Result | Invalid or absent input |
|---|---|---|
| [`get`](../core-env/get.md) | `std::optional<std::string>` | `std::nullopt` |
| [`get_bool`](../core-env/get_bool.md) | `bool` | caller's fallback |
| [`get_double`](../core-env/get_double.md) | `double` | caller's fallback |
| [`get_double_clamped`](../core-env/get_double_clamped.md) | bounded `double` | clamp the parsed value or fallback |
| [`get_int`](../core-env/get_int.md) | `int` | caller's fallback |
| [`get_int_clamped`](../core-env/get_int_clamped.md) | bounded `int` | clamp the parsed value or fallback |
| [`get_or`](../core-env/get_or.md) | `std::string` | caller's fallback |

Use `get()` when absence is meaningful and must remain distinguishable from an
empty string. Use `get_or()` when the application already has a textual
default. Prefer the typed readers over parsing `get()` manually: their token,
overflow, non-finite, and fallback rules are consistent across call sites.

```{toctree}
:maxdepth: 1
:hidden:

../core-env/get
../core-env/get_bool
../core-env/get_double
../core-env/get_double_clamped
../core-env/get_int
../core-env/get_int_clamped
../core-env/get_or
```
