# `pluss::ai::nn::square_subsequent_mask`

Builds a square `causal_attention_mask` for self-attention over one
sequence.

## Signature

```cpp
Series nn::square_subsequent_mask(
    std::int64_t size,
    pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Bool).with_platform(pluss::pCPU));
Series nn::square_subsequent_mask(std::int64_t size, const Series& like);
```

## What It Does

`square_subsequent_mask(size, ...)` is exactly
`causal_attention_mask(size, size, ...)` -- the `target_len == source_len
== size` special case for self-attention over a single sequence, where
the query and key positions are the same sequence. See
[`causal_attention_mask`](causal_attention_mask.md) for the full
`true`/`false` pattern and how the result is typically used (as
`attn_mask`, equivalent to `causal=true`).

## Parameters

`std::int64_t size`
: Both dimensions of the resulting `[size, size]` mask.

`pluss::Config cfg`
: Dtype and platform. Defaults to `Bool` on CPU.

`const Series& like`
: Infers the mask's platform from `like`'s own platform; dtype stays
  `Bool`.

## Returns

A `[size, size]` `Series`.

## Errors

- `size` non-positive -- throws.

## Practical Examples

Three examples: the mask's shape and pattern confirmed identical to the
equivalent `causal_attention_mask` call, the `like`-overload's platform
inference, and using the mask directly with
[`MultiheadAttention`](../attention/MultiheadAttention.md).

### Example 1

`square_subsequent_mask(3)` and `causal_attention_mask(3, 3)` produce the
exact same mask.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series square = nn::square_subsequent_mask(3);
  const pluss::Series equiv = nn::causal_attention_mask(3, 3);

  std::cout << "square_subsequent_mask(3): " << square << "\n";

  const pluss::Series both_true = square.logical_and(equiv);
  const pluss::Series either_true = square.logical_or(equiv);
  std::cout << "identical to causal_attention_mask(3, 3): "
            << std::boolalpha
            << both_true.to(pluss::ScalarType::Int64).sum().allclose(either_true.to(pluss::ScalarType::Int64).sum())
            << "\n";

  return 0;
}
```

Output

```text
square_subsequent_mask(3): pluss::Series(shape=[3,3], strides=[3,1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[[false, true, true],
  [false, false, true],
  [false, false, false]]
identical to causal_attention_mask(3, 3): true
```

### Example 2

The `like`-overload infers the mask's platform from an existing
`Series`, the same convention [`causal_attention_mask`](causal_attention_mask.md)'s
own `like`-overload uses.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series logits = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
  const pluss::Series mask = nn::square_subsequent_mask(2, logits);

  std::cout << "mask platform matches logits' own: "
            << std::boolalpha << (mask.platform() == logits.platform()) << "\n";

  return 0;
}
```

Output

```text
mask platform matches logits' own: true
```

### Example 3

Passed as `attn_mask` to a live
[`MultiheadAttention`](../attention/MultiheadAttention.md) instance,
this mask produces the same result as that call's own `causal=true`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  nn::MultiheadAttention attn(4, 2);
  const pluss::Series x = pluss::make_series(
      {1.0, 0.0, 0.0, 0.0,
       0.0, 1.0, 0.0, 0.0,
       0.0, 0.0, 1.0, 0.0},
      {3, 4});

  const pluss::Series mask = nn::square_subsequent_mask(3);

  const auto [out_mask, w1] = attn.forward(x, x, x, mask);
  const auto [out_causal, w2] = attn.forward(x, x, x, pluss::series::undefined(),
      pluss::series::undefined(), /*causal=*/true);

  std::cout << "explicit mask == causal=true: " << std::boolalpha << out_mask.allclose(out_causal) << "\n";

  return 0;
}
```

Output

```text
explicit mask == causal=true: true
```

## See Also

- [`causal_attention_mask`](causal_attention_mask.md) -- the general
  (non-square) form this specializes.
- [`MultiheadAttention`](../attention/MultiheadAttention.md) -- a common
  consumer of this mask, as its `attn_mask` argument.
