<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 082526f1a945 -->

# DataFrameGrouped::yield

[Back to DataFrameGroupBuilder API Reference](../../dataframe_group_accessor_api_reference)

## API Signature

```cpp
DataFrameGrouped::yield() const -> DataFrame
```

## Notes

`yield()` materializes the grouped rows without aggregating them. It is the
"show me the rows as these groups see them" terminal:

- rows are kept at full row grain
- grouping keys become contiguous blocks
- row order inside each group is preserved
- `dropnull` and `sort` from `group(...)` are respected

This is useful when you want grouped row neighborhoods for a follow-up rowwise
operation such as `shift()`, lag/lead construction, within-group comparisons,
or debugging a grouping plan before you reduce it.

## Inputs

This method takes no parameters.

## Output

`DataFrame`
    A regrouped dataframe at the original row grain. No aggregation happens.
    All original features remain. If `dropnull = true`, rows excluded by null
    grouping keys are omitted from the result.

## Options

`yield()` itself has no options. Its behavior is controlled by the options
already supplied to `group(...)`, especially:

- `dropnull`
- `sort`

## Examples

Example 1. Materialize rows grouped by account.

```cpp
using namespace pluss::table;

auto tx = DataFrame(
    {
        {"account", Thread(pluss::make_note({"B", "A", "B", "A"}))},
        {"amount", Thread(pluss::make_series({20.0, 10.0, 30.0, 40.0}))},
    },
    Thread(pluss::make_note({"r0", "r1", "r2", "r3"})));

auto grouped = tx.group({.sort = true})
                 .by({"account"})
                 .yield();
std::cout << grouped;
```

### Expected output

The rows stay unaggregated, but the `A` rows appear together before the `B`
rows:

```text
================
     | account amount
----------------
r1   | A       10
r3   | A       40
r0   | B       20
r2   | B       30
----------------
[4 cs x 2 feat]
```

Example 2. Use `yield()` before a within-group `shift()`.

```cpp
using namespace pluss::table;

auto tx = DataFrame({
    {"account", Thread(pluss::make_note({"A", "A", "B", "B"}))},
    {"date", Thread(pluss::make_timeline({"2024-01-01", "2024-01-05",
                                          "2024-02-01", "2024-02-03"}))},
    {"amount", Thread(pluss::make_series({10.0, 15.0, 20.0, 25.0}))},
});

auto grouped = tx.arrange({"account", "date"})
                 .group()
                 .by({"account"})
                 .yield();

auto next_date = grouped.features("date").shift(-1);
std::cout << grouped.assign({{"next_date", next_date}}).features({"account", "date", "next_date"});
```

### Expected output

Rows for each account stay contiguous, so the shifted date becomes the next row
inside that grouped block:

```text
account A rows appear first, then account B rows
the last row in each account block gets a null next_date
```

## See Also

- [`DataFrameGroupBuilder::by`](by.md)
- [`DataFrameGrouped::agg`](agg.md)
- [`DataFrameGrouped::transform`](transform.md)
- [`DataFrameGrouped::filter`](filter.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="../dataframe-group_accessor_api_reference.html">
    <span class="label">Next</span>
    <span class="title">Group API</span>
  </a>
</div>
