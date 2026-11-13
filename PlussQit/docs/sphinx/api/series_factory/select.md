<!-- pluss-hand-authored -->

# pluss::series::select

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::select(conditions,
                             choices,
                             const Series& default_value)

Series pluss::series::select({conditions...},
                             {choices...},
                             const Series& default_value)

Series pluss::series::select(condition,
                             choice,
                             const Series& default_value)

Series pluss::series::select(..., numeric_default)
Series pluss::series::select(..., bool_default)
```

## What It Builds

`select(...)` builds one output `Series` from several ordered branches.

The usage model is:

1. start from `default_value`
2. walk the condition/choice pairs in order
3. the first true condition for each position wins
4. keep the default wherever no condition wins

So this is the multi-branch constructor for "if / else if / else" style data
assembly.

## Overload Map

**Parallel condition/choice lists**

The main overload takes two parallel lists plus a default.

**Initializer-list spelling**

The most convenient public form for handwritten examples and ordinary call
sites.

**Single-condition shortcut**

Useful when the real job is just one masked branch over a fallback.

**Scalar default overloads**

Numeric and boolean defaults are converted into scalar-like `Series` values and
broadcast from there.

## Parameters

**conditions**

A list of boolean masks.

Rules that matter in practice:

- the list must not be empty
- every condition must be defined
- every condition must have `Bool` dtype
- the condition count must match the choice count

`select(...)` does not infer boolean masks from numeric data. If the source
data is numeric, build a real boolean mask first.

**choices**

A list of payload `Series` values parallel to `conditions`.

Each choice is used where its matching condition is true and no earlier
condition already won.

Choices must be defined. They may be full-size or scalar-like, as long as the
branch assembly can broadcast them meaningfully.

**default_value**

The fallback branch.

If no condition wins at a position, that position keeps `default_value`.

This can be:

- a full `Series`
- a scalar-like `Series`
- a numeric scalar on the numeric-default overloads
- a boolean scalar on the boolean-default overload

## Returns

`Series`
    A new `Series` assembled from the first winning branch at each position.

The result dtype follows the underlying branch-selection promotion rules, so
mixed branch dtypes can widen the output.

## Examples

Example 1. The single-branch shortcut behaves like a masked fallback.

```cpp
std::cout << pluss::series::select(
    pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool),
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    -1.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[10, -1, 30, -1]
```

Example 2. When conditions overlap, the first matching branch wins.

```cpp
std::cout << pluss::series::select(
    {
        pluss::make_series({true, true, false, false}, pluss::ScalarType::Bool),
        pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool),
    },
    {
        pluss::make_series({100.0, 100.0, 100.0, 100.0}),
        pluss::make_series({200.0, 200.0, 200.0, 200.0}),
    },
    0.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[100, 100, 200, 0]
```

Example 3. The fallback can be a full `Series`.

```cpp
std::cout << pluss::series::select(
    {
        pluss::make_series({false, true, false, false}, pluss::ScalarType::Bool),
        pluss::make_series({false, false, true, false}, pluss::ScalarType::Bool),
    },
    {
        pluss::make_series({10.0, 10.0, 10.0, 10.0}),
        pluss::make_series({20.0, 20.0, 20.0, 20.0}),
    },
    pluss::make_series({-1.0, -2.0, -3.0, -4.0}));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[-1, 10, 20, -4]
```

Example 4. Scalar-like branch choices broadcast cleanly.

```cpp
std::cout << pluss::series::select(
    {
        pluss::make_series({false, true, false, true}, pluss::ScalarType::Bool),
    },
    {
        pluss::make_series(9.0),
    },
    pluss::make_series({1.0, 2.0, 3.0, 4.0}));
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 9, 3, 9]
```

Example 5. Pure boolean branches stay boolean.

```cpp
std::cout << pluss::series::select(
    {
        pluss::make_series({true, false, false, true}, pluss::ScalarType::Bool),
        pluss::make_series({false, true, false, false}, pluss::ScalarType::Bool),
    },
    {
        pluss::make_series({true, true, true, true}, pluss::ScalarType::Bool),
        pluss::make_series({false, false, false, false}, pluss::ScalarType::Bool),
    },
    true);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=bool, platform=cpu, numel=4, contiguous=true)
data=[true, false, true, true]
```

Example 6. The single-`Series` overload is a convenience wrapper around one
branch.

```cpp
std::cout << pluss::series::select(
    pluss::make_series({false, true, false, false}, pluss::ScalarType::Bool),
    pluss::make_series(5.0),
    1.0);
```

### Output

```text
pluss::Series(shape=[4], strides=[1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[1, 5, 1, 1]
```

Example 7. Condition and choice counts must match exactly.

```cpp
try {
  std::cout << pluss::series::select(
      {
          pluss::make_series({true, false}, pluss::ScalarType::Bool),
          pluss::make_series({false, true}, pluss::ScalarType::Bool),
      },
      {
          pluss::make_series({1.0, 1.0}),
      },
      0.0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::series::select: condition/choice count mismatch
```

Example 8. Numeric conditions are rejected instead of being treated as masks.

```cpp
try {
  std::cout << pluss::series::select(
      pluss::make_series({1.0, 0.0, 1.0}),
      pluss::make_series({10.0, 20.0, 30.0}),
      -1.0) << "\n";
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
pluss::series::select: conditions must use Bool dtype
```

## Gotchas And Failure Modes

- `select(...)` is first-match-wins, not last-match-wins.
- It does not auto-cast numeric conditions.
- Scalar defaults are supported directly, but scalar choices should be supplied
  as scalar-like `Series` values such as `pluss::make_series(9.0)`.
- Broadcasting still has to make sense across the branches.

## Summary

Use `pluss::series::select(...)` when you need ordered multi-branch
construction rather than one binary `where(...)` call.

- keep masks boolean
- remember that earlier branches have priority
- use scalar-like `Series` values when you want one branch to broadcast

## See Also

- [`pluss::series::print`](print.md): render the selected result explicitly.
- [`Series::where`](../series/where.md): binary branch selection on an existing
  `Series`.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="print.html">
    <span class="label">Previous</span>
    <span class="title">print</span>
  </a>
  <a class="next-link" href="ones.html">
    <span class="label">Next</span>
    <span class="title">ones</span>
  </a>
</div>
