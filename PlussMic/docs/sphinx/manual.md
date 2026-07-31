# PlussMic User Manual

This manual is the long-form guide for PlussMic, the data-analysis layer of the
Pluss ecosystem. It is written for readers who want to use PlussMic as a daily
tool, not only look up signatures.

PlussMic has three related models:

```text
table  -> DataFrame, Thread, Atlas
kindle -> Stream, Catalog, Archive
bundle -> Slot, Registry, Axel
```

Most users start with `DataFrame`, because it is the table abstraction used for
CSV, Excel, JSON, Parquet, feature engineering, exploratory analysis, plotting,
and export. `Thread` is the typed column model underneath `DataFrame`. `Atlas`
stacks related frames. `Kindle` and `Bundle` are lower-level record/stream
models used when the data is not naturally just a rectangular table.

## How to read this manual

Read Part I and Part II in order. After that, jump to the chapters that match
your task:

- Loading files: Part III.
- Loading and saving files: Part IV.
- Cleaning and transforming tables: Part V.
- Column-level operations: Part VI.
- Query expressions and SQL: Part VII.
- Time series: Part VIII.
- Plotting: Part IX.
- Multi-frame Atlas data: Part X.
- Kindle and Bundle models: Part XI and Part XII.
- Recipes: Part XIII.
- Production debugging: Part XIV.

The API reference pages remain the authority for exact overload lists. This
manual explains how to choose and combine those APIs.

## Example policy

This manual does not use one token example per feature. Important APIs are
shown in several contexts: a minimal call, a realistic workflow, option and
error cases, and combinations with adjacent APIs. A short example teaches the
shape of a call; later examples teach how to use it in a program.

In-memory frames are created directly when the lesson is about table
operations. I/O chapters use files such as `"sales.csv"` and
`"transactions.parquet"`, assumed to be in the program's working directory.
The manual shows the relevant input file contents before the code. Public
examples never contain paths from the author's machine.

Pluss values are passed directly where `Thread` conversion is implicit:

```cpp
DataFrame sales({
    {"city", pluss::make_note({"Accra", "Tema"})},
    {"units", pluss::make_series({10.0, 7.0})},
});
```

Explicit `Thread` construction appears only when the example needs Thread
metadata, a validity mask, or a named standalone column.

## Verified examples

Examples marked as verified are backed by this source file:

```text
PlussMic/docs/sphinx/manual/examples/plussmic_manual_examples.cpp
```

It is compiled as an external SDK consumer with:

```bash
cmake -S PlussMic/docs/sphinx/manual/examples \
  -B /tmp/plussmic_manual_examples_build \
  -DCMAKE_PREFIX_PATH="/path/to/pluss-sdk;/path/to/cpp_env"

cmake --build /tmp/plussmic_manual_examples_build --target plussmic_manual_examples
/tmp/plussmic_manual_examples_build/plussmic_manual_examples
```

The printed outputs in this manual come from running the exact displayed
operations against the current PlussMic build. A table shown after
`df.to_string()` is its `to_string()` result; output from `head()`, `show()`, or
another renderer is never substituted. When output reveals surprising
behavior, the source and tests are checked first; source-level defects are
fixed before the manual teaches the behavior as correct.

```{toctree}
:maxdepth: 2

manual/part01_orientation
manual/part02_installation
manual/part03_table_model
manual/part04_io
manual/part05_dataframe_workflows
manual/part06_thread_workflows
manual/part07_query_sql
manual/part08_time_series
manual/part09_plotting
manual/part10_atlas
manual/part11_kindle
manual/part12_bundle
manual/part13_recipes
manual/part14_troubleshooting
manual/part15_api_map
```
