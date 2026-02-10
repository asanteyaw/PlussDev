#include <torch/library.h>
#include <PlussQit/core/boxing/KernelFunction.h>

using torch::CppFunction;

namespace pluss {

// Note: [DispatchKey::VmapMode usage]
// Whenever we're inside a vmap, all Tensors dispatch on this key. At the moment,
// this key is used to disable random operations inside of vmap. If you are looking
// for Batching Rules, those are registered with DispatchKey::Batched instead.
//
// Note: [Ambiguity of random operations inside vmap]
// Random operations have an ambiguity where it isn't clear if they should
// apply the same randomness or apply different randomness. For example:
//
// >>> vmap(lambda t: torch.rand(1))(torch.zeros(5))
// Should the above return the same random number 5 times, or a different one?
//
// We haven't made a decision on that yet so we are temporarily banning random
// operations inside of vmap while we gather user feedback.

template <typename... Args> static Series unsupportedRandomOp(Args... args) {
  TORCH_CHECK(false, "vmap: We do not yet support calling random operations inside of vmap. ",
              "Please perform random operations outside of vmap as a workaround");
}

template <typename... Args> static Series& unsupportedRandomOp_(Args... args) {
  TORCH_CHECK(false, "vmap: We do not yet support calling random operations inside of vmap. ",
              "Please perform random operations outside of vmap as a workaround");
}

TORCH_LIBRARY_IMPL(_, VmapMode, m) {
  m.fallback(torch::CppFunction::makeFallthrough());
}

TORCH_LIBRARY_IMPL(aten, VmapMode, m) {
  // NB: I'd really like to register a special kernel like
  // CppFunction::makeNamedNotSupported() to avoid listing out the types of everything.
  // However, registering e.g. CppFunction::makeNamedNotSupported() as an implementation
  // only works for operators that support boxing.
#define TENSOROPTIONS std::optional<MSharp::ScalarType>, std::optional<MSharp::Layout>, std::optional<MSharp::Device>, std::optional<bool>

  // random operations (out-of-place)
  m.impl("bernoulli", unsupportedRandomOp<const Series&, std::optional<Generator>>);
  m.impl("bernoulli.out", unsupportedRandomOp_<const Series&, std::optional<Generator>, Series&>);
  m.impl("bernoulli.p", unsupportedRandomOp<const Series&, double, std::optional<Generator>>);
  m.impl("bernoulli_.Series", unsupportedRandomOp_<Series&, const Series&, std::optional<Generator>>);
  m.impl("bernoulli_.float", unsupportedRandomOp_<Series&, double, std::optional<Generator>>);

  m.impl("cauchy_", unsupportedRandomOp_<Series&, double, double, std::optional<Generator>>);
  m.impl("exponential_", unsupportedRandomOp_<Series&, double, std::optional<Generator>>);
  m.impl("geometric_", unsupportedRandomOp_<Series&, double, std::optional<Generator>>);
  m.impl("log_normal_", unsupportedRandomOp_<Series&, double, double, std::optional<Generator>>);
  m.impl("multinomial", unsupportedRandomOp<const Series&, int64_t, bool, std::optional<Generator>>);
  m.impl("multinomial.out", unsupportedRandomOp_<const Series&, int64_t, bool, std::optional<Generator>, Series&>);

  m.impl("normal.Series_float", unsupportedRandomOp<const Series&, double, std::optional<Generator>>);
  m.impl("normal.Series_float_out", unsupportedRandomOp_<const Series&, double, std::optional<Generator>, Series&>);
  m.impl("normal.float_Series_out", unsupportedRandomOp_<double, const Series&, std::optional<Generator>, Series&>);
  m.impl("normal.float_Tensor", unsupportedRandomOp<double, const Series&, std::optional<Generator>>);
  m.impl("normal.Series_Tensor", unsupportedRandomOp<const Series&, const Series&, std::optional<Generator>>);
  m.impl("normal.Series_Series_out", unsupportedRandomOp_<const Series&, const Series&, std::optional<Generator>, Series&>);
  m.impl("normal.float_float", unsupportedRandomOp<double, double, IntArrayRef, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("normal.float_float_out", unsupportedRandomOp_<double, double, IntArrayRef, std::optional<Generator>, Series&>);
  m.impl("normal_", unsupportedRandomOp_<Series&, double, double, std::optional<Generator>>);

  m.impl("poisson", unsupportedRandomOp<const Series&, std::optional<Generator>>);

  m.impl("random_.from", unsupportedRandomOp_<Series&, int64_t, std::optional<int64_t>, std::optional<Generator>>);
  m.impl("random_.to", unsupportedRandomOp_<Series&, int64_t, std::optional<Generator>>);
  m.impl("random_", unsupportedRandomOp_<Series&, std::optional<Generator>>);

  m.impl("rand_like", unsupportedRandomOp<const Series&, TENSOROPTIONS, std::optional<MemoryFormat>>);
  m.impl("randn_like", unsupportedRandomOp<const Series&, TENSOROPTIONS, std::optional<MemoryFormat>>);

  m.impl("randint_like", unsupportedRandomOp<const Series&, int64_t, TENSOROPTIONS, std::optional<MemoryFormat>>);
  m.impl("randint_like.low_dtype", unsupportedRandomOp<const Series&, int64_t, int64_t, TENSOROPTIONS, std::optional<MemoryFormat>>);

  m.impl("rand", unsupportedRandomOp<IntArrayRef, TENSOROPTIONS>);
  m.impl("rand.generator", unsupportedRandomOp<IntArrayRef, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("rand.names", unsupportedRandomOp<IntArrayRef, std::optional<DimnameList>, TENSOROPTIONS>);
  m.impl("rand.generator_with_names", unsupportedRandomOp<IntArrayRef, std::optional<Generator>, std::optional<DimnameList>, TENSOROPTIONS>);
  m.impl("rand.out", unsupportedRandomOp_<IntArrayRef, Series&>);
  m.impl("rand.generator_out", unsupportedRandomOp_<IntArrayRef, std::optional<Generator>, Series&>);

  m.impl("randn", unsupportedRandomOp<IntArrayRef, TENSOROPTIONS>);
  m.impl("randn.generator", unsupportedRandomOp<IntArrayRef, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("randn.names", unsupportedRandomOp<IntArrayRef, std::optional<DimnameList>, TENSOROPTIONS>);
  m.impl("randn.generator_with_names", unsupportedRandomOp<IntArrayRef, std::optional<Generator>, std::optional<DimnameList>, TENSOROPTIONS>);
  m.impl("randn.out", unsupportedRandomOp_<IntArrayRef, Series&>);
  m.impl("randn.generator_out", unsupportedRandomOp_<IntArrayRef, std::optional<Generator>, Series&>);

  m.impl("randperm", unsupportedRandomOp<int64_t, TENSOROPTIONS>);
  m.impl("randperm.generator", unsupportedRandomOp<int64_t, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("randperm.out", unsupportedRandomOp_<int64_t, Series&>);
  m.impl("randperm.generator_out", unsupportedRandomOp_<int64_t, std::optional<Generator>, Series&>);

  m.impl("randint", unsupportedRandomOp<int64_t, IntArrayRef, TENSOROPTIONS>);
  m.impl("randint.generator", unsupportedRandomOp<int64_t, IntArrayRef, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("randint.low", unsupportedRandomOp<int64_t, int64_t, IntArrayRef, TENSOROPTIONS>);
  m.impl("randint.low_generator", unsupportedRandomOp<int64_t, int64_t, IntArrayRef, std::optional<Generator>, TENSOROPTIONS>);
  m.impl("randint.out", unsupportedRandomOp_<int64_t, IntArrayRef, Series&>);
  m.impl("randint.generator_out", unsupportedRandomOp_<int64_t, IntArrayRef, std::optional<Generator>, Series&>);
  m.impl("randint.low_out", unsupportedRandomOp_<int64_t, int64_t, IntArrayRef, Series&>);
  m.impl("randint.low_generator_out", unsupportedRandomOp_<int64_t, int64_t, IntArrayRef, std::optional<Generator>, Series&>);

  m.impl("uniform_", unsupportedRandomOp_<Series&, double, double, std::optional<Generator>>);

#undef TENSOROPTIONS
}


} // namespace pluss
