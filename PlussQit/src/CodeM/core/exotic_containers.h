// ----- Parameter / reference types (move to a shared header) -----
namespace pluss::codem {
  template <class T>
  using ArrayRef = std::span<const T>;

  using IntArrayRef = ArrayRef<std::int64_t>;

  // Minimal symbolic int model (forward-compatible with true SymInt later).
  // You can later replace this with a richer SymInt (e.g. holding a variant of
  // int64_t and a symbolic node pointer).
  struct SymInt {
    constexpr SymInt() noexcept = default;
    constexpr SymInt(std::int64_t v) noexcept : value(v) {}
    constexpr operator std::int64_t() const noexcept { return value; }
    std::int64_t value{0};
  };

  using SymIntArrayRef = ArrayRef<SymInt>;
  using OptionalIntArrayRef = std::optional<IntArrayRef>;
  using OptionalSymIntArrayRef = std::optional<SymIntArrayRef>;
} // namespace pluss::codem