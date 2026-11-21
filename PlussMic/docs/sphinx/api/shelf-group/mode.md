<!-- pluss-hand-authored -->

# ShelfGrouped::mode

[Back to ShelfGrouped API Reference](../../shelf_group_accessor_api_reference)

## Signature

```cpp
DataFrame ShelfGrouped::mode() const
```

## What It Does

For each group formed by [`ShelfGroupBuilder::by`](by.md), `mode()` finds the most frequent value in each group. It is the one-call form of
[`agg`](agg.md) with a single `GroupOp::Mode` aggregate.

Rows whose grouping key is `NULL` are left out unless the grouping was made with `ShelfGroupOptions::dropnull = false`.
`NULL` values in the value shelf are ignored. When several values are equally frequent, the smallest of them is returned. A group whose values are all `NULL` has the mode `NULL`.

## Parameters

`None` (the value shelf and the keys were supplied by `Shelf::group()` and `.by(...)`).

## Returns

`DataFrame`
    One row per group. The first columns hold the group key or keys, named `key`, `key_2` and so on, and the last column
    is named `mode`. Groups appear in the order their key first occurs, or in key order when the grouping was made with
    `ShelfGroupOptions::sort = true`.

## Errors

The value shelf must be numeric for every aggregate except `count` and `ndistinct`; a text value shelf is refused with `group: numeric aggregation requires a numeric Series shelf`.

## Examples

Example 1. Group by one key.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({1.0, 1.0, 2.0, 3.0, 3.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));

std::cout << values.group().by(region).mode();
```

### Output

```text
=================
     | key   mode
-----------------
0    | east  1   
1    | west  1   
-----------------
[2 cs x 2 feat]
```

Example 2. Group by two keys.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({1.0, 1.0, 2.0, 3.0, 3.0}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "east"}));
auto tier = Shelf(pluss::make_note({"a", "a", "b", "b", "a"}));

std::cout << values.group().by({region, tier}).mode();
```

### Output

```text
========================
     | key   key_2  mode
------------------------
0    | east  a      1   
1    | west  a      1   
2    | east  b      2   
3    | west  b      3   
------------------------
[4 cs x 3 feat]
```

Example 3. Sort the groups by key with `ShelfGroupOptions::sort`.

```cpp
using namespace pluss::table;

ShelfGroupOptions options;
options.sort = true;

auto values = Shelf(pluss::make_series({1.0, 1.0, 2.0, 3.0, 3.0}));
auto key = Shelf(pluss::make_note({"b", "a", "b", "a", "b"}));

std::cout << values.group(options).by(key).mode();
```

### Output

```text
=================
     | key   mode
-----------------
0    | a     1   
1    | b     1   
-----------------
[2 cs x 2 feat]
```

Example 4. `NULL` values are ignored.

```cpp
using namespace pluss::table;

auto values = Shelf(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}), pluss::make_series({true, false, true, true, true}));
auto region = Shelf(pluss::make_note({"east", "west", "east", "west", "solo"}));

std::cout << values.group().by(region).mode();
```

### Output

```text
=================
     | key   mode
-----------------
0    | east  10  
1    | west  40  
2    | solo  50  
-----------------
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
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="ndistinct.html">
    <span class="label">Next</span>
    <span class="title">ndistinct</span>
  </a>
</div>
