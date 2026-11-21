<!-- pluss-hand-authored -->

# ShelfGrouped::agg

[Back to ShelfGrouped API Reference](../../shelf_group_accessor_api_reference)

## Signatures

```cpp
DataFrame ShelfGrouped::agg(std::initializer_list<ShelfGroupSpec> specs) const
DataFrame ShelfGrouped::agg(pluss::RangeView<ShelfGroupSpec> specs) const
```

## What It Does

Computes one or more aggregates for every group in a single pass and returns them as a frame with one row per group.
Each `ShelfGroupSpec` names an output column and chooses the aggregate.

| `ShelfGroupSpec` field | Type | Meaning |
|---|---|---|
| `output` | `std::string` | The name of the result column. |
| `op` | `GroupOp` | One of `Count`, `Ndistinct`, `Sum`, `Mean`, `Median`, `Min`, `Max`, `Std`, `Var`, `Mode`. |

The behavior of each aggregate on `NULL` values, small groups and ties is described on its own page:
[`count`](count.md), [`ndistinct`](ndistinct.md), [`sum`](sum.md), [`mean`](mean.md), [`median`](median.md),
[`min`](min.md), [`max`](max.md), [`std`](std.md), [`var`](var.md), [`mode`](mode.md).

## Parameters

**specs** (`std::initializer_list<ShelfGroupSpec>` or `pluss::RangeView<ShelfGroupSpec>`)

The aggregates to compute, in output order.

## Returns

`DataFrame`
    The key columns (`key`, `key_2`, ...) followed by one column per spec, named by `output`. Groups appear in
    first-occurrence order, or key order when the grouping was made with `ShelfGroupOptions::sort = true`.

## Errors

Every aggregate except `Count` and `Ndistinct` requires a numeric value shelf. A text value shelf is refused with
`group: numeric aggregation requires a numeric Series shelf`.

## Examples

Example 1. Two aggregates in one pass.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << sales.group().by(region).agg({ShelfGroupSpec{"total", GroupOp::Sum}, ShelfGroupSpec{"avg", GroupOp::Mean}});
```

### Output

```text
========================
     | key   total  avg 
------------------------
0    | east  90     30  
1    | west  60     30  
------------------------
[2 cs x 3 feat]
```

Example 2. One aggregate, named.

```cpp
using namespace pluss::table;

auto sales = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << sales.group().by(region).agg({ShelfGroupSpec{"biggest", GroupOp::Max}});
```

### Output

```text
====================
     | key   biggest
--------------------
0    | east  50     
1    | west  40     
--------------------
[2 cs x 2 feat]
```

Example 3. Numeric aggregates need a numeric value shelf.

```cpp
using namespace pluss::table;

auto words = Shelf(pluss::make_note({"p", "q", "r"}));
auto key = Shelf(pluss::make_note({"a", "a", "b"}));

try {
  auto out = words.group().by(key).agg({ShelfGroupSpec{"total", GroupOp::Sum}});
} catch (const pluss::Error& e) {
  std::cout << e.message() << "\n";
}
```

### Output

```text
group: numeric aggregation requires a numeric Series shelf
```

## Notes

- `agg` collapses each group to one row. Use [`transform`](transform.md) to keep one row per input row.
- The single-aggregate methods such as [`sum`](sum.md) are shorthand for a one-spec `agg`.

## See Also

- [`ShelfGroupBuilder::by`](by.md): forms the groups.
- [`ShelfGrouped::transform`](transform.md): the same aggregates broadcast back to every row.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="by.html">
    <span class="label">Next</span>
    <span class="title">by</span>
  </a>
</div>
