# Deterministic Seeds API Reference

[Back to Core Utilities](../core.md)

The seed family coordinates reproducible random work across convenience
factories that do not receive their own nonzero seed. It holds one process-wide
base seed and a stream position. Resetting the base also resets that position;
requesting a derived seed advances the position without changing the base.

This is seed allocation, not random sampling. Each derived value is intended to
initialize an independent generator, split, sampler, or model component. The
generator then produces its own sequence without returning to this shared
state for every element.

## Choosing an operation

| Operation | Purpose | Advances the shared stream |
|---|---|---|
| [`next_rng_seed`](../core-seed/next_rng_seed.md) | Allocate the next deterministic derived seed | Yes |
| [`rng_seed`](../core-seed/rng_seed.md) | Read the base, or reset the base and stream | Setter only resets; getter does not advance |
| [`seed_or_next`](../core-seed/seed_or_next.md) | Preserve a nonzero explicit seed, or allocate when the argument is zero | Only for zero |

The state is shared by all threads in the process. Allocation and reset are
serialized, so callers never observe a base seed paired with another
generation's stream position. Concurrent allocations are distinct, although
thread scheduling determines which worker receives each position. Assign
seeds before launching order-sensitive work when worker-to-seed identity must
also be reproducible.

```{toctree}
:maxdepth: 1
:hidden:

../core-seed/next_rng_seed
../core-seed/rng_seed
../core-seed/seed_or_next
```
