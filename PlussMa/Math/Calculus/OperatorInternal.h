#pragma once

#include "pluss/subsp/math/operators.h"

#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

namespace pluss::subsp::math::operators::detail {

enum class OperatorKind : std::uint8_t {
  Identity,
  Leaf,
  Composition,
};

class OperatorConcept : public pluss::msharp::RefCounted {
public:
  ~OperatorConcept() override = default;

  [[nodiscard]] virtual math::Function
  apply(const math::Function &input) const = 0;
  [[nodiscard]] virtual Capabilities capabilities() const noexcept = 0;
  [[nodiscard]] virtual std::string_view key() const noexcept = 0;
  [[nodiscard]] virtual std::uint32_t version() const noexcept = 0;
  [[nodiscard]] virtual OperatorKind kind() const noexcept = 0;
  [[nodiscard]] virtual std::string_view payload() const noexcept = 0;
  // Builds the formal adjoint, for operators that know it; null otherwise.
  [[nodiscard]] virtual const std::function<Operator()> *
  adjoint_factory() const noexcept {
    return nullptr;
  }
};

class LeafOperator final : public OperatorConcept {
public:
  LeafOperator(std::string key, std::uint32_t version,
               Capabilities capabilities,
               std::function<math::Function(const math::Function &)> apply,
               OperatorKind kind = OperatorKind::Leaf, std::string payload = {},
               std::function<Operator()> adjoint = {})
      : key_(std::move(key)), version_(version), capabilities_(capabilities),
        apply_(std::move(apply)), kind_(kind), payload_(std::move(payload)),
        adjoint_(std::move(adjoint)) {}
  [[nodiscard]] const std::function<Operator()> *
  adjoint_factory() const noexcept override {
    return adjoint_ ? &adjoint_ : nullptr;
  }

  [[nodiscard]] math::Function
  apply(const math::Function &input) const override {
    return apply_(input);
  }
  [[nodiscard]] Capabilities capabilities() const noexcept override {
    return capabilities_;
  }
  [[nodiscard]] std::string_view key() const noexcept override { return key_; }
  [[nodiscard]] std::uint32_t version() const noexcept override {
    return version_;
  }
  [[nodiscard]] OperatorKind kind() const noexcept override { return kind_; }
  [[nodiscard]] std::string_view payload() const noexcept override {
    return payload_;
  }

private:
  std::string key_{};
  std::uint32_t version_ = 1;
  Capabilities capabilities_{};
  std::function<math::Function(const math::Function &)> apply_{};
  OperatorKind kind_ = OperatorKind::Leaf;
  std::string payload_{};
  std::function<Operator()> adjoint_{};
};

class CompositionOperator final : public OperatorConcept {
public:
  CompositionOperator(std::vector<Operator> chain, Capabilities capabilities)
      : chain_(std::move(chain)), capabilities_(capabilities) {}

  [[nodiscard]] math::Function
  apply(const math::Function &input) const override;
  [[nodiscard]] Capabilities capabilities() const noexcept override {
    return capabilities_;
  }
  [[nodiscard]] std::string_view key() const noexcept override {
    return "compose";
  }
  [[nodiscard]] std::uint32_t version() const noexcept override { return 1; }
  [[nodiscard]] OperatorKind kind() const noexcept override {
    return OperatorKind::Composition;
  }
  [[nodiscard]] std::string_view payload() const noexcept override {
    return {};
  }
  [[nodiscard]] const std::vector<Operator> &chain() const noexcept {
    return chain_;
  }

private:
  std::vector<Operator> chain_{};
  Capabilities capabilities_{};
};

struct OperatorAccess final {
  [[nodiscard]] static Operator
  make(pluss::msharp::intrusive_ptr<OperatorConcept> impl);
  [[nodiscard]] static const OperatorConcept &get(const Operator &op);
  [[nodiscard]] static std::size_t component_count(const Operator &op);
  [[nodiscard]] static std::string_view payload(const Operator &op);
  [[nodiscard]] static const std::vector<Operator> &chain(const Operator &op);
  [[nodiscard]] static Operator restore(std::string_view key,
                                        std::uint32_t version,
                                        std::string_view payload);
  static void save(const Operator &op, std::ostream &output);
  [[nodiscard]] static Operator load(std::istream &input);
};

} // namespace pluss::subsp::math::operators::detail
