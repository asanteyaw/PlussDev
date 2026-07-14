# Thread::ends

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::ends(std::string_view suffix) -> Thread
```

Check whether each note value ends with the supplied suffix.

`ends(...)` is the note-thread suffix test. Use it when you are building filename filters, extension masks, code-family checks, or any text workflow where the tail of the string matters.

## Parameters

**suffix**

Literal trailing text to test for at the end of each note value.

## Returns

`Thread`
    Boolean-style thread marking which note values end with the supplied suffix.

## See Also

- <a href="starts.html"><code>Thread::starts</code></a>: prefix-based companion check.
- <a href="matches.html"><code>Thread::matches</code></a>: regex-oriented matching instead of literal suffix checks.

## Notes

`ends(...)` is note-only. It is intended for textual payloads.

## Examples

Example 1. Build a suffix mask for filename-like values.

```cpp
using namespace pluss::table;

auto names = Thread(pluss::make_note({"Ada.csv", "Bola.txt", "Chen.csv"}));

auto out = names.ends(".csv");
```

### Output

```text
0  true
1  false
2  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="eq.html">
    <span class="label">Next</span>
    <span class="title">eq</span>
  </a>
</div>
