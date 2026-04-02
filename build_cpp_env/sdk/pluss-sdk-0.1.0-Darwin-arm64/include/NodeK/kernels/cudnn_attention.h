#pragma once

#include <pluss/core.h>

#include <optional>
#include <string>

namespace pluss::ai::kernels::cudnn {

bool attention_route_enabled() noexcept;
bool attention_route_strict() noexcept;
bool attention_route_ready() noexcept;

bool supports_scaled_attention_softmax(
    const Series& logits,
    bool require_backward,
    std::string* reason = nullptr) noexcept;

std::optional<Series> try_scaled_attention_softmax(
    const Series& logits,
    bool require_backward);

} // namespace pluss::ai::kernels::cudnn
