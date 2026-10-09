<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 85c99b011399 -->
# `pluss::ai::ice::Callback::on_checkpoint_load`

[Back to `pluss::ai::ice::Callback`](../Callback.md)

## Signature

```cpp
virtual void pluss::ai::ice::Callback::on_checkpoint_load(pluss::ai::ice::RunState& state, std::string_view filepath);
```

## What It Does

Fires exactly once, right after
[`CheckpointPolicy::resume_from`](../../options/CheckpointPolicy.md)
has been loaded -- after [`on_run_begin`](on_run_begin.md), before the
first (resumed) epoch's [`on_epoch_begin`](on_epoch_begin.md). Never
fires at all when `resume_from` is empty (the default, a fresh run).
The default implementation does nothing -- override it to log a resume
event, or to react to state this callback itself persisted via
[`save_state`](save_state.md) and had restored through
[`load_state`](load_state.md) just before this fires.

## Parameters

`RunState& state`
: Already carries the resumed run's state -- `state.epoch` is
  `next_epoch - 1` from the checkpoint (the epoch loop's own starting
  point, one before the first epoch that will actually run),
  `state.global_step`/`state.best_metric`/the checkpoint-path fields
  are all restored too.

`std::string_view filepath`
: The same path as `CheckpointPolicy::resume_from`.

## Returns

`void`.

## Errors

None from the base class; whatever an override throws propagates out
of the run and triggers [`on_exception`](on_exception.md) instead.

## Practical Examples

Three examples: a real resume firing this hook once, confirming it
never fires when `resume_from` is empty, and `state.epoch` already
reflecting where the resumed run picks up.

### Example 1

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogCheckpointLoadCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_checkpoint_load(pluss::ai::ice::RunState&, std::string_view filepath) override {
    ++fire_count;
    std::cout << "on_checkpoint_load: filepath = " << filepath << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions save_options{};
  save_options.execution.max_epochs = 2;
  save_options.learning_rate = 0.1;
  save_options.validation.split_ratio = 0.0;
  save_options.checkpoint.enabled = true;
  save_options.checkpoint.save_last = true;
  save_options.checkpoint.dirpath = "./ice_checkpoint_demo";
  save_options.checkpoint.filename = "demo";

  ice::Coach(save_options).learn(model, x, y);

  // Resume from the checkpoint the first run wrote.
  ice::Outline resumed_model(nn::Fusion(2, 1));
  ice::TrainOptions resume_options = save_options;
  resume_options.execution.max_epochs = 3;
  resume_options.checkpoint.resume_from = "./ice_checkpoint_demo/demo-last.ckpt";

  const auto logger = std::make_shared<LogCheckpointLoadCallback>();
  ice::Coach resume_coach(resume_options);
  resume_coach.add_callback(logger);
  resume_coach.learn(resumed_model, x, y);

  std::cout << "on_checkpoint_load fired " << logger->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_checkpoint_load: filepath = ./ice_checkpoint_demo/demo-last.ckpt
on_checkpoint_load fired 1 time(s)
```

### Example 2

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class CountCheckpointLoadCallback final : public pluss::ai::ice::Callback {
 public:
  int fire_count = 0;
  void on_checkpoint_load(pluss::ai::ice::RunState&, std::string_view) override { ++fire_count; }
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
  // resume_from left empty -- no checkpoint to resume from.

  const auto counter = std::make_shared<CountCheckpointLoadCallback>();
  ice::Coach coach(options);
  coach.add_callback(counter);
  coach.learn(model, x, y);

  std::cout << "on_checkpoint_load fired " << counter->fire_count << " time(s)\n";

  return 0;
}
```

Output

```text
on_checkpoint_load fired 0 time(s)
```

### Example 3

The first run trains epochs `0` and `1`, writing `next_epoch = 2` into
the checkpoint. On resume, `state.epoch` is `1` (`next_epoch - 1`) at
`on_checkpoint_load`, and the very next hook,
[`on_epoch_begin`](on_epoch_begin.md), already reports epoch `2` -- the
resumed run never repeats an epoch that already ran.

```cpp
#include <pluss/pluss.h>
#include <pluss/ai/ice.h>
#include <pluss/nn.h>

#include <iostream>
#include <memory>

namespace {

class LogCheckpointLoadStateCallback final : public pluss::ai::ice::Callback {
 public:
  void on_checkpoint_load(pluss::ai::ice::RunState& state, std::string_view) override {
    std::cout << "on_checkpoint_load: epoch = " << state.epoch << "\n";
  }
  void on_epoch_begin(pluss::ai::ice::RunState& state) override {
    std::cout << "on_epoch_begin:     epoch = " << state.epoch << "\n";
  }
};

} // namespace

int main() {
  namespace ice = pluss::ai::ice;
  namespace nn = pluss::nn;

  ice::Outline model(nn::Fusion(2, 1));
  const pluss::Series x = pluss::make_series({0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f}, {4, 2});
  const pluss::Series y = pluss::make_series({0.0f, 1.0f, 1.0f, 2.0f}, {4, 1});

  ice::TrainOptions save_options{};
  save_options.execution.max_epochs = 2;  // trains epoch 0, epoch 1
  save_options.learning_rate = 0.1;
  save_options.validation.split_ratio = 0.0;
  save_options.checkpoint.enabled = true;
  save_options.checkpoint.save_last = true;
  save_options.checkpoint.dirpath = "./ice_checkpoint_demo";
  save_options.checkpoint.filename = "demo";

  ice::Coach(save_options).learn(model, x, y);

  ice::Outline resumed_model(nn::Fusion(2, 1));
  ice::TrainOptions resume_options = save_options;
  resume_options.execution.max_epochs = 3;
  resume_options.checkpoint.resume_from = "./ice_checkpoint_demo/demo-last.ckpt";

  ice::Coach resume_coach(resume_options);
  resume_coach.add_callback(std::make_shared<LogCheckpointLoadStateCallback>());
  resume_coach.learn(resumed_model, x, y);

  return 0;
}
```

Output

```text
on_checkpoint_load: epoch = 1
on_epoch_begin:     epoch = 2
```

## See Also

- [`on_checkpoint_save`](on_checkpoint_save.md) -- what writes the file
  this hook loads back.
- [`CheckpointPolicy::resume_from`](../../options/CheckpointPolicy.md) --
  the field that triggers this hook at all.
- [`load_state`](load_state.md) -- a callback's own state, restored
  right before this fires.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->

<!-- apiref:nav:start -->
<div class="pluss-inline-prev-next">
  <a class="prev-link" href="on_before_step.html">
    <span class="label">Previous</span>
    <span class="title">on_before_step</span>
  </a>
  <a class="next-link" href="on_checkpoint_save.html">
    <span class="label">Next</span>
    <span class="title">on_checkpoint_save</span>
  </a>
</div>
<!-- apiref:nav:end -->
