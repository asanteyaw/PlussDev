<!-- pluss-hand-authored -->
# Note::NlpAccessor::mask_times

[Back to Note NLP Accessor API Reference](../../note_nlp_accessor_api_reference)

## Signature

```cpp
Note Note::NlpAccessor::mask_times() const
```

## Replace Valid Clock Shapes As One Unit

`mask_times()` replaces clock expressions with `[TIME]`. It recognizes
24-hour `H:MM`/`HH:MM` forms from `0:00` through `23:59`, and 12-hour forms
from `1am` through `12pm`, optionally including minutes and one space before
the suffix.

| Form | Examples | Rejected boundary |
|---|---|---|
| 24-hour | `00:00`, `9:05`, `23:59` | `24:00`, `12:60` |
| 12-hour | `7pm`, `07:05 AM`, `12:00pm` | `0pm`, `19:30pm` |

The full suffixed form is replaced atomically; `07:05 AM` does not leave the
suffix behind. Seconds, timezone suffixes, and prose such as `noon` are not
part of this grammar. Use broad number masking only if those numeric fragments
should be hidden independently.

The operation accepts Text, returns a fresh Text Note with identical shape,
and does not mutate the receiver. It is idempotent, propagates undefined input,
and preserves defined empty shapes. Cells without digits bypass recognition.

## Example 1: 24-Hour And 12-Hour Forms

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"00:00", "23:59", "7pm", "07:05 AM"})
        .nlp().mask_times()) << '\n';
```

### Output

```text
Note(shape=[4], dim=1, numel=4, encoding=utf8)
["[TIME]", "[TIME]", "[TIME]", "[TIME]"]
```

## Example 2: Invalid Ranges Stay Visible

```cpp
std::cout << pluss::note::print(
    pluss::note::make({"keep 24:00 19:30pm 12:60; mask 12:00pm"})
        .nlp().mask_times()) << '\n';
```

### Output

```text
Note(shape=[1], dim=1, numel=1, encoding=utf8)
["keep 24:00 19:30pm 12:60; mask [TIME]"]
```

## Example 3: Empty, Undefined, And Bytes

```cpp
const auto empty = pluss::note::make(std::vector<std::string>{})
                       .nlp().mask_times();
std::cout << "times empty dim=" << empty.dim()
          << " numel=" << empty.numel() << '\n';
std::cout << "times undefined=" << std::boolalpha
          << pluss::Note::undefined().nlp().mask_times().defined() << '\n';
try {
  (void)pluss::note::bytes(
      std::vector<std::string>{"7pm"}, std::array<std::int64_t, 1>{1})
      .nlp().mask_times();
} catch (const pluss::Error&) { std::cout << "times Bytes: rejected\n"; }
```

### Output

```text
times empty dim=1 numel=0
times undefined=false
times Bytes: rejected
```

## See Also

- [`mask_dates`](mask_dates.md): mask calendar-shaped spans.
- [`mask_numbers`](mask_numbers.md): mask numeric fragments regardless of clock validity.
- [`mask_pii`](mask_pii.md): apply time masking in the standard multi-category order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask_pii.html"><span class="label">Previous</span><span class="title">mask_pii</span></a>
  <a class="next-link" href="mask_urls.html"><span class="label">Next</span><span class="title">mask_urls</span></a>
</div>
