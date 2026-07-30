<!-- pluss-hand-authored -->

# ThreadLabelAccessor::distinct

[Back to ThreadLabelAccessor API Reference](../../thread_label_accessor_api_reference)

## Signature

```cpp
ThreadLabelAccessor::distinct(bool dropnull = true) const -> Thread
```

## What It Does

Returns the bound thread's unique case-label values, in first-occurrence
order. With `dropnull = false`, a single `NULL` is kept (at the position of
its first occurrence) if any case label is null; with the default
`dropnull = true`, `NULL` is excluded entirely.

## Parameters

**dropnull** (`bool`, default `true`)

Whether to exclude a null case label from the result. When `false`, at most
one `NULL` entry is kept.

## Returns

`Thread`
    The distinct case-label values, in first-occurrence order, `display_name`
    `"distinct"`, plain default integer index.

## Examples

Example 1. Case labels with one repeated value.

```cpp
using namespace pluss::table;

auto tagged = Thread(pluss::make_series({100.0, 200.0, 150.0, 90.0}))
                  .label().set(Thread(pluss::make_note({"east", "west", "east", "north"})));

auto out = tagged.label().distinct();
std::cout << out;
```

### Output

```text
0  east
1  west
2  north

ptype: Note
```

`"east"` appears twice (rows `0` and `2`) but only once in the result, at the
position of its first occurrence.

Example 2. Pass `dropnull = false` to keep one `NULL` entry instead of
excluding it.

```cpp
auto with_gap = Thread(pluss::make_series({100.0, 150.0, 200.0, 90.0}))
                    .label().set(Thread(pluss::make_note({"10", "x", "20", "10"}),
                                        pluss::make_series(std::vector<bool>{true, false, true, true})));

std::cout << with_gap.label().distinct();
std::cout << with_gap.label().distinct(false);
```

### Output (default dropnull = true)

```text
0  10
1  20

ptype: Note
```

### Output (dropnull = false)

```text
0  10
1  NULL
2  20

ptype: Note
```

## Notes

- The result carries no case labels of its own; it prints with a plain
  default integer index.

## See Also

- [`ThreadLabelAccessor::freq`](freq.md): the value counts behind this same
  deduplication.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diff.html">
    <span class="label">Previous</span>
    <span class="title">diff</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
