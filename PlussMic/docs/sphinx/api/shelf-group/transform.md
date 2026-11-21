<!-- pluss-hand-authored -->

# ShelfGrouped::transform

[Back to ShelfGrouped API Reference](../../shelf_group_accessor_api_reference)

## Signatures

```cpp
Shelf ShelfGrouped::transform(GroupOp op) const
DataFrame ShelfGrouped::transform(std::initializer_list<ShelfGroupSpec> specs) const
DataFrame ShelfGrouped::transform(pluss::msharp::RangeView::Of<ShelfGroupSpec> specs) const
```

## What It Does

Computes aggregates per group and writes each group's result back onto every row of that group. The result has one row
per input row, in the original order, instead of one row per group.

- `transform(op)` gives a single aggregate as a `Shelf`.
- `transform(specs)` gives several, as a `DataFrame` with one column per `ShelfGroupSpec::output`.

A row whose own value is `NULL` still receives its group's aggregate. Rows excluded because their key was `NULL` are
the only exception, and the `dropnull` option controls that.

## Parameters

**op** (`GroupOp`)

The aggregate to compute: `Count`, `Ndistinct`, `Sum`, `Mean`, `Median`, `Min`, `Max`, `Std`, `Var` or `Mode`.

**specs**

Named aggregates, as for [`agg`](agg.md).

## Returns

`Shelf` for `transform(op)`, `DataFrame` for `transform(specs)`.
    Same length as the value shelf.

## Errors

The numeric aggregates require a numeric value shelf, as for [`agg`](agg.md).

## Examples

Example 1. Broadcast one aggregate back to every row with `transform(op)`.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << sales.group().by(region).transform(GroupOp::Mean);
```

### Output

```text
0  30
1  30
2  30
3  30
4  30
ptype: Series
```

Example 2. Several broadcast aggregates as a frame with `transform(specs)`.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << sales.group().by(region).transform({ShelfGroupSpec{"mean", GroupOp::Mean}, ShelfGroupSpec{"max", GroupOp::Max}});
```

### Output

```text
=================
     | mean  max 
-----------------
0    | 30    50  
1    | 30    40  
2    | 30    50  
3    | 30    40  
4    | 30    50  
-----------------
[5 cs x 2 feat]
```

Example 3. A `NULL` value still receives its group aggregate.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}), pluss::make_series({true, false, true, true, true}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "solo"}));

std::cout << values.group().by(region).transform(GroupOp::Sum);
```

### Output

```text
0  40
1  40
2  40
3  40
4  50
ptype: Series
```

## Notes

- Use `transform` to compare each row with its group, for example a value minus its group mean.
- Use [`agg`](agg.md) when one row per group is what you need.

## See Also

- [`ShelfGroupBuilder::by`](by.md): forms the groups.
- [`ShelfGrouped::agg`](agg.md): one row per group.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sum.html">
    <span class="label">Previous</span>
    <span class="title">sum</span>
  </a>
  <a class="next-link" href="var.html">
    <span class="label">Next</span>
    <span class="title">var</span>
  </a>
</div>
