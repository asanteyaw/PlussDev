<!-- pluss-hand-authored -->

# Studio::name

[Back to Studio API Reference](../../studio_api_reference)

## Signature

```cpp
const std::string& Studio::name() const noexcept
```

## What It Does

Returns the figure's current name — whatever [`studio()`](studio.md)/the constructor set
it to, as most recently overridden by [`rename()`](rename.md).

## Parameters

*(none)*

## Returns

`const std::string&`
    The current name, by const reference. Empty if never set.

## Examples

Example 1. Reading the name back after construction and after a rename.

```cpp
using namespace pluss::pl;

auto fig = studio("initial");
std::cout << fig.name() << "\n";

fig.rename("renamed");
std::cout << fig.name() << "\n";
```

### Output

```text
initial
renamed
```

## Notes

- `noexcept`, and never throws for any figure state — even a freshly-default-constructed
  `Studio` has a well-defined (empty) name.
- Returned by const reference — the reference is invalidated by anything that would
  invalidate the `Studio` itself (destruction, move).

## See Also

- [`Studio::rename`](rename.md): the only way to change the name after construction.
- [`studio()`](studio.md): sets the initial name.
- [`Studio::headline_text`](headline_text.md): the sibling accessor for the
  actually-rendered headline text.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="headline_text.html">
    <span class="label">Previous</span>
    <span class="title">headline_text</span>
  </a>
  <a class="next-link" href="theme_tokens.html">
    <span class="label">Next</span>
    <span class="title">theme_tokens</span>
  </a>
</div>
