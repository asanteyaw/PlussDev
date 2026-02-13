namespace pluss::codem::factories {
  
Series empty(std::span<const std::int64_t> sizes, Config cfg) { return empty(sizes, cfg.dtype); }
Series zeros(std::span<const std::int64_t> sizes, Config cfg) { return zeros(sizes, cfg.dtype); }
Series ones(std::span<const std::int64_t> sizes, Config cfg) { return ones(sizes, cfg.dtype); }
Series full(std::span<const std::int64_t> sizes, double value, Config cfg) { return full(sizes, value, cfg.dtype); }

Series rand(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) { return rand(sizes, cfg.dtype, seed); }
Series randn(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) { return randn(sizes, cfg.dtype, seed); }
Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed) {
  if (cfg.dtype == ScalarType::Int64 || cfg.dtype == ScalarType::Int32 || cfg.dtype == ScalarType::Int16 ||
      cfg.dtype == ScalarType::Int8 || cfg.dtype == ScalarType::UInt8 || cfg.dtype == ScalarType::Bool) {
    return randint(lo, hi, sizes, cfg.dtype, seed);
  }
  Series tmp = randint(lo, hi, sizes, ScalarType::Int64, seed);
  return tmp.to(cfg.dtype);
}

Series linspace(double start, double end, std::int64_t steps, Config cfg) {
  PLUSS_CALL_GUARD();
  PLUSS_CHECK(steps >= 0, "linspace: steps must be >= 0");
  if (steps == 0) return empty({0}, cfg.dtype);

  if (cfg.dtype == ScalarType::Undefined) cfg.dtype = ScalarType::Float32;

  // Float8 generation is intentionally not supported in v1 (needs quantization policy).
  PLUSS_CHECK(cfg.dtype != ScalarType::Float8E4M3FN && cfg.dtype != ScalarType::Float8E5M2,
              "linspace: Float8 output not supported yet (use Float16/Float32)");

  Series out = empty({steps}, cfg.dtype);
  auto oc = out.contiguous();
  const double denom = (steps <= 1) ? 1.0 : double(steps - 1);

  const auto n = steps;
  switch (cfg.dtype) {
    case ScalarType::Float64: {
      auto* p = reinterpret_cast<double*>(oc.raw_data());
    #if defined(__clang__)
    #pragma clang loop vectorize(enable)
    #pragma clang loop interleave(enable)
    #endif
      for (std::int64_t i=0;i<n;++i) p[i] = start + (end - start) * (double(i) / denom);
      break;
    }
    case ScalarType::Float32: {
      auto* p = reinterpret_cast<float*>(oc.raw_data());
    #if defined(__clang__)
    #pragma clang loop vectorize(enable)
    #pragma clang loop interleave(enable)
    #endif
      for (std::int64_t i=0;i<n;++i) p[i] = (float)(start + (end - start) * (double(i) / denom));
      break;
    }
    case ScalarType::Float16: {
      auto* p = reinterpret_cast<Float16*>(oc.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = float16_from_float((float)(start + (end - start) * (double(i) / denom)));
      break;
    }
    case ScalarType::BFloat16: {
      auto* p = reinterpret_cast<BFloat16*>(oc.raw_data());
      for (std::int64_t i=0;i<n;++i) p[i] = bfloat16_from_float((float)(start + (end - start) * (double(i) / denom)));
      break;
    }
    case ScalarType::Complex64: {
      auto* p = reinterpret_cast<Complex64*>(oc.raw_data());
      for (std::int64_t i=0;i<n;++i) {
        const float v = (float)(start + (end - start) * (double(i) / denom));
        p[i] = Complex64(v, 0.0f);
      }
      break;
    }
    case ScalarType::Complex128: {
      auto* p = reinterpret_cast<Complex128*>(oc.raw_data());
      for (std::int64_t i=0;i<n;++i) {
        const double v = start + (end - start) * (double(i) / denom);
        p[i] = Complex128(v, 0.0);
      }
      break;
    }
    default: {
      // integer/bool: cast the double values
      const auto info = dtype_info(cfg.dtype);
      PLUSS_CHECK(info.itemsize > 0, "linspace: invalid dtype");
      for (std::int64_t i=0;i<n;++i) {
        const double v = start + (end - start) * (double(i) / denom);
        oc.mutable_item_from_double(i, v);
      }
      break;
    }
  }
  return oc;
}


Series eye(std::int64_t n, std::int64_t m, Config cfg) {
  PLUSS_CALL_GUARD();
  if (m < 0) m = n;
  if (cfg.dtype == ScalarType::Undefined) cfg.dtype = ScalarType::Float32;

  PLUSS_CHECK(cfg.dtype != ScalarType::Float8E4M3FN && cfg.dtype != ScalarType::Float8E5M2,
              "eye: Float8 output not supported yet (use Float16/Float32)");

  Series out = zeros({n, m}, cfg.dtype);
  auto oc = out.contiguous();
  const auto diag = (n < m) ? n : m;

  // Write ones along the diagonal in a contiguous matrix.
  // For non-float types, we still use the same scalar setter.
  for (std::int64_t i=0;i<diag;++i) {
    const std::int64_t off = i*m + i;
    oc.mutable_item_from_double(off, 1.0);
  }
  return oc;
}

Series zeros_like(const Series& a, Config cfg) {
  if (cfg.dtype == ScalarType::Undefined) cfg.dtype = a.scalar_type();
  return zeros(a.sizes(), cfg.dtype);
}
Series ones_like(const Series& a, Config cfg) {
  if (cfg.dtype == ScalarType::Undefined) cfg.dtype = a.scalar_type();
  return ones(a.sizes(), cfg.dtype);
}
Series full_like(const Series& a, double v, Config cfg) {
  if (cfg.dtype == ScalarType::Undefined) cfg.dtype = a.scalar_type();
  return full(a.sizes(), v, cfg.dtype);
}

Series zeros_like(const Series& a, Config cfg) { return zeros(a.sizes(), cfg.dtype); }
Series ones_like(const Series& a, Config cfg) { return ones(a.sizes(), cfg.dtype); }
Series full_like(const Series& a, double v, Config cfg) { return full(a.sizes(), v, cfg.dtype); }

} // namespace pluss::codem::factories