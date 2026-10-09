<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3055a93796b3 -->
# `pluss::ai::ice::Callback::on_checkpoint_save`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_checkpoint_save(pluss::ai::ice::RunState& state, std::string_view filepath);
```

## What It Does

Fires right after a checkpoint has actually been written to disk --
once per [`CheckpointPolicy::save_last`](../../options/CheckpointPolicy.md)
write (every `every_n_epochs` epochs) and once per `save_best` write
(every improving validation epoch), independently, so both can fire in
the same epoch. Never fires at all if
[`CheckpointPolicy::enabled`](../../options/CheckpointPolicy.md) is
`false`. The default implementation does nothing -- override it to log
checkpoint activity, upload the file somewhere, or prune old
checkpoints.

## Parameters

`RunState& state`
: `state.checkpoint_path` and `state.last_checkpoint_path` are both
  already updated to `filepath` by the time this fires (even for a
  `save_best` write -- `checkpoint_path` always tracks whichever write
  happened most recently, of either kind).

`std::string_view filepath`
: The path just written to -- `<dirpath>/<filename>-best.ckpt` or
  `<dirpath>/<filename>-last.ckpt`, per
  [`CheckpointPolicy::dirpath`/`filename`](../../options/CheckpointPolicy.md).

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: `save_last` firing once per epoch, `save_best` and
`save_last` both enabled firing independently with different suffixes,
and confirming `RunState`'s own checkpoint-path fields already match
`filepath` by this point.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogCheckpointSaveCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_checkpoint_save(pluss::ai::ice::RunState& state, std::string_view filepath) override {
    ++fire_count;
    std::cout << "on_checkpoint_save: epoch = " << state.epoch
               << ", filepath = " << filepath << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 2;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.dirpath = "./ice_checkpoint_demo";
  options.checkpoint.filename = "demo";

  const auto logger = std::make_shared<LogCheckpointSaveCallback>();
  ice::Coach coach(options);
  coach.add_callback(logger);
  coach.learn(model, x, y);

  std::cout << "on_checkpoint_save fired " << logger->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_checkpoint_save: epoch = 0, filepath = ./ice_checkpoint_demo/demo-last.ckpt
on_checkpoint_save: epoch = 1, filepath = ./ice_checkpoint_demo/demo-last.ckpt
on_checkpoint_save fired 2 time(s)
```

### Example 2

With both `save_best` and `save_last` enabled over two epochs where
every epoch improves the monitored metric, each epoch fires this hook
twice -- once for each write, with a different suffix.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogCheckpointSaveCallback final : public pluss::ai::ice::Callback {
 public:
  void on_checkpoint_save(pluss::ai::ice::RunState&, std::string_view filepath) override {
    std::cout << "on_checkpoint_save: filepath = " << filepath << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 2;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.5;
  options.shuffle = false;
  options.checkpoint.enabled = true;
  options.checkpoint.save_best = true;
  options.checkpoint.save_last = true;
  options.checkpoint.dirpath = "./ice_checkpoint_demo";
  options.checkpoint.filename = "demo";

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogCheckpointSaveCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_checkpoint_save: filepath = ./ice_checkpoint_demo/demo-best.ckpt
on_checkpoint_save: filepath = ./ice_checkpoint_demo/demo-last.ckpt
on_checkpoint_save: filepath = ./ice_checkpoint_demo/demo-best.ckpt
on_checkpoint_save: filepath = ./ice_checkpoint_demo/demo-last.ckpt
```

### Example 3

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogCheckpointSaveCallback final : public pluss::ai::ice::Callback {
 public:
  void on_checkpoint_save(pluss::ai::ice::RunState& state, std::string_view filepath) override {
    std::cout << "on_checkpoint_save: state.checkpoint_path == filepath: "
               << (state.checkpoint_path == filepath)
               << ", state.last_checkpoint_path == filepath: "
               << (state.last_checkpoint_path == filepath) << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions options{};
  options.execution.max_epochs = 1;
  options.learning_rate = 0.1;
  options.validation.split_ratio = 0.0;
  options.checkpoint.enabled = true;
  options.checkpoint.save_last = true;
  options.checkpoint.dirpath = "./ice_checkpoint_demo";
  options.checkpoint.filename = "demo";

  ice::Coach coach(options);
  coach.add_callback(std::make_shared<LogCheckpointSaveCallback>());
  coach.learn(model, x, y);

  return 0;
}
```

Output

```text
on_checkpoint_save: state.checkpoint_path == filepath: 1, state.last_checkpoint_path == filepath: 1
```

## See Also

- [`on_checkpoint_load`](on_checkpoint_load.md) -- the matching hook
  when resuming from one of these files.
- [`CheckpointPolicy`](../../options/CheckpointPolicy.md) -- every field
  that controls when and where this fires.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_checkpoint_load.html">
    <span class="label">Previous</span>
    <span class="title">on_checkpoint_load</span>
  </a>
  <a class="next-link" href="on_epoch_begin.html">
    <span class="label">Next</span>
    <span class="title">on_epoch_begin</span>
  </a>
</div>
<!-- apiref:nav:end -->
