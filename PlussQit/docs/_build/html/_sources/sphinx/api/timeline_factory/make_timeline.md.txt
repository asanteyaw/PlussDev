<!-- pluss-hand-authored -->
# pluss::make_timeline

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signature

```cpp
Timeline pluss::make_timeline(text_input values)
```

## Flat Shortcut For `pluss::timeline::make`

`pluss::make_timeline()` is a permanent public alias of
`pluss::timeline::make()`. Every overload forwards directly to the namespaced
factory, so parsing, shape inference, timezone handling, nanosecond precision,
ownership, and failures are identical.

Use whichever spelling makes the construction site clearer. This page is
intentionally brief; the complete accepted grammar, overload inventory, edge
behavior, and failure modes are taught on [`timeline::make`](make.md).

## Example 1: Scalar Shortcut

```cpp
const auto value = pluss::make_timeline(
    "2024-02-29T23:30:45.123456789-07:00");
std::cout << pluss::note::print(
    pluss::timeline::format(value, "%FT%T.%f%z")) << '\n';
```

### Output

```text
Note(shape=[], dim=0, numel=1, encoding=utf8)
"2024-03-01T06:30:45.123456789+00:00"
```

## Example 2: Flat Runtime Data And Shape

```cpp
const std::vector<std::string> flat{
    "2024-01-01", "2024-01-02", "2024-01-03", "2024-01-04"};
const auto matrix = pluss::make_timeline(flat, {2, 2});
std::cout << pluss::note::print(
    pluss::timeline::format(matrix, "%F")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-01", "2024-01-02"], 
  ["2024-01-03", "2024-01-04"]
]
```

## See Also

- [`pluss::timeline::make`](make.md): canonical documentation for both names.
