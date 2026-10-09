<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_numbers

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_numbers() const
```

## Collapse Numeric Runs Into One Semantic Marker

`mask_numbers()` replaces each ASCII numeric run with `[NUM]`. A run begins
at an ASCII digit and continues across digits plus a restricted separator when
that separator is immediately followed by another digit.

| Byte inside a run | Rule |
|---|---|
| `0` through `9` | Always continues the current run. |
| `, . - / : _` | Continues only when the next byte is a digit. |
| Sign, whitespace, letter, or other punctuation | Ends the run or remains outside it. |

Consequently `1,234.50`, `2024-01-02`, `3:45`, and `6_000` each become one
placeholder. In `+7`, the plus sign remains and only `7` becomes `[NUM]`.
`1..2` contains two runs because the first dot is not followed by a digit.
Digits embedded in `x7y` are still masked; this method does not require word
boundaries.

This broad behavior is appropriate for removing quantities and identifiers.
When semantic categories matter, run the narrower currency, date, and time
maskers first, or use [`mask_pii`](mask_pii.md), whose precedence does
that automatically. The method recognizes ASCII digits only.

The result is a fresh, shape-identical Text Note. Existing `[NUM]` markers are
unchanged, so repeated application is idempotent. The receiver is not mutated.
A digit precheck makes no-number cells a cheap copy path.

## Example 1: Whole Numeric Runs

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"IDs 42, 1,234.50, 2024-01-02, +7 and none"})
        .nlp().mask_numbers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["IDs [NUM], [NUM], [NUM], +[NUM] and none"]
```

## Example 2: Separator And Word Boundaries

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"1/2 3:45 6_000 8-9; 1..2 and x7y"})
        .nlp().mask_numbers()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["[NUM] [NUM] [NUM] [NUM]; [NUM]..[NUM] and x[NUM]y"]
```

## Example 3: Empty, Undefined, And Non-Text Dtypes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_numbers();
std::cout << "numbers empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "numbers undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_numbers().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"42"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_numbers();
} catch (const pluss::Error&) { std::cout << "numbers Bytes: rejected\n"; }
```

### Output

```text
numbers empty dim=1 numel=0
numbers undefined=false
numbers Bytes: rejected
```

Bool, Int64, and Float64 Notes reject for the same reason: this is a text
rewriter, not numeric formatting or arithmetic.

## See Also

- [`mask_currency`](mask_currency.md): preserve monetary category before broad number masking.
- [`mask_dates`](mask_dates.md): recognize complete date-shaped spans.
- [`mask_times`](mask_times.md): recognize valid clock-shaped spans.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_mentions.html"><span class="label">Previous</span><span class="title">mask_mentions</span></a>
  <a class="next-link" href="mask_pii.html"><span class="label">Next</span><span class="title">mask_pii</span></a>
</div>
