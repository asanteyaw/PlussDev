# `pluss::ai::ice::io`

`io` owns the serialization primitives behind
[`Model::save`/`load`](model.md) and
[`Coach`](coach.md)'s checkpointing: a weights-only pair
([`save_parameters`](io/save_parameters.md)/
[`load_parameters`](io/load_parameters.md)) and a full,
training-resumable pair
([`save_checkpoint`](io/save_checkpoint.md)/
[`load_checkpoint`](io/load_checkpoint.md)), plus the
[`CheckpointState`](io/CheckpointState.md) payload type the latter two
exchange. Every one of these is a free function usable standalone, with
no `Model`/`Coach` required for the parameter-only pair.

## The Surface

| Entity | What it gives you |
| --- | --- |
| [`save_parameters`](io/save_parameters.md) / [`load_parameters`](io/load_parameters.md) | a versioned, checksummed weights-only artifact for any `std::vector<Series*>` -- what `Model::save`/`load` call internally |
| [`CheckpointState`](io/CheckpointState.md) | the recipe/epoch/step/best-metric/RNG/callback-state payload a full checkpoint carries, beyond parameters |
| [`save_checkpoint`](io/save_checkpoint.md) / [`load_checkpoint`](io/load_checkpoint.md) | the full, training-resumable artifact -- model weights, optimizer state, and `CheckpointState` in one file -- what `Coach::learn` calls internally for every checkpoint write/resume |

## How To Call It

```cpp
namespace ice = pluss::ai::ice;
using pluss::Config;
using pluss::ScalarType;
using pluss::Series;

const Config f32 = Config{}.with_dtype(ScalarType::Float32).with_platform(pluss::pCPU);
Series w = pluss::make_series({1.0F, 2.0F, 3.0F, 4.0F}, {2, 2}, f32);
Series b = pluss::make_series({0.5F}, {1}, f32);
ice::io::save_parameters("./io_params_demo.artifact", {&w, &b});

Series w2 = pluss::make_series({0.0F, 0.0F, 0.0F, 0.0F}, {2, 2}, f32);
Series b2 = pluss::make_series({0.0F}, {1}, f32);
ice::io::load_parameters("./io_params_demo.artifact", {&w2, &b2});

std::cout << "b2 = " << b2.item<float>() << "\n";
```

```text
b2 = 0.5
```

Most code never calls into `io` directly --
[`Model::save`/`load`](model.md) and
[`CheckpointPolicy`](options/CheckpointPolicy.md)-driven
[`Coach::learn`](coach.md) already do. Reach for `io` directly for a
hand-rolled training loop built on
[`make_optimizer`](optim/make_optimizer.md) that still wants real
checkpoint/resume support with no `Model`/`Coach` involved, or to
inspect a checkpoint's own metadata
([`CheckpointState::next_epoch`](io/CheckpointState.md)/`best_metric`/
etc.) without resuming a full run.

## Related Pages

- [`model`](model.md) -- `Model::save`/`load`, built on
  `save_parameters`/`load_parameters`.
- [`coach`](coach.md) -- `Coach::learn`'s own checkpoint write/resume,
  built on `save_checkpoint`/`load_checkpoint`.
- [`callback`](callback.md) -- `on_checkpoint_save`/`on_checkpoint_load`,
  the hooks that fire right around `Coach`'s own internal calls into
  this family.
- [`optim`](optim.md) -- `make_optimizer`, the standalone-loop
  companion to this family.

<!-- apiref:toctree:start -->
```{toctree}
:maxdepth: 2
:hidden:

io/Introduction
io/save_parameters
io/load_parameters
io/CheckpointState
io/save_checkpoint
io/load_checkpoint
```
<!-- apiref:toctree:end -->
