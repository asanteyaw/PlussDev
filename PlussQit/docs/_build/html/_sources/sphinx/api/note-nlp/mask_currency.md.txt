<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_currency

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_currency() const
```

## Preserve The Sentence, Hide The Amount

`mask_currency()` replaces each recognized monetary amount with the text
`[CUR]`. It is useful when the fact that money was discussed matters, but the
amount itself must not survive into logs, corpora, prompts, or features. Unlike
deletion, masking preserves a visible category marker and usually leaves the
sentence grammatically inspectable.

The recognizer accepts these forms:

| Form | Recognized examples | Notes |
|---|---|---|
| Leading symbol | `$12.50`, `€ 20`, `£9`, `¥100` | Optional space follows the symbol. `$` amounts may end in `k`, `M`, or `B`. |
| Leading code | `CAD 42`, `CHF1,250.00` | The code match is case-insensitive. |
| Trailing code | `30 eur`, `99 GBP` | One optional space may separate amount and code. |

Supported codes are `USD`, `EUR`, `GBP`, `JPY`, `CAD`, `AUD`, `CHF`, `CNY`,
and `INR`. A bare number is deliberately untouched. Use
[`mask_numbers`](mask_numbers.md) when every numeric span should be
hidden, or [`mask_pii`](mask_pii.md) when several categories must be
processed in a defined precedence order.

The method recognizes amount syntax; it does not convert currencies, validate
locale-specific grouping, or preserve the currency kind. Every match becomes
the same `[CUR]` marker. Existing markers contain no amount syntax, so applying
the method again is idempotent.

## Shape, Storage, And Cost

The receiver must be a Text `Note`. Bytes, Bool, Int64, and Float64 reject
before cell access. The result is a fresh Text `Note` with exactly the same
rank and extents; the receiver is unchanged. Undefined input propagates as
undefined, and a defined empty shape remains defined and empty.

Cells are independent. A cheap character precheck bypasses recognition for
cells that contain neither digits nor a supported symbol. Work is linear in
the text scanned plus the output produced, with one packed output arena.

## Example 1: Symbol Amounts

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"$12.50", "€ 20", "£9", "¥100", "$2M"})
        .nlp().mask_currency()) << '\n';
```

### Output

```text
Note(shape=[5], dim=1, numel=5, encoding=utf8)
["[CUR]", "[CUR]", "[CUR]", "[CUR]", "[CUR]"]
```

## Example 2: Currency Codes And A Bare Number

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"CAD 42", "30 eur", "CHF1,250.00", "keep 99"})
        .nlp().mask_currency()) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["[CUR]", "[CUR]", "[CUR]", "keep 99"]
```

## Example 3: Matrix Shape Is Preserved

```cpp
const auto matrix = pluss::note::make(
    std::vector<std::string>{"$1", "a@b.io", "7pm", "#tag"},
    std::array<std::int64_t, 2>{2, 2});
std::cout << pluss::note::print(matrix.nlp().mask_currency()) << '\n';
```

### Output

```text
Note(shape=[2, 2], dim=2, numel=4, encoding=utf8)
[
  ["[CUR]", "a@b.io"], 
  ["7pm", "#tag"]
]
```

Only the amount changes. Email, time, and hashtag syntax belongs to other
maskers and passes through untouched.

## Example 4: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_currency();
std::cout << "currency empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "currency undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_currency().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"$1"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_currency();
} catch (const pluss::Error&) { std::cout << "currency Bytes: rejected\n"; }
```

### Output

```text
currency empty dim=1 numel=0
currency undefined=false
currency Bytes: rejected
```

## See Also

- [`mask_numbers`](mask_numbers.md): hide numeric runs without requiring currency syntax.
- [`mask_pii`](mask_pii.md): mask all selected sensitive categories in one ordered pass.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="lower_tokens.html"><span class="label">Previous</span><span class="title">lower_tokens</span></a>
  <a class="next-link" href="mask_dates.html"><span class="label">Next</span><span class="title">mask_dates</span></a>
</div>
