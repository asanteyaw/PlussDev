# `lymph::models::builder::Builder`

`Builder` is a named, in-memory workspace of candidate [`Model`](../Model.md)
objects: register several under stable names, inspect the set, benchmark them all
in one call, and keep the winner.

> See the [builder subfamily](Introduction.md) for what this concern owns.

## Scope And Alias

The implementation lives at the full scope `lymph::models::builder::Builder`, and
the `models` family re-exports it as `lymph::models::Builder`. Both spellings name
the same type:

```cpp
lymph::models::builder::Builder a = lymph::models::builder::build();   // fully explicit
lymph::models::Builder          b = lymph::models::build();            // family shortcut
```

Construction always goes through the free `build(...)` functions — `Builder` has
no public non-default constructor. Pick the explicit form when you want the
reader to see exactly where the type comes from; pick the shortcut to keep
downstream code terse. The method pages below mix the two on purpose.

## Construction

```cpp
// full scope                              // family alias
lymph::models::builder::Builder            lymph::models::Builder

lymph::models::builder::build();           lymph::models::build();
lymph::models::builder::build(
    std::initializer_list<
        std::pair<std::string, Model>>);   lymph::models::build({{"name", model}, ...});
lymph::models::builder::build(
    const std::map<std::string, Model>&);  lymph::models::build(map);
lymph::models::builder::build(
    const std::vector<
        std::pair<std::string, Model>>&);  lymph::models::build(vector);
```

`build()` makes an empty registry; the braced-list, map, and vector overloads
populate it at construction time. `Builder` is move-only — it holds candidate
models and is not meant to be copied.

## State

`std::int64_t size() const`
: How many models are registered.

`bool empty() const`
: `true` while nothing is registered.

`bool benchmarked() const`
: `true` once [`benchmark`](Builder/benchmarked.md) has run and an outcome is
  remembered.

## Methods

| Method | Summary |
|--------|---------|
| [`benchmark`](Builder/benchmark.md) | Cross-validate every held model and remember the outcome. |
| [`benchmarked`](Builder/benchmarked.md) | Whether a comparison outcome is currently remembered. |
| [`best`](Builder/best.md) | The winning candidate from the last benchmark, fitted. |
| [`eject`](Builder/eject.md) | Remove one model by name and return it. |
| [`get`](Builder/get.md) | A reference to a stored model, without removing it. |
| [`has`](Builder/has.md) | Whether a name is registered. |
| [`inject`](Builder/inject.md) | Register one or more named models. |
| [`list`](Builder/list.md) | The first *n* names, as a preview. |
| [`names`](Builder/names.md) | Every registered name, in insertion order. |
| [`report`](Builder/report.md) | A one-row-per-model DataFrame of the registry. |
| [`worst`](Builder/worst.md) | The lowest-ranked candidate from the last benchmark, fitted. |

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 1
:hidden:

Builder/Introduction
Builder/benchmark
Builder/benchmarked
Builder/best
Builder/eject
Builder/get
Builder/has
Builder/inject
Builder/list
Builder/names
Builder/report
Builder/worst
```
<!-- apiref:toctree:end -->

## Changelog

Added in 0.1.
