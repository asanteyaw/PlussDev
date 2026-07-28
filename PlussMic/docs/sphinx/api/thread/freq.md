<!-- pluss-hand-authored -->

# Thread::freq

[Back to Thread API Reference](../../thread_api_reference)

## API Signature

```cpp
Thread::freq() -> Thread
Thread::freq(options: ThreadFreqOptions) -> Thread
```

## Notes

Counts how often each distinct value occurs in one thread and returns a labeled
`Thread`. The output values are the counts or proportions; the output labels are
the distinct source values.

This is the 1D frequency surface. Use [`DataFrame::freq`](../dataframe/freq.md)
when you want grouped frequencies across one or more dataframe features.

## Options

```cpp
struct ThreadFreqOptions {
  ThreadFreqKind kind{ThreadFreqKind::Count};
  bool dropnull{true};
  bool sort{true};
  bool ascending{false};
  std::int64_t top{0};
};
```

`kind`
`Count` returns occurrence counts. `Prop` returns normalized proportions.

`dropnull`
When `true`, missing values are excluded. When `false`, missing values are counted
under the label `NULL`.

`sort`
When `true`, results are ordered by frequency. When `false`, first-seen order is
preserved.

`ascending`
Only applies when `sort=true`. Sorts from smallest to largest frequency.

`top`
Keeps only the first `top` rows after ordering. `0` keeps all rows.

## Output

`Thread`
The payload is numeric:

- `Int64` when `kind = Count`
- `Float64` when `kind = Prop`

The labels hold the distinct source values.

## Examples

```cpp
using namespace pluss::table;

auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Tema", "Accra"}));
auto counts = cities.freq();
std::cout << counts;
```

## Expected output

```text
Accra    3
Tema     2
Name: count, ptype: Series
```

```cpp
auto prop = cities.freq({
    .kind = ThreadFreqKind::Prop,
});
std::cout << prop;
```

## Expected output

```text
Accra    0.6
Tema     0.4
Name: prop, ptype: Series
```

```cpp
auto with_gap = Thread(
    pluss::make_note({"Accra", "", "Tema"}),
    pluss::make_series(std::vector<bool>{true, false, true})
);

std::cout << with_gap.freq({
    .dropnull = false,
    .sort = false,
});
```

## Expected output

```text
Accra    1
NULL     1
Tema     1
Name: count, ptype: Series
```

## See Also

- [`DataFrame::freq`](../dataframe/freq.md)
- [`Thread::distinct`](distinct.md)
- [`Thread::ndistinct`](ndistinct.md)

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="take.html">
    <span class="label">Previous</span>
    <span class="title">take</span>
  </a>
  <a class="next-link" href="titlecase.html">
    <span class="label">Next</span>
    <span class="title">titlecase</span>
  </a>
</div>
