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

  void clear() {
    for (Series* p : parameters()) {
      if (p && p->defined()) {
        p->clear_sensitivity();
      }
    }
  }

  Module& to(Platform platform) {
    for (auto& param : params_) {
      if (param.series->defined()) {
        const bool tracks = param.series->tracks_slope();
        *param.series = param.series->to(platform);
        param.series->tracks_slope_(tracks);
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
        const bool tracks = param.series->tracks_slope();
        *param.series = param.series->to(dtype);
        param.series->tracks_slope_(tracks);
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
        const bool tracks = param.series->tracks_slope();
        *param.series = param.series->to(platform, dtype);
        param.series->tracks_slope_(tracks);
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

  struct SnapshotEntry {
    std::string name;
    Series value{};
  };

  struct Snapshot {
    bool training{true};
    std::vector<SnapshotEntry> parameters{};
    std::vector<SnapshotEntry> buffers{};
  };

  struct RestoreReport {
    std::vector<std::string> missing{};
    std::vector<std::string> unexpected{};
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

  Snapshot snapshot() const {
    Snapshot out{};
    out.training = training_;
    for (const auto& named : named_parameters()) {
      SnapshotEntry entry{};
      entry.name = named.name;
      entry.value = (named.series != nullptr && named.series->defined()) ? named.series->clone() : Series();
      out.parameters.push_back(std::move(entry));
    }
    for (const auto& named : named_buffers()) {
      SnapshotEntry entry{};
      entry.name = named.name;
      entry.value = (named.series != nullptr && named.series->defined()) ? named.series->clone() : Series();
      out.buffers.push_back(std::move(entry));
    }
    return out;
  }

  RestoreReport restore(const Snapshot& snap, bool strict = true) {
    RestoreReport report{};
    auto params_named = named_parameters();
    auto buffers_named = named_buffers();

    auto find_entry = [](const std::vector<SnapshotEntry>& entries, const std::string& name) -> const SnapshotEntry* {
      for (const auto& entry : entries) {
        if (entry.name == name) return &entry;
      }
      return nullptr;
    };

    auto find_named_param = [&](const std::string& name) -> ParameterRef* {
      for (auto& named : params_named) {
        if (named.name == name) return &named;
      }
      return nullptr;
    };

    auto find_named_buffer = [&](const std::string& name) -> BufferRef* {
      for (auto& named : buffers_named) {
        if (named.name == name) return &named;
      }
      return nullptr;
    };

    auto assign_like = [](Series* target, const Series& source) {
      if (target == nullptr) return;
      if (!target->defined()) {
        *target = source.defined() ? source.clone() : Series();
      } else if (!source.defined()) {
        *target = Series();
      } else {
        *target = source.to(target->platform(), target->scalar_type());
      }
    };

    for (auto& named : params_named) {
      const SnapshotEntry* entry = find_entry(snap.parameters, named.name);
      if (entry == nullptr) {
        report.missing.push_back("param:" + named.name);
        continue;
      }
      const bool tracks = (named.series != nullptr && named.series->defined()) ? named.series->tracks_slope() : true;
      assign_like(named.series, entry->value);
      if (named.series != nullptr && named.series->defined()) {
        named.series->tracks_slope_(tracks);
      }
    }

    for (auto& named : buffers_named) {
      const SnapshotEntry* entry = find_entry(snap.buffers, named.name);
      if (entry == nullptr) {
        report.missing.push_back("buffer:" + named.name);
        continue;
      }
      assign_like(named.series, entry->value);
    }

    for (const auto& entry : snap.parameters) {
      if (find_named_param(entry.name) == nullptr) {
        report.unexpected.push_back("param:" + entry.name);
      }
    }
    for (const auto& entry : snap.buffers) {
      if (find_named_buffer(entry.name) == nullptr) {
        report.unexpected.push_back("buffer:" + entry.name);
      }
    }

    if (strict && (!report.missing.empty() || !report.unexpected.empty())) {
      std::string message = "pluss::nn::Module::restore: checkpoint mismatch";
      if (!report.missing.empty()) {
        message += " missing=[";
        for (std::size_t i = 0; i < report.missing.size(); ++i) {
          if (i) message += ", ";
          message += report.missing[i];
        }
        message += "]";
      }
      if (!report.unexpected.empty()) {
        message += " unexpected=[";
        for (std::size_t i = 0; i < report.unexpected.size(); ++i) {
          if (i) message += ", ";
          message += report.unexpected[i];
        }
        message += "]";
      }
      PLUSS_FAIL(message);
    }

    train(snap.training);
    return report;
  }

  template <class ChildModule>
  ChildModule& module_record(std::string name, ChildModule& child) {
    Module* child_ptr = module_ptr_(child);
    PLUSS_CHECK(child_ptr != nullptr, "pluss::nn::Module::module_record: attempted to register an empty child module");
    children_.push_back(ModuleRef{std::move(name), child_ptr});
    child_ptr->train(training_);
    for (Series* p : child_ptr->parameters()) {
      if (p && p->defined()) p->tracks_slope_(true);
    }
    return child;
  }

protected:
  Series& parameter_record(std::string name, Series& series) {
    if (series.defined()) series.tracks_slope_(true);
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
    static_assert(std::is_default_constructible_v<Impl>,
                  "Attempted to default-construct a module with no default constructor. "
                  "Use `{nullptr}` for empty state.");
    impl_ = std::make_shared<Impl>();
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
  void clear() { this->operator->()->clear(); }
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
  auto snapshot() const { return this->operator->()->snapshot(); }
  auto restore(const Module::Snapshot& snap, bool strict = true) { return this->operator->()->restore(snap, strict); }

  std::shared_ptr<Impl> ptr() const noexcept { return impl_; }

private:
  std::shared_ptr<Impl> impl_{};
};

} // namespace pluss::ai::nn

#define PLUSS_MODULE_IMPL(Name, ImplType)                               \
  class Name : public ::pluss::ai::nn::ModuleHolder<ImplType> {         \
  public:                                                                \
    using ::pluss::ai::nn::ModuleHolder<ImplType>::ModuleHolder;         \
    using Impl [[maybe_unused]] = ImplType;                              \
  }

#define PLUSS_MODULE(Name) PLUSS_MODULE_IMPL(Name, Name##Impl)
