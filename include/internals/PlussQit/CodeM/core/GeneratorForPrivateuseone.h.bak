#pragma once

#include <PlussQit/core/Generator.h>
#include <MSharp/util/intrusive_ptr.h>

namespace pluss {

using GeneratorFuncType = std::function<pluss::Generator(MSharp::DeviceIndex)>;

TORCH_API std::optional<GeneratorFuncType>& GetGeneratorPrivate();

class TORCH_API _GeneratorRegister {
 public:
  explicit _GeneratorRegister(const GeneratorFuncType& func);
};

TORCH_API pluss::Generator GetGeneratorForPrivateuse1(
    MSharp::DeviceIndex device_index);

/**
 * This is used to register Generator to PyTorch for `privateuse1` key.
 *
 * Usage: REGISTER_GENERATOR_PRIVATEUSE1(MakeGeneratorForPrivateuse1)
 *
 * class CustomGeneratorImpl : public MSharp::GeneratorImpl {
 *   CustomGeneratorImpl(DeviceIndex device_index = -1);
 *   explicit ~CustomGeneratorImpl() override = default;
 *   ...
 * };
 *
 * pluss::Generator MakeGeneratorForPrivateuse1(MSharp::DeviceIndex id) {
 *   return pluss::make_generator<CustomGeneratorImpl>(id);
 * }
 */

#define REGISTER_GENERATOR_PRIVATEUSE1(GeneratorPrivate) \
  static auto temp##GeneratorPrivate = pluss::_GeneratorRegister(GeneratorPrivate);

} // namespace pluss
