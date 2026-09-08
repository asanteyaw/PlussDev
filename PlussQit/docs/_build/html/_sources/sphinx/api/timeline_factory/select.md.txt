<!-- pluss-hand-authored -->
# pluss::timeline::select

[Back to Timeline Factory API Reference](../../timeline_factory_api_reference)

## Signatures

```cpp
Timeline pluss::timeline::select(series_range conditions,
                                 timeline_range choices,
                                 const Timeline& default_value)
Timeline pluss::timeline::select(const Series& condition,
                                 const Timeline& choice,
                                 const Timeline& default_value)
Timeline pluss::timeline::select(series_range conditions,
                                 timeline_range choices,
                                 std::string_view default_value)
Timeline pluss::timeline::select(const Series& condition,
                                 std::string_view choice,
                                 std::string_view default_value)
```

The range forms accept contiguous ranges or braced lists. The single-condition
forms are concise wrappers over a one-branch selection.

## Build An Ordered Temporal Choice

`select()` constructs one Timeline from boolean conditions and temporal
choices. At each broadcast output position, conditions are considered from
first to last. The choice paired with the first true condition supplies the
instant. If every condition is false, `default_value` supplies it.

This ordered rule matters when conditions overlap: later branches are fallbacks,
not overwrites. Put the narrowest or highest-priority condition first. All
choices are evaluated and validated before selection, so a malformed or
timezone-incompatible branch is an error even if its condition happens to be
false everywhere.

Use the Timeline overloads when choices are already computed, carry explicit
timezone metadata, contain absent instants, or have non-scalar shape. The text
shorthand is for one scalar text choice and one scalar text default; each token
is classified as date-only, time-only, or complete datetime text and parsed by
the corresponding strict factory.

## Broadcasting And Output

Every condition, choice, and the default participates in ordinary trailing-axis
broadcasting. Scalar choices therefore expand without first allocating a full
Timeline. A vector choice may broadcast across rows of a matrix condition. The
result shape is the common broadcast shape and the result owns newly allocated,
contiguous storage.

Conditions must be defined boolean Series. Numeric truthiness is not accepted.
The number of conditions must equal the number of choices and at least one pair
is required.

All Timeline operands must carry the same timezone label. Selection combines
absolute instants, but one result can expose only one civil projection; rejecting
mixed metadata prevents branch order from silently choosing that projection.
Convert operands explicitly before selection when a common display zone is
intended.

Absent Timeline values are ordinary branch values. Selecting an absent choice
produces an absent output lane; an absent value does not alter condition order.

## Example 1: Single-Condition Text Shorthand

```cpp
const auto holiday = pluss::make_series({true, false, false});
const auto result = pluss::timeline::select(
    holiday, "2024-12-25", "2024-01-01");

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[3], dim=1, numel=3, encoding=utf8)
["2024-12-25", "2024-01-01", "2024-01-01"]
```

The scalar choice and default broadcast to the condition's three positions.

## Example 2: Broadcast A Vector Choice Across A Matrix

```cpp
const auto mask = pluss::make_series({
    {true, false},
    {false, true}});
const auto choices = pluss::timeline::date(
    {"2024-01-10", "2024-01-20"});
const auto fallback = pluss::timeline::date("2024-01-31");

const auto result = pluss::timeline::select(mask, choices, fallback);
std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["2024-01-10", "2024-01-31"], 
  ["2024-01-31", "2024-01-20"]
]
```

The two-element choice aligns with the final axis. Where the mask is false, the
scalar fallback supplies January 31.

## Example 3: First True Condition Wins

```cpp
const auto early = pluss::make_series({true, true, false, false});
const auto late  = pluss::make_series({false, true, true, false});

const auto result = pluss::timeline::select(
    {early, late},
    {pluss::timeline::date("2024-01-10"),
     pluss::timeline::date("2024-01-20")},
    pluss::timeline::date("2024-01-31"));

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["2024-01-10", "2024-01-10", "2024-01-20", "2024-01-31"]
```

At position 1 both conditions are true, and the earlier branch supplies January
10. Position 3 reaches the default because neither condition is true.

## Example 4: Select An Absent Instant Deliberately

```cpp
const auto result = pluss::timeline::select(
    pluss::make_series({true, false}),
    pluss::timeline::date("2024-02-29"),
    pluss::timeline::from_epoch(pluss::Timeline::kNaT));

std::cout << pluss::note::print(
    pluss::timeline::format(result, "%F")) << '\n';
```

### Output

```text
Note(shape=[2], dim=1, numel=2, encoding=utf8)
["2024-02-29", "NaT"]
```

## Failure Modes And Cost

`select()` throws for an empty condition list, unequal condition and choice
counts, undefined or non-boolean conditions, undefined choices, incompatible
broadcast shapes, timezone mismatch, or malformed text choices. Work is linear
in the number of branches times the broadcast result size; every branch is
materialized through the selection fold.

## See Also

- [`Timeline::where`](../timeline/where.md): direct two-way selection from an existing Timeline.
- [`date`](date.md), [`time`](time.md), and [`make`](make.md): explicit text parsing before selection.
