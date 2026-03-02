#pragma once

#include <pluss/nn_detail.h>

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace pluss::ai::nn {

class PLUSS_API Module {
public:
  explicit Module(bool training = true) noexcept : training_(training) {}
  virtual ~Module() = default;

  bool is_training() const noexcept { return training_; }

  virtual void train(bool enabled = true) noexcept {
    training_ = enabled;
    on_training_changed_(enabled);
    for (auto& child : children_) {
      child.module->train(enabled);
    }
  }

  void eval() noexcept { train(false); }

  Module& to(Platform platform) {
    for (auto& param : params_) {
      if (param.series->defined()) {
        *param.series = param.series->to(platform);
      }
    }
    for (auto& buffer : buffers_) {
      if (buffer.series->defined()) {
        *buffer.series = buffer.series->to(platform);
      }
    }
    on_platform_changed_(platform);
    for (auto& child : children_) {
      child.module->to(platform);
    }
    return *this;
  }

  Module& to(ScalarType dtype) {
    for (auto& param : params_) {
      if (param.series->defined()) {
        *param.series = param.series->to(dtype);
      }
    }
    for (auto& buffer : buffers_) {
      if (buffer.series->defined()) {
        *buffer.series = buffer.series->to(dtype);
      }
    }
    on_dtype_changed_(dtype);
    for (auto& child : children_) {
      child.module->to(dtype);
    }
    return *this;
  }

  Module& to(Platform platform, ScalarType dtype) {
    for (auto& param : params_) {
      if (param.series->defined()) {
        *param.series = param.series->to(platform, dtype);
      }
    }
    for (auto& buffer : buffers_) {
      if (buffer.series->defined()) {
        *buffer.series = buffer.series->to(platform, dtype);
      }
    }
    on_platform_dtype_changed_(platform, dtype);
    for (auto& child : children_) {
      child.module->to(platform, dtype);
    }
    return *this;
  }

  struct ParameterRef {
    std::string name;
    Series* series = nullptr;
  };

  struct ConstParameterRef {
    std::string name;
    const Series* series = nullptr;
  };

  struct BufferRef {
    std::string name;
    Series* series = nullptr;
  };

  struct ConstBufferRef {
    std::string name;
    const Series* series = nullptr;
  };

  struct ModuleRef {
    std::string name;
    Module* module = nullptr;
  };

  struct ConstModuleRef {
    std::string name;
    const Module* module = nullptr;
  };

  const std::vector<ParameterRef>& parameter_refs() noexcept { return params_; }
  const std::vector<ParameterRef>& parameter_refs() const noexcept { return params_; }
  const std::vector<BufferRef>& buffer_refs() noexcept { return buffers_; }
  const std::vector<BufferRef>& buffer_refs() const noexcept { return buffers_; }
  const std::vector<ModuleRef>& module_refs() noexcept { return children_; }
  const std::vector<ModuleRef>& module_refs() const noexcept { return children_; }

  std::vector<Series*> parameters() {
    std::vector<Series*> out;
    collect_parameters_(out);
    return out;
  }

  std::vector<const Series*> parameters() const {
    std::vector<const Series*> out;
    collect_parameters_(out);
    return out;
  }

  std::vector<ParameterRef> named_parameters() {
    std::vector<ParameterRef> out;
    collect_named_parameters_(out, "");
    return out;
  }

  std::vector<ConstParameterRef> named_parameters() const {
    std::vector<ConstParameterRef> out;
    collect_named_parameters_(out, "");
    return out;
  }

  std::vector<Series*> buffers() {
    std::vector<Series*> out;
    collect_buffers_(out);
    return out;
  }

  std::vector<const Series*> buffers() const {
    std::vector<const Series*> out;
    collect_buffers_(out);
    return out;
  }

  std::vector<BufferRef> named_buffers() {
    std::vector<BufferRef> out;
    collect_named_buffers_(out, "");
    return out;
  }

  std::vector<ConstBufferRef> named_buffers() const {
    std::vector<ConstBufferRef> out;
    collect_named_buffers_(out, "");
    return out;
  }

  std::vector<Module*> modules() {
    std::vector<Module*> out;
    collect_modules_(out);
    return out;
  }

  std::vector<const Module*> modules() const {
    std::vector<const Module*> out;
    collect_modules_(out);
    return out;
  }

  std::vector<ModuleRef> named_modules() {
    std::vector<ModuleRef> out;
    collect_named_modules_(out, "");
    return out;
  }

  std::vector<ConstModuleRef> named_modules() const {
    std::vector<ConstModuleRef> out;
    collect_named_modules_(out, "");
    return out;
  }

  template <class ChildModule>
  ChildModule& module_record(std::string name, ChildModule& child) {
    Module* child_ptr = module_ptr_(child);
    static_assert(!std::is_same_v<ChildModule, ChildModule> || true, "module_record expects an nn::Module or nn::ModuleHolder child");
    PLUSS_CHECK(child_ptr != nullptr, "pluss::nn::Module::module_record: attempted to register an empty child module");
    children_.push_back(ModuleRef{std::move(name), child_ptr});
    child_ptr->train(training_);
    return child;
  }

protected:
  Series& parameter_record(std::string name, Series& series) {
    params_.push_back(ParameterRef{std::move(name), &series});
    return series;
  }

  Series& buffer_record(std::string name, Series& series) {
    buffers_.push_back(BufferRef{std::move(name), &series});
    return series;
  }

  void clear_parameter_records_() noexcept { params_.clear(); }
  void clear_buffer_records_() noexcept { buffers_.clear(); }
  void clear_module_records_() noexcept { children_.clear(); }

  virtual void on_training_changed_(bool) noexcept {}
  virtual void on_platform_changed_(Platform) {}
  virtual void on_dtype_changed_(ScalarType) {}

  virtual void on_platform_dtype_changed_(Platform platform, ScalarType dtype) {
    on_platform_changed_(platform);
    on_dtype_changed_(dtype);
  }

private:
  template <class ChildModule>
  static Module* module_ptr_(ChildModule& child) {
    if constexpr (std::is_base_of_v<Module, std::remove_reference_t<ChildModule>>) {
      return &child;
    } else {
      static_assert(std::is_convertible_v<decltype(child.get()), Module*>,
                    "module_record expects an nn::Module or nn::ModuleHolder child");
      return child.get();
    }
  }

  void collect_parameters_(std::vector<Series*>& out) {
    for (auto& param : params_) {
      out.push_back(param.series);
    }
    for (auto& child : children_) {
      child.module->collect_parameters_(out);
    }
  }

  void collect_parameters_(std::vector<const Series*>& out) const {
    for (const auto& param : params_) {
      out.push_back(param.series);
    }
    for (const auto& child : children_) {
      child.module->collect_parameters_(out);
    }
  }

  void collect_named_parameters_(std::vector<ParameterRef>& out, const std::string& prefix) {
    for (auto& param : params_) {
      out.push_back(ParameterRef{prefix + param.name, param.series});
    }
    for (auto& child : children_) {
      child.module->collect_named_parameters_(out, prefix + child.name + ".");
    }
  }

  void collect_named_parameters_(std::vector<ConstParameterRef>& out, const std::string& prefix) const {
    for (const auto& param : params_) {
      out.push_back(ConstParameterRef{prefix + param.name, param.series});
    }
    for (const auto& child : children_) {
      child.module->collect_named_parameters_(out, prefix + child.name + ".");
    }
  }

  void collect_buffers_(std::vector<Series*>& out) {
    for (auto& buffer : buffers_) {
      out.push_back(buffer.series);
    }
    for (auto& child : children_) {
      child.module->collect_buffers_(out);
    }
  }

  void collect_buffers_(std::vector<const Series*>& out) const {
    for (const auto& buffer : buffers_) {
      out.push_back(buffer.series);
    }
    for (const auto& child : children_) {
      child.module->collect_buffers_(out);
    }
  }

  void collect_named_buffers_(std::vector<BufferRef>& out, const std::string& prefix) {
    for (auto& buffer : buffers_) {
      out.push_back(BufferRef{prefix + buffer.name, buffer.series});
    }
    for (auto& child : children_) {
      child.module->collect_named_buffers_(out, prefix + child.name + ".");
    }
  }

  void collect_named_buffers_(std::vector<ConstBufferRef>& out, const std::string& prefix) const {
    for (const auto& buffer : buffers_) {
      out.push_back(ConstBufferRef{prefix + buffer.name, buffer.series});
    }
    for (const auto& child : children_) {
      child.module->collect_named_buffers_(out, prefix + child.name + ".");
    }
  }

  void collect_modules_(std::vector<Module*>& out) {
    out.push_back(this);
    for (auto& child : children_) {
      child.module->collect_modules_(out);
    }
  }

  void collect_modules_(std::vector<const Module*>& out) const {
    out.push_back(this);
    for (const auto& child : children_) {
      child.module->collect_modules_(out);
    }
  }

  void collect_named_modules_(std::vector<ModuleRef>& out, const std::string& prefix) {
    out.push_back(ModuleRef{prefix, this});
    for (auto& child : children_) {
      const std::string child_name = prefix.empty() ? child.name : prefix + "." + child.name;
      child.module->collect_named_modules_(out, child_name);
    }
  }

  void collect_named_modules_(std::vector<ConstModuleRef>& out, const std::string& prefix) const {
    out.push_back(ConstModuleRef{prefix, this});
    for (const auto& child : children_) {
      const std::string child_name = prefix.empty() ? child.name : prefix + "." + child.name;
      child.module->collect_named_modules_(out, child_name);
    }
  }

  bool training_ = true;
  std::vector<ParameterRef> params_;
  std::vector<BufferRef> buffers_;
  std::vector<ModuleRef> children_;
};

template <class Impl>
class ModuleHolder {
public:
  using ImplType = Impl;

  ModuleHolder() noexcept(std::is_nothrow_default_constructible_v<Impl>) {
    if constexpr (std::is_default_constructible_v<Impl>) {
      impl_ = std::make_shared<Impl>();
    }
  }
  ModuleHolder(std::nullptr_t) noexcept {}
  explicit ModuleHolder(std::shared_ptr<Impl> impl) : impl_(std::move(impl)) {}

  template <class... Args>
  explicit ModuleHolder(std::in_place_t, Args&&... args)
      : impl_(std::make_shared<Impl>(std::forward<Args>(args)...)) {}

  template <class Arg0, class... Args,
            class = std::enable_if_t<
                !std::is_same_v<std::decay_t<Arg0>, ModuleHolder> &&
                !std::is_same_v<std::decay_t<Arg0>, std::shared_ptr<Impl>> &&
                !std::is_same_v<std::decay_t<Arg0>, std::nullptr_t> &&
                !std::is_same_v<std::decay_t<Arg0>, std::in_place_t>>>
  explicit ModuleHolder(Arg0&& arg0, Args&&... args)
      : impl_(std::make_shared<Impl>(std::forward<Arg0>(arg0), std::forward<Args>(args)...)) {}

  bool is_empty() const noexcept { return !impl_; }
  explicit operator bool() const noexcept { return static_cast<bool>(impl_); }
  bool is_training() const { return this->operator->()->is_training(); }

  Impl* get() noexcept { return impl_.get(); }
  const Impl* get() const noexcept { return impl_.get(); }

  Impl* operator->() noexcept {
    PLUSS_CHECK(static_cast<bool>(impl_), "pluss::nn::ModuleHolder: attempted to access an empty holder");
    return impl_.get();
  }

  const Impl* operator->() const noexcept {
    PLUSS_CHECK(static_cast<bool>(impl_), "pluss::nn::ModuleHolder: attempted to access an empty holder");
    return impl_.get();
  }

  Impl& operator*() noexcept {
    PLUSS_CHECK(static_cast<bool>(impl_), "pluss::nn::ModuleHolder: attempted to dereference an empty holder");
    return *impl_;
  }

  const Impl& operator*() const noexcept {
    PLUSS_CHECK(static_cast<bool>(impl_), "pluss::nn::ModuleHolder: attempted to dereference an empty holder");
    return *impl_;
  }

  template <class... Args>
  Impl& reset(Args&&... args) {
    impl_ = std::make_shared<Impl>(std::forward<Args>(args)...);
    return *impl_;
  }

  void reset(std::nullptr_t = nullptr) noexcept { impl_.reset(); }
  void train(bool enabled = true) { this->operator->()->train(enabled); }
  void eval() { this->operator->()->eval(); }
  Module& to(Platform platform) { return this->operator->()->to(platform); }
  Module& to(ScalarType dtype) { return this->operator->()->to(dtype); }
  Module& to(Platform platform, ScalarType dtype) { return this->operator->()->to(platform, dtype); }
  template <class... Args>
  decltype(auto) forward(Args&&... args) { return this->operator->()->forward(std::forward<Args>(args)...); }
  template <class... Args>
  decltype(auto) forward(Args&&... args) const { return this->operator->()->forward(std::forward<Args>(args)...); }
  template <class... Args>
  decltype(auto) operator()(Args&&... args) { return (*this->operator->())(std::forward<Args>(args)...); }
  template <class... Args>
  decltype(auto) operator()(Args&&... args) const { return (*this->operator->())(std::forward<Args>(args)...); }
  auto parameters() { return this->operator->()->parameters(); }
  auto parameters() const { return this->operator->()->parameters(); }
  auto named_parameters() { return this->operator->()->named_parameters(); }
  auto named_parameters() const { return this->operator->()->named_parameters(); }
  auto buffers() { return this->operator->()->buffers(); }
  auto buffers() const { return this->operator->()->buffers(); }
  auto named_buffers() { return this->operator->()->named_buffers(); }
  auto named_buffers() const { return this->operator->()->named_buffers(); }
  auto modules() { return this->operator->()->modules(); }
  auto modules() const { return this->operator->()->modules(); }
  auto named_modules() { return this->operator->()->named_modules(); }
  auto named_modules() const { return this->operator->()->named_modules(); }

  std::shared_ptr<Impl> ptr() const noexcept { return impl_; }

private:
  std::shared_ptr<Impl> impl_{};
};

} // namespace pluss::ai::nn

#define PLUSS_MODULE_IMPL(Name, Impl)                           \
  class Name : public ::pluss::ai::nn::ModuleHolder<Impl> {     \
  public:                                                       \
    using ::pluss::ai::nn::ModuleHolder<Impl>::ModuleHolder;    \
  }

#define PLUSS_MODULE(Name) PLUSS_MODULE_IMPL(Name, Name##Impl)
