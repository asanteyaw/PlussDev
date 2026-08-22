#pragma once

#include "pluss/nn.h"

namespace pluss::ai::ice {

// Enum, not a bool: a plain "use_gpu" flag can't distinguish "let ice pick
// the best available device" from "I explicitly want acceleration, tell me
// if you can't give it to me" — those two intents warrant different
// fallback behavior (silent vs. warned), so they get distinct enumerators.
enum class TrainPlatform : std::uint8_t {
  // Always trains on CPU, regardless of what's available. The default —
  // matches ice's behavior before this option existed, so opting in to GPU
  // acceleration is something a caller always does deliberately.
  CPU = 0,
  // Explicit request for acceleration: CUDA if available, else MPS, else
  // pluss::validate::warn(...) and fall back to CPU. Use this when the
  // absence of a GPU is worth telling the caller about.
  GPU = 1,
  // Best-effort: CUDA if available, else MPS, else CPU — same fallback
  // order as GPU, but quiet about it. Use this when either device is
  // genuinely fine and a warning would just be noise.
  Auto = 2,
};

// Resolves `requested` against a real runtime query of this machine
// (pluss::cuda::is_ready() / pluss::mps::is_ready() — not a compile-time
// guess), independent of whatever platform the caller's data currently
// lives on: pluss::ai::ice::Model::learn() moves both the model and the
// training data to the resolved platform for you.
//
// There is no way to select a specific GPU by index, and no distributed or
// multi-GPU training support — the underlying engine (NodeK/CodeM) has no
// per-device addressing or collective-communication backend yet, so there
// is nothing here for a convenience layer to wrap. See
// PlussAI/Ice/ICE_BLUEPRINT.md for the full reasoning.
[[nodiscard]] Platform resolve_platform(TrainPlatform requested);

} // namespace pluss::ai::ice
