#pragma once

#include "pluss/ai/ice/model.h"
#include "pluss/nn.h"

#include <utility>

namespace pluss::ai::ice {

// Ice's answer to pluss::nn::Schedule (NodeK's nn.Sequential-equivalent) —
// the one-liner way to compose existing layers into something trainable
// (ICE_BLUEPRINT.md §4/§6). Owns a real Schedule (composition, not
// inheritance); nothing about the layers is reimplemented, so dropping to
// raw `pluss::nn::Schedule` at any point costs nothing.
class Outline final : public Model {
public:
  template <class... Layers>
  explicit Outline(Layers&&... layers) : schedule_(std::forward<Layers>(layers)...) {
    register_module(*schedule_.get());
  }

  // Un-hides Model::forward(const Batch&) -- overriding one overload of a
  // base class's overload set hides the rest at this scope unless brought
  // back in explicitly, which would otherwise make the inherited, virtual
  // Batch overload uncallable through a concrete Outline (only reachable
  // through a Model&/Model* reference).
  using Model::forward;
  [[nodiscard]] Series forward(const Series& x) const override { return schedule_->forward(x); }

  [[nodiscard]] pluss::nn::Schedule& schedule() noexcept { return schedule_; }
  [[nodiscard]] const pluss::nn::Schedule& schedule() const noexcept { return schedule_; }

private:
  pluss::nn::Schedule schedule_;
};

} // namespace pluss::ai::ice
