<!-- pluss-hand-authored -->
<!-- pluss-verifier: PlussAI/examples/NodeB/nodeb_embed_docs_reference.cpp -->
<!-- pluss-signature-hash: c9af28299bca -->
# `pluss::ai::nlp::embed::ContextualEmbedder::generate`

[Back to `ContextualEmbedder`](../ContextualEmbedder.md)

## Signature

```cpp
pluss::ai::nlp::StatusOr<std::vector<std::string>>
pluss::ai::nlp::embed::ContextualEmbedder::generate(
    const pluss::ai::nlp::Doc& prompt,
    const pluss::ai::nlp::embed::GenerateOptions& options = {}) const;
```

## What It Does

`generate()` continues a prepared prompt with the causal language-model head learned by `learn()`. It frames the prompt, removes the closing marker, repeatedly encodes the growing prefix, projects the last row to vocabulary logits, selects one token, and appends it. The returned strings are only newly generated tokens: the prompt and end marker are omitted.

Greedy mode selects argmax and is deterministic. Sampling mode optionally divides logits by temperature, retains the top `k`, renormalizes, and draws from that distribution with the supplied seed. Sampling-only fields are wholly ignored in greedy mode. An empty prompt is valid and begins from the model's start marker.

Generation stops at end-of-sequence, `max_new_tokens`, or position capacity. Because the full prefix is re-encoded each step, cost grows more quickly than a single `encode()`.

## Errors

The model must be ready and `max_new_tokens` positive. Sampling additionally requires finite positive temperature and non-negative `top_k`.

## Practical Examples

### Greedy continuation

```cpp
embed::GenerateOptions options{.max_new_tokens = 3, .greedy = true};
const auto tokens = model.generate(prep.infer("the bank raised interest"), options);
for (const auto& token : tokens.value()) std::cout << token << ' ';
std::cout << '\n';
```

Output

```text
rate 
```

### Seeded top-k sampling

```cpp
embed::GenerateOptions options{.max_new_tokens = 4, .greedy = false,
                               .temperature = 0.8, .top_k = 3, .seed = 7};
const auto a = model.generate(prompt, options).value();
const auto b = model.generate(prompt, options).value();
std::cout << std::boolalpha << (a == b) << ' ' << a.front() << '\n';
```

Output

```text
true rate
```

### Empty prompt

```cpp
const auto tokens = model.generate(nlp::Doc{}, {.max_new_tokens = 3});
std::cout << std::boolalpha << tokens.ok() << ' '
          << (tokens.value().size() <= 3) << '\n';
```

Output

```text
true true
```

### Greedy ignores sampling fields

```cpp
embed::GenerateOptions options{.max_new_tokens = 3, .greedy = true,
                               .temperature = std::numeric_limits<double>::quiet_NaN(),
                               .top_k = -1};
std::cout << std::boolalpha
          << (model.generate(prompt, options).value() == greedy_tokens) << '\n';
```

Output

```text
true
```

### Invalid sampling controls

```cpp
options.greedy = false;
std::cout << model.generate(prompt, options).status().message << '\n';
```

Output

```text
nlp::embed::ContextualEmbedder::generate: temperature must be positive.
```

## See Also

- [`GenerateOptions`](../GenerateOptions.md) details every decoding field.

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="encode.html">
    <span class="label">Previous</span>
    <span class="title">encode</span>
  </a>
  <a class="next-link" href="learn.html">
    <span class="label">Next</span>
    <span class="title">learn</span>
  </a>
</div>
<!-- apiref:nav:end -->
