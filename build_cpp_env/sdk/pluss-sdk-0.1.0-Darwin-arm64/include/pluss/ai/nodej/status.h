#pragma once

#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace pluss::ai::nodej {

enum class StatusCode : unsigned char {
  Ok = 0,
  InvalidArgument = 1,
  Precondition = 2,
  ShapeMismatch = 3,
  NotImplemented = 4,
  NumericalFailure = 5,
  Internal = 6,
};

struct Status {
  StatusCode code = StatusCode::Ok;
  std::string message{};

  [[nodiscard]] bool ok() const noexcept { return code == StatusCode::Ok; }

  static Status success() { return Status{}; }
  static Status invalid_argument(std::string msg) {
    return Status{StatusCode::InvalidArgument, std::move(msg)};
  }
  static Status precondition(std::string msg) {
    return Status{StatusCode::Precondition, std::move(msg)};
  }
  static Status shape_mismatch(std::string msg) {
    return Status{StatusCode::ShapeMismatch, std::move(msg)};
  }
  static Status not_implemented(std::string msg) {
    return Status{StatusCode::NotImplemented, std::move(msg)};
  }
  static Status numerical_failure(std::string msg) {
    return Status{StatusCode::NumericalFailure, std::move(msg)};
  }
  static Status internal(std::string msg) {
    return Status{StatusCode::Internal, std::move(msg)};
  }
};

template <class T>
class StatusOr {
public:
  StatusOr(Status status, T value)
      : status_(std::move(status)) {
    if (status_.ok()) value_.emplace(std::move(value));
  }

  explicit StatusOr(T value) : status_(Status::success()), value_(std::move(value)) {}
  explicit StatusOr(Status status) : status_(std::move(status)) {}

  [[nodiscard]] bool ok() const noexcept { return value_.has_value() && status_.ok(); }
  [[nodiscard]] const Status& status() const noexcept { return status_; }
  [[nodiscard]] const T& value() const {
    if (!value_) throw std::logic_error("StatusOr::value(): result has no value");
    return *value_;
  }
  [[nodiscard]] T& value() {
    if (!value_) throw std::logic_error("StatusOr::value(): result has no value");
    return *value_;
  }
  [[nodiscard]] T&& move_value() {
    if (!value_) throw std::logic_error("StatusOr::move_value(): result has no value");
    return std::move(*value_);
  }

private:
  Status status_{};
  std::optional<T> value_{};
};

} // namespace pluss::ai::nodej
