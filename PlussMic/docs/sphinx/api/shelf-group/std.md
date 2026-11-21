<!-- pluss-hand-authored -->

# ShelfGrouped::std

[Back to ShelfGrouped API Reference](../../shelf_group_accessor_api_reference)

## Signature

```cpp
DataFrame ShelfGrouped::std() const
```

## What It Does

For each group formed by [`ShelfGroupBuilder::by`](by.md), `std()` computes the sample standard deviation of each group (divisor `n - 1`). It is the one-call form of
[`agg`](agg.md) with a single `GroupOp::Std` aggregate.

Rows whose grouping key is `NULL` are left out unless the grouping was made with `ShelfGroupOptions::dropnull = false`.
`NULL` values in the value shelf are ignored. A group with fewer than two non-`NULL` values has the result `NULL`.

## Parameters

`None` (the value shelf and the keys were supplied by `Shelf::group()` and `.by(...)`).

## Returns

`DataFrame`
    One row per group. The first columns hold the group key or keys, named `key`, `key_2` and so on, and the last column
    is named `std`. Groups appear in the order their key first occurs, or in key order when the grouping was made with
    `ShelfGroupOptions::sort = true`.

## Errors

The value shelf must be numeric for every aggregate except `count` and `ndistinct`; a text value shelf is refused with `group: numeric aggregation requires a numeric Series shelf`.

## Examples

Example 1. Group by one key.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << values.group().by(region).std();
```

### Output

```text
========================
     | key   std        
------------------------
0    | east  20         
1    | west  14.14213562
------------------------
[2 cs x 2 feat]
```

Example 2. Group by two keys.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto tier = Shelf(pluss::make_note({"a", "a", "b", "b", "a"}));

std::cout << values.group().by({region, tier}).std();
```

### Output

```text
===============================
     | key   key_2  std        
-------------------------------
0    | east  a      28.28427125
1    | west  a      NULL       
2    | east  b      NULL       
3    | west  b      NULL       
-------------------------------
[4 cs x 3 feat]
```

Example 3. Sort the groups by key with `ShelfGroupOptions::sort`.

```cpp
using namespace pluss::table;

ShelfGroupOptions options;
options.sort = true;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto key = Shelf(pluss::make_note({"b", "a", "b", "a", "b"}));

std::cout << values.group(options).by(key).std();
```

### Output

```text
========================
     | key   std        
------------------------
0    | a     14.14213562
1    | b     20         
------------------------
[2 cs x 2 feat]
```

Example 4. `NULL` values are ignored.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}), pluss::make_series({true, false, true, true, true}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "solo"}));

std::cout << values.group().by(region).std();
```

### Output

```text
========================
     | key   std        
------------------------
0    | east  14.14213562
1    | west  NULL       
2    | solo  NULL       
------------------------
[3 cs x 2 feat]
```

## Notes

- The value shelf must be numeric for every aggregate except `count` and `ndistinct`; a text value shelf is refused with `group: numeric aggregation requires a numeric Series shelf`.
- To compute several aggregates in one pass, or to choose the output column names, use [`agg`](agg.md).
- To put the aggregate back on every input row instead of one row per group, use [`transform`](transform.md).

## See Also

- [`Shelf::group`](../shelf/group.md): enters the grouped workflow.
- [`ShelfGroupBuilder::by`](by.md): forms the groups.
- [`ShelfGrouped::agg`](agg.md): several aggregates at once.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ndistinct.html">
    <span class="label">Previous</span>
    <span class="title">ndistinct</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
