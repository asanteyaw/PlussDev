# Thread::describe

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::describe(bool render = false) -> ThreadView
```

Summarize one thread instead of carrying every original value forward.

`describe()` adapts to the payload family: numeric threads get numeric descriptive statistics, while note threads get count/unique/top/freq style summaries.

This is the direct per-column analogue of `DataFrame::describe()` when you already know which one thread you want to inspect.

## Parameters

**render**

Display toggle for the returned view.

- `false`: keep explicit control over when the summary is printed.
- `true`: allow the returned view to auto-render in temporary display contexts.

## Returns

`ThreadView`
    Compact summary view of the current thread.

## Notes

Numeric series and note-backed text threads follow different summary contracts, so the output shape depends on the payload family.

For numeric threads, the statistics are ordered as `count`, `mean`, `std`, `min`, `25%`, `50%`, `75%`, `max`.

For note threads, the summary is frequency-oriented: observed count, number of unique values, most frequent value, and its frequency.

## Examples

Example 1. Describe a numeric thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0})).describe(false);
```

### Output

```text
count  4
 mean  25
  std  12.90994449
  min  10
  25%  17.5
  50%  25
  75%  32.5
  max  40
Name: value, ptype: Series
```

Example 2. Describe a note-backed text thread.

```cpp
using namespace pluss::table;

auto out = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"})).describe(false);
```

### Output

```text
count  5
unique  3
   top  Accra
  freq  2
Name: value, ptype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="display_name.html">
    <span class="label">Next</span>
    <span class="title">display_name</span>
  </a>
</div>
