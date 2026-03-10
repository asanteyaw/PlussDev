#pragma once

#include "pluss/ai/nodej/families.h"
#include "pluss/ai/nodej/interfaces.h"

#include <memory>

namespace pluss::ai::nodej {

StatusOr<std::unique_ptr<ClassifierBase>> make_classifier(const ModelSpec& spec);
StatusOr<std::unique_ptr<RegressorBase>> make_regressor(const ModelSpec& spec);

} // namespace pluss::ai::nodej
