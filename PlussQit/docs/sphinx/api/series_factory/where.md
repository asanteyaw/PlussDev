<!-- pluss-hand-authored -->

# pluss::series::where

[Back to Series Factory API Reference](../../series_factory_api_reference)

## Signature

```cpp
Series pluss::series::where(const Series& condition, const Series& a, const Series& b)
Series pluss::series::where(const Series& condition, const Series& a, double b)
Series pluss::series::where(const Series& condition, double a, const Series& b)
Series pluss::series::where(const Series& condition, double a, double b)
```

## What It Builds

Builds one series by choosing, position by position, between two sources: the value from `a` where `condition` is true and the
value from `b` where it is false. It is the free-function form of `a.where(condition, b)`, written with the condition first so
the choice reads from left to right.

When both sources are numbers the result has the shape of `condition` and the dtype `Float64`. Otherwise the dtype follows the series
operand, except that a fractional number chosen against an integer or boolean series produces `Float64`.

## Parameters

**condition** (`const Series&`)

The test. It may have any dtype: a non-zero value is true.

**a**, **b** (`const Series&` or `double`)

The value taken where the condition is true (`a`) and where it is false (`b`). A number is used for every position.

## Returns

`Series`
    A new series. The condition and the sources broadcast to a common shape.

## Errors

Operands whose shapes cannot be broadcast together are refused.

## Examples

Example 1. Two series: `a` where the condition is true, `b` elsewhere.

```cpp
auto cond = pluss::make_series({true, false, true});
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({10.0, 20.0, 30.0});

std::cout << pluss::series::where(cond, a, b);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 20, 3]
```

Example 2. A series and a number.

```cpp
auto cond = pluss::make_series({true, false, true});
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({10.0, 20.0, 30.0});

std::cout << pluss::series::where(cond, a, 0.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 3]
```

Example 3. A number and a series.

```cpp
auto cond = pluss::make_series({true, false, true});
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({10.0, 20.0, 30.0});

std::cout << pluss::series::where(cond, 0.0, b);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[0, 20, 0]
```

Example 4. Two numbers build a new series from the condition alone.

```cpp
auto cond = pluss::make_series({true, false, true});
auto a = pluss::make_series({1.0, 2.0, 3.0});
auto b = pluss::make_series({10.0, 20.0, 30.0});

std::cout << pluss::series::where(cond, 1.0, -1.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, -1, 1]
```

Example 5. The condition may have any dtype: non-zero is true.

```cpp
auto cond = pluss::make_series({2.0, 0.0, 5.0});
auto a = pluss::make_series({1.0, 2.0, 3.0});

std::cout << pluss::series::where(cond, a, 0.0);
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 3]
```

Example 6. All three operands broadcast.

```cpp
auto cond = pluss::make_series({true, false, true});
auto a = pluss::make_series({1.0, 2.0, 3.0});

std::cout << pluss::series::where(cond, a, pluss::make_series({100.0}));
```

### Output

```text
pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 100, 3]
```

## Notes

- To choose between several branches in order, use [`pluss::series::choose`](choose.md).
- The method form is [`Series::where`](../series/where.md).

## See Also

- [`Series::where`](../series/where.md): the method form.
- [`pluss::series::choose`](choose.md): several ordered branches.
