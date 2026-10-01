# `pluss::ai::nn::causal_attention_mask`

Builds a `target x source` additive mask that blocks attending to future
positions.

## Signature

```cpp
Series nn::causal_attention_mask(
    std::int64_t target_len, std::int64_t source_len,
    pluss::Config cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Bool).with_platform(pluss::pCPU));
Series nn::causal_attention_mask(std::int64_t target_len, std::int64_t source_len, const Series& like);
```

## What It Does

Builds a `[target_len, source_len]` mask, `true` (or `1`, for a
non-bool `cfg.dtype`) at position `[i, j]` exactly where `j > i` --
blocking every query position `i` from attending to any key position
after it, the standard autoregressive mask. Passing this directly as
`attn_mask` to [`scaled_attention`](scaled_attention.md) (or
[`MultiheadAttention::forward`](../attention/MultiheadAttention.md#methods))
produces bit-for-bit the same result as that call's own `causal=true`
toggle -- this function exists so the mask can be built once and reused
across many calls, rather than recomputed by every one. Two overloads:
an explicit `cfg` (dtype/platform), or a `like` `Series` whose platform
the mask's platform is inferred from (dtype stays `Bool` in that form).

## Parameters

`std::int64_t target_len` / `std::int64_t source_len`
: The mask's two dimensions -- `target_len` rows (query positions),
  `source_len` columns (key positions). Independent; need not be equal
  (see [`square_subsequent_mask`](square_subsequent_mask.md) for the
  common equal-length case).

`pluss::Config cfg`
: Dtype and platform. Defaults to `Bool` on CPU.

`const Series& like`
: Infers the mask's platform from `like`'s own platform; dtype stays
  `Bool`.

## Returns

A `[target_len, source_len]` `Series`.

## Errors

- `target_len`/`source_len` non-positive -- throws.

## Practical Examples

Three examples: the boolean mask's shape and exact `true`/`false`
pattern (and its relationship to `square_subsequent_mask`), a non-bool
dtype and the `like`-overload's platform inference, and confirming this
mask, passed directly as `attn_mask`, produces the same result as
`causal=true`.

### Example 1

A non-square `2x3` mask -- `true` exactly where the column index exceeds
the row index. `square_subsequent_mask(n)` is exactly
`causal_attention_mask(n, n)`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series mask = nn::causal_attention_mask(2, 3);
  std::cout << "causal_attention_mask(2, 3): " << mask << "\n";

  const pluss::Series square = nn::square_subsequent_mask(3);
  std::cout << "square_subsequent_mask(3): " << square << "\n";

  // square_subsequent_mask(n) is exactly causal_attention_mask(n, n)
  const pluss::Series equiv = nn::causal_attention_mask(3, 3);
  const pluss::Series both_true = square.logical_and(equiv);
  const pluss::Series either_true = square.logical_or(equiv);
  std::cout << "square_subsequent_mask(3) == causal_attention_mask(3,3): "
            << std::boolalpha << both_true.to(pluss::ScalarType::Int64).sum().allclose(either_true.to(pluss::ScalarType::Int64).sum()) << "\n";

  return 0;
}
```

Output

```text
causal_attention_mask(2, 3): pluss::Series(shape=[2,3], strides=[3,1], dtype=bool, platform=cpu, numel=6, contiguous=true)
data=[[false, true, true],
  [false, false, true]]
square_subsequent_mask(3): pluss::Series(shape=[3,3], strides=[3,1], dtype=bool, platform=cpu, numel=9, contiguous=true)
data=[[false, true, true],
  [false, false, true],
  [false, false, false]]
square_subsequent_mask(3) == causal_attention_mask(3,3): true
```

### Example 2

`cfg.dtype` set to a non-bool type turns `true`/`false` into `1.0`/`0.0`
-- useful when the mask needs to be combined additively rather than as a
boolean. The `like`-overload infers the mask's platform from an existing
`Series`.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  // Config-form with an explicit non-bool dtype -- true/false become 1.0/0.0
  const pluss::Config f64_cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const pluss::Series float_mask = nn::causal_attention_mask(2, 2, f64_cfg);
  std::cout << "float-dtype mask: " << float_mask << "\n";

  // the `like` overload infers platform from an existing Series
  const pluss::Series logits = pluss::make_series({1.0, 2.0, 3.0, 4.0}, {2, 2});
  const pluss::Series like_mask = nn::causal_attention_mask(2, 2, logits);
  std::cout << "like-overload mask platform matches logits' own: "
            << std::boolalpha << (like_mask.platform() == logits.platform()) << "\n";

  return 0;
}
```

Output

```text
float-dtype mask: pluss::Series(shape=[2,2], strides=[2,1], dtype=float64, platform=cpu, numel=4, contiguous=true)
data=[[0, 1],
  [0, 0]]
like-overload mask platform matches logits' own: true
```

### Example 3

Passed directly as `attn_mask` to [`scaled_attention`](scaled_attention.md),
this mask produces bit-for-bit the same output as that same call's own
`causal=true` toggle.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai.h>

#include <iostream>

int main() {
  namespace nn = pluss::ai::nn;

  const pluss::Series q = pluss::make_series({1.0, 0.0, 0.0, 1.0, 1.0, 1.0}, {3, 2});

  nn::AttentionOptions causal_opts{};
  causal_opts.causal = true;
  const auto [out_causal_flag, w1] = nn::scaled_attention(q, q, q, pluss::series::undefined(), causal_opts);

  const pluss::Series mask = nn::square_subsequent_mask(3);
  const auto [out_explicit_mask, w2] = nn::scaled_attention(q, q, q, mask);

  std::cout << "causal=true == explicit square_subsequent_mask(3) as attn_mask: "
            << std::boolalpha << out_causal_flag.allclose(out_explicit_mask) << "\n";

  return 0;
}
```

Output

```text
causal=true == explicit square_subsequent_mask(3) as attn_mask: true
```

## See Also

- [`square_subsequent_mask`](square_subsequent_mask.md) -- the square
  (`target_len == source_len`) special case.
- [`scaled_attention`](scaled_attention.md) /
  [`multihead_attention`](multihead_attention.md) -- accept this
  directly as `attn_mask`.
