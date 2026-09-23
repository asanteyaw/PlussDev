# `pluss::ai::nn::PReLUOptions`

The learnable weight's shape, its initial value, and where it is allocated.

Passed to [`PReLU`](PReLU.md)'s constructor; all three fields have defaults,
so `PReLUOptions{}` reproduces a single shared slope initialized to `0.25`.
Unlike every other options struct in this family, `PReLU` does not accept
this struct as a single defaulted constructor argument -- instead it has a
second constructor that takes `num_parameters`, `init`, and `cfg` as three
separate defaulted parameters (see [`PReLU`](PReLU.md)'s Signature), which
is what makes plain `nn::PReLU prelu;` work. Passing a `PReLUOptions` value
directly is equivalent and often clearer once more than one field is being
overridden.

## Fields

### `num_parameters`

`std::int64_t num_parameters = 1`
: The size of the `weight` parameter. `1` means every channel shares a
  single learned slope. Any larger value creates one independent slope per
  channel -- see [`PReLU`](PReLU.md)'s per-channel broadcasting caveat for
  where that channel dimension needs to sit in the input's shape.

### `init`

`double init = 0.25`
: The value every entry of `weight` is set to by [`reset_parameters`
  ](PReLU.md), including the one implicitly called during construction.
  Matches the conventional PReLU default.

### `cfg`

`pluss::Config cfg = {}`
: The dtype and platform `weight` is allocated with. Defaults to the
  engine's default `Config` (Float32 on CPU). Set this to construct a
  `PReLU` whose parameter already lives on a specific platform, instead of
  constructing on CPU and moving the whole module afterward with `.to(...)`.

## Guidance

Leave `num_parameters=1` for a single shared slope (the common case, and
the cheapest to train). Set it to the channel count when per-channel slopes
are worth the extra parameters -- this only pays off with enough data per
channel to fit each slope reliably, and the input's channel axis must be
the *last* axis for the broadcast against `weight` to succeed (see
[`PReLU`](PReLU.md)).

## See Also

- [`PReLU`](PReLU.md) -- the only consumer of this options struct.
