<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_dates

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_dates() const
```

## Replace Date-Shaped Spans Without Rewriting The Record

`mask_dates()` replaces recognized calendar expressions with `[DATE]`.
Choose it when downstream work needs to know that a date occurred but must not
retain its exact value. It does not alter standalone years, times, or arbitrary
numbers.

| Family | Accepted shape | Examples |
|---|---|---|
| Year first | `YYYY-MM-DD`, with `-`, `/`, or `.` used consistently | `2024-01-02` |
| Month first | `MM/DD/YY` or `MM/DD/YYYY` | `12/31/2024` |
| Day first | `DD.MM.YY` or `DD.MM.YYYY` | `31.12.2024` |
| Named month | English month name, day, optional year | `January 2, 2024`, `Sep 7` |

Numeric month fields are restricted to 1 through 12 and day fields to 1
through 31. Mixed separators such as `2024-01/02` do not match. The operation
is intentionally a conservative text recognizer, not a civil-calendar parser:
`Feb 31, 2024` is date-shaped and is masked. This avoids leaking a date-like
identifier merely because its source is malformed. Use Timeline construction
when calendar validity is the question.

Matches are case-insensitive for English month names. Relative language such
as `tomorrow`, day-first named forms such as `2 January 2024`, and non-English
month names are outside this method's grammar. `[DATE]` itself is unchanged on
another pass.

## Shape And Failure Contract

The operation accepts only Text Notes, allocates a fresh packed Text result,
and preserves scalar, vector, matrix, or rank-three shape exactly. Undefined
input remains undefined. Empty input remains a defined empty Note. Bytes and
native dtypes reject before scanning.

## Example 1: Every Accepted Family

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"2024-01-02", "12/31/2024", "31.12.2024",
                       "January 2, 2024", "Sep 7"})
        .nlp().mask_dates()) << '\n';
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["[DATE]", "[DATE]", "[DATE]", "[DATE]", "[DATE]"]
```

## Example 2: Range And Separator Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({
        "keep 2024-13-01 and 2024-01/02; syntax Feb 31, 2024"})
        .nlp().mask_dates()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["keep 2024-13-01 and 2024-01/02; syntax [DATE]"]
```

## Example 3: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_dates();
std::cout << "dates empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "dates undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_dates().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"2024-01-02"},
      std::array<std::int64_t, 1>{1}).nlp().mask_dates();
} catch (const pluss::Error&) { std::cout << "dates Bytes: rejected\n"; }
```

### Output

```text
dates empty dim=1 numel=0
dates undefined=false
dates Bytes: rejected
```

## See Also

- [`mask_times`](mask_times.md): mask clock expressions.
- [`mask_pii`](mask_pii.md): apply date masking with the other sensitive categories.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_currency.html"><span class="label">Previous</span><span class="title">mask_currency</span></a>
  <a class="next-link" href="mask_emails.html"><span class="label">Next</span><span class="title">mask_emails</span></a>
</div>
