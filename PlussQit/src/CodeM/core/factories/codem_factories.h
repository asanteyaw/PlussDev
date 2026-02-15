namespace pluss {
// --------------------------------------------------------------------------
  // Factories (create NEW Series). Ops on existing Series live as methods.
  // --------------------------------------------------------------------------

  inline Series empty(std::span<const std::int64_t> sizes, ScalarType dt) { return codem::empty(sizes, dt); }
  inline Series empty(std::span<const std::int64_t> sizes, Config cfg) { return codem::empty(sizes, cfg); }
  template <class... Opts>
  inline Series empty(std::span<const std::int64_t> sizes, Opts... opts) {
    return codem::empty(sizes, detail::make_config(opts...));
  }
  template <class T>
  inline Series empty(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return codem::empty(sizes, pluss::config(kv));
  }

  inline Series zeros(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return codem::zeros(sizes, dt); }
  inline Series zeros(std::span<const std::int64_t> sizes, Config cfg) { return codem::zeros(sizes, cfg); }
  template <class... Opts>
  inline Series zeros(std::span<const std::int64_t> sizes, Opts... opts) {
    return codem::zeros(sizes, detail::make_config(opts...));
  }
  template <class T>
  inline Series zeros(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return codem::zeros(sizes, pluss::config(kv));
  }

  inline Series ones(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32) { return codem::ones(sizes, dt); }
  inline Series ones(std::span<const std::int64_t> sizes, Config cfg) { return codem::ones(sizes, cfg); }
  template <class... Opts>
  inline Series ones(std::span<const std::int64_t> sizes, Opts... opts) {
    return codem::ones(sizes, detail::make_config(opts...));
  }
  template <class T>
  inline Series ones(std::span<const std::int64_t> sizes, std::initializer_list<std::pair<const char*, T>> kv) {
    return codem::ones(sizes, pluss::config(kv));
  }

  inline Series full(std::span<const std::int64_t> sizes, double fill_value, ScalarType dt = ScalarType::Float32) { return codem::full(sizes, fill_value, dt); }
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Config cfg) { return codem::full(sizes, fill_value, cfg); }
  template <class... Opts>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, Opts... opts) {
    return codem::full(sizes, fill_value, detail::make_config(opts...));
  }
  template <class T>
  inline Series full(std::span<const std::int64_t> sizes, double fill_value, std::initializer_list<std::pair<const char*, T>> kv) {
    return codem::full(sizes, fill_value, pluss::config(kv));
  }

  // Scalar constants (factories)
  inline Series nan(ScalarType dt = ScalarType::Float32) { return codem::nan(dt); }
  inline Series inf(ScalarType dt = ScalarType::Float32) { return codem::inf(dt); }

  // Ranges / random
  inline Series arange(std::int64_t start, std::int64_t end, std::int64_t step = 1, ScalarType dt = ScalarType::Int64) { return codem::arange(start, end, step, dt); }
  inline Series linspace(double start, double end, std::int64_t steps, ScalarType dt = ScalarType::Float32) { return codem::linspace(start, end, steps, dt); }
  inline Series eye(std::int64_t n, std::int64_t m = -1, ScalarType dt = ScalarType::Float32) { return codem::eye(n, m, dt); }

  inline Series rand(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return codem::rand(sizes, dt, seed); }
  inline Series rand(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) { return codem::rand(sizes, cfg, seed); }
  template <class... Opts>
  inline Series rand(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return codem::rand(sizes, detail::make_config(opts...), seed);
  }

  inline Series randn(std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Float32, std::uint64_t seed = 0) { return codem::randn(sizes, dt, seed); }
  inline Series randn(std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) { return codem::randn(sizes, cfg, seed); }
  template <class... Opts>
  inline Series randn(std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return codem::randn(sizes, detail::make_config(opts...), seed);
  }

  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, ScalarType dt = ScalarType::Int64, std::uint64_t seed = 0) { return codem::randint(lo, hi, sizes, dt, seed); }
  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, Config cfg, std::uint64_t seed = 0) { return codem::randint(lo, hi, sizes, cfg, seed); }
  template <class... Opts>
  inline Series randint(std::int64_t lo, std::int64_t hi, std::span<const std::int64_t> sizes, std::uint64_t seed, Opts... opts) {
    return codem::randint(lo, hi, sizes, detail::make_config(opts...), seed);
  }

#ifdef PLUSS_ENABLE_TORCH
  #include <torch/torch.h>
  inline Series from_torch(const torch::Tensor& t) { return codem::from_torch(t); }
#endif

  // --------------------------------------------------------------------------
  // Value / blob / tensor-like factories (create NEW Series from user data)
  // --------------------------------------------------------------------------

  // Scalars:
  inline Series value(double v, ScalarType dtype = ScalarType::Float32) { return codem::value(v, dtype); }
  template <class T, std::enable_if_t<std::is_integral_v<T> && !std::is_same_v<T, bool>, int> = 0>
  inline Series value(T v, ScalarType dtype = ScalarType::Int64) { return codem::value(v, dtype); }
  inline Series value(bool v, ScalarType dtype = ScalarType::Bool) { return codem::value(v, dtype); }

  // Nested initializer_list: dtype auto-deduction when user doesn't specify.
  template <class T>
  inline Series value(std::initializer_list<T> list) {
    constexpr ScalarType dt = pluss::detail::dtype_from_init_list<std::initializer_list<T>>();
    return codem::value(list, dt);
  }
  template <class T>
  inline Series value(std::initializer_list<T> list, ScalarType dtype) {
    return codem::value(list, dtype);
  }

  // tensor(...) convenience: from std::span / std::vector / std::array with element-type deduction.
  template <class T>
  inline Series tensor(std::span<const T> data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series tensor(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T, std::size_t N>
  inline Series tensor(const std::array<T, N>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(std::span<const T>(data.data(), N), sizes, cfg);
  }

  // Backwards-compatible name: make_series (deduced T)
  template <class T>
  inline Series make_series(const std::vector<T>& data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series make_series(std::span<const T> data, std::span<const std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }
  template <class T>
  inline Series make_series(std::span<const T> data, std::initializer_list<std::int64_t> sizes, Config cfg = {}) {
    return codem::make_series<T>(data, sizes, cfg);
  }

  inline Series from_blob(void* data,
                          std::span<const std::int64_t> sizes,
                          std::span<const std::int64_t> strides,
                          BlobDeleter deleter,
                          ScalarType dtype = ScalarType::Float32) {
    return codem::from_blob(data, sizes, strides, deleter, dtype);
  }
}