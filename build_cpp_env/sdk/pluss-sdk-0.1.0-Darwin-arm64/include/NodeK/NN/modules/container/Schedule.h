#pragma once

#include "../Module.h"

#include <any>
#include <concepts>
#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

namespace pluss::ai::nn {

class PLUSS_API ScheduleImpl final : public Module {
public:
  struct StageInfo {
    std::string name;
    std::string input_type;
    std::string output_type;
    bool module_stage{false};
  };

  ScheduleImpl() : Module(true) {}

  template <class... Layers, std::enable_if_t<(sizeof...(Layers) > 0), int> = 0>
  explicit ScheduleImpl(Layers&&... layers) : Module(true) {
    reserve(sizeof...(Layers));
    (append(std::forward<Layers>(layers)), ...);
  }

  template <class Layer>
  ScheduleImpl& append(Layer&& layer) {
    return append(std::to_string(next_auto_name_++), std::forward<Layer>(layer));
  }

  template <class Layer>
  ScheduleImpl& append(std::string name, Layer&& layer) {
    return append_as<Series, Series>(std::move(name), std::forward<Layer>(layer));
  }

  template <class In, class Out, class Layer>
  ScheduleImpl& append_as(Layer&& layer) {
    return append_as<In, Out>(std::to_string(next_auto_name_++), std::forward<Layer>(layer));
  }

  template <class In, class Out, class Layer>
  ScheduleImpl& append_as(std::string name, Layer&& layer) {
    auto stage = make_stage_<In, Out>(std::forward<Layer>(layer));
    append_stage_(std::move(name), std::move(stage));
    return *this;
  }

  // Multiple same-typed, auto-named stages in one call -- the
  // Sequential(...)-style list form. A view over an existing container
  // (vector, array, span, ...); see the initializer_list overload below for
  // a literal `{...}` argument, which a class template like RangeView<Layer>
  // cannot deduce Layer from directly.
  template <class Layer>
  ScheduleImpl& append(pluss::RangeView<Layer> layers) {
    reserve(size() + layers.size());
    for (const auto& layer : layers) {
      append(layer);
    }
    return *this;
  }

  template <class Layer>
  ScheduleImpl& append(std::initializer_list<Layer> layers) {
    return append(pluss::RangeView<Layer>(layers));
  }

  template <class Layer>
  ScheduleImpl& insert(std::size_t index, Layer&& layer) {
    return insert(index, std::to_string(next_auto_name_++), std::forward<Layer>(layer));
  }

  template <class Layer>
  ScheduleImpl& insert(std::size_t index, std::string name, Layer&& layer) {
    return insert_as<Series, Series>(index, std::move(name), std::forward<Layer>(layer));
  }

  template <class In, class Out, class Layer>
  ScheduleImpl& insert_as(std::size_t index, Layer&& layer) {
    return insert_as<In, Out>(index, std::to_string(next_auto_name_++), std::forward<Layer>(layer));
  }

  template <class In, class Out, class Layer>
  ScheduleImpl& insert_as(std::size_t index, std::string name, Layer&& layer) {
    auto stage = make_stage_<In, Out>(std::forward<Layer>(layer));
    insert_stage_(index, std::move(name), std::move(stage));
    return *this;
  }

  ScheduleImpl& extend(const ScheduleImpl& other) {
    for (const auto& entry : other.entries_) {
      std::string name = entry.name;
      if (contains(name)) {
        name = std::to_string(next_auto_name_++);
      }
      append_stage_(std::move(name), entry.stage->clone());
    }
    return *this;
  }

  template <class Container>
  ScheduleImpl& extend(const Container& layers) {
    for (const auto& layer : layers) {
      append(layer);
    }
    return *this;
  }

  void erase(std::size_t index) {
    check_index_(index, "erase");
    entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(index));
    rebuild_module_records_();
  }

  Series forward(const Series& input) const {
    return run<Series>(input);
  }

  template <class ReturnType = Series, class... InputTypes>
  ReturnType run(InputTypes&&... inputs) const {
    static_assert(sizeof...(InputTypes) > 0, "pluss::nn::Schedule::run requires at least one input argument");
    PLUSS_CHECK(!entries_.empty(), "pluss::nn::Schedule::run: cannot run an empty schedule");

    std::any value = make_input_any_(std::forward<InputTypes>(inputs)...);
    const auto& first = *entries_.front().stage;
    PLUSS_CHECK(
        value.type() == first.input_type(),
        "pluss::nn::Schedule::run: first stage '" + entries_.front().name +
            "' expected input type " + type_name_(first.input_type()) +
            " but got " + type_name_(value.type()));

    for (const auto& entry : entries_) {
      value = entry.stage->invoke(std::move(value), entry.name);
    }

    PLUSS_CHECK(
        value.type() == typeid(ReturnType),
        "pluss::nn::Schedule::run: requested return type " + type_name_(typeid(ReturnType)) +
            " but chain produced " + type_name_(value.type()));
    return std::any_cast<ReturnType>(std::move(value));
  }

  Series operator()(const Series& input) const { return forward(input); }
  ScheduleImpl* operator->() noexcept { return this; }
  const ScheduleImpl* operator->() const noexcept { return this; }

  std::size_t size() const noexcept { return entries_.size(); }
  bool empty() const noexcept { return entries_.empty(); }
  void reserve(std::size_t count) { entries_.reserve(count); }

  bool contains(std::string_view name) const noexcept {
    return find_index_(name).has_value();
  }

  std::size_t index_of(std::string_view name) const {
    const auto index = find_index_(name);
    PLUSS_CHECK(index.has_value(), "pluss::nn::Schedule::index_of: unknown stage name '" + std::string(name) + "'");
    return *index;
  }

  const std::string& name_at(std::size_t index) const {
    check_index_(index, "name_at");
    return entries_[index].name;
  }

  const std::type_info& input_type_at(std::size_t index) const {
    check_index_(index, "input_type_at");
    return entries_[index].stage->input_type();
  }

  const std::type_info& output_type_at(std::size_t index) const {
    check_index_(index, "output_type_at");
    return entries_[index].stage->output_type();
  }

  Module* module_at(std::size_t index) {
    check_index_(index, "module_at");
    Module* module = entries_[index].stage->module_ptr();
    PLUSS_CHECK(module != nullptr, "pluss::nn::Schedule::module_at: stage '" + entries_[index].name + "' is not an nn::Module");
    return module;
  }

  const Module* module_at(std::size_t index) const {
    check_index_(index, "module_at");
    const Module* module = entries_[index].stage->module_ptr();
    PLUSS_CHECK(module != nullptr, "pluss::nn::Schedule::module_at: stage '" + entries_[index].name + "' is not an nn::Module");
    return module;
  }

  StageInfo stage_info(std::size_t index) const {
    check_index_(index, "stage_info");
    const auto& entry = entries_[index];
    return StageInfo{
        .name = entry.name,
        .input_type = type_name_(entry.stage->input_type()),
        .output_type = type_name_(entry.stage->output_type()),
        .module_stage = entry.stage->module_ptr() != nullptr,
    };
  }

  StageInfo at(std::size_t index) const {
    return stage_info(index);
  }

  std::vector<StageInfo> stage_manifest() const {
    std::vector<StageInfo> info;
    info.reserve(entries_.size());
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      info.push_back(stage_info(i));
    }
    return info;
  }

  void clear() {
    entries_.clear();
    clear_module_records_();
    next_auto_name_ = 0;
  }

private:
  struct StageBase;
  using StagePtr = std::unique_ptr<StageBase>;

  struct StageEntry {
    std::string name;
    StagePtr stage;
  };

  template <class>
  static constexpr bool always_false_v_ = false;

  template <class T>
  static constexpr bool holder_like_() {
    if constexpr (requires(T& value) { value.get(); }) {
      return std::is_convertible_v<decltype(std::declval<T&>().get()), Module*>;
    } else {
      return false;
    }
  }

  template <class T>
  static constexpr bool const_holder_like_() {
    if constexpr (requires(const T& value) { value.get(); }) {
      return std::is_convertible_v<decltype(std::declval<const T&>().get()), const Module*>;
    } else {
      return false;
    }
  }

  template <class T>
  static constexpr bool module_like_() {
    return std::is_base_of_v<Module, T> || holder_like_<T>() || const_holder_like_<T>();
  }

  template <class LayerType, class In, class Out, class ValueType>
  static Out call_layer_as_(const LayerType& layer, ValueType&& value) {
    if constexpr (requires(const LayerType& item, ValueType&& x) {
                    { item.forward(std::forward<ValueType>(x)) } -> std::convertible_to<Out>;
                  }) {
      return static_cast<Out>(layer.forward(std::forward<ValueType>(value)));
    } else if constexpr (requires(const LayerType& item, ValueType&& x) {
                           { item(std::forward<ValueType>(x)) } -> std::convertible_to<Out>;
                         }) {
      return static_cast<Out>(layer(std::forward<ValueType>(value)));
    } else {
      static_assert(
          always_false_v_<LayerType>,
          "pluss::nn::Schedule stage signature mismatch: layer must expose forward(In)->Out or operator()(In)->Out");
    }
  }

  template <class T>
  static Module* module_ptr_(T& layer) noexcept {
    if constexpr (std::is_base_of_v<Module, T>) {
      return &layer;
    } else if constexpr (holder_like_<T>()) {
      return layer.get();
    } else {
      return nullptr;
    }
  }

  template <class T>
  static const Module* module_ptr_(const T& layer) noexcept {
    if constexpr (std::is_base_of_v<Module, T>) {
      return &layer;
    } else if constexpr (const_holder_like_<T>()) {
      return layer.get();
    } else {
      return nullptr;
    }
  }

  struct StageBase {
    virtual ~StageBase() = default;
    virtual StagePtr clone() const = 0;
    virtual std::any invoke(std::any&& input, std::string_view stage_name) const = 0;
    virtual const std::type_info& input_type() const noexcept = 0;
    virtual const std::type_info& output_type() const noexcept = 0;
    virtual Module* module_ptr() noexcept = 0;
    virtual const Module* module_ptr() const noexcept = 0;
  };

  template <class LayerType, class In, class Out>
  struct StageModel final : StageBase {
    explicit StageModel(LayerType layer) : layer_(std::move(layer)) {}

    StagePtr clone() const override {
      static_assert(std::is_copy_constructible_v<LayerType>,
                    "pluss::nn::Schedule::extend requires copyable stage objects");
      return std::make_unique<StageModel>(*this);
    }

    std::any invoke(std::any&& input, std::string_view stage_name) const override {
      try {
        In typed = std::any_cast<In>(std::move(input));
        return std::any(call_layer_as_<LayerType, In, Out>(layer_, std::move(typed)));
      } catch (const std::bad_any_cast&) {
        PLUSS_CHECK(
            false,
            "pluss::nn::Schedule::run: stage '" + std::string(stage_name) +
                "' expected input type " + type_name_(typeid(In)) +
                " but got a different runtime type");
        return std::any{};
      }
    }

    const std::type_info& input_type() const noexcept override { return typeid(In); }
    const std::type_info& output_type() const noexcept override { return typeid(Out); }

    Module* module_ptr() noexcept override {
      return module_ptr_(layer_);
    }

    const Module* module_ptr() const noexcept override {
      return module_ptr_(layer_);
    }

    LayerType layer_;
  };

  template <class LayerType, class In, class Out>
  static constexpr bool layer_invocable_as_v_ =
      requires(const LayerType& layer, In value) {
        { call_layer_as_<LayerType, In, Out>(layer, std::move(value)) } -> std::same_as<Out>;
      };

  template <class In, class Out, class Layer>
  static StagePtr make_stage_(Layer&& layer) {
    using InType = std::decay_t<In>;
    using OutType = std::decay_t<Out>;
    using LayerType = std::decay_t<Layer>;

    static_assert(!std::is_void_v<OutType>, "pluss::nn::Schedule stages cannot return void");
    static_assert(
        layer_invocable_as_v_<LayerType, InType, OutType>,
        "pluss::nn::Schedule stage signature mismatch: layer must expose forward(In)->Out or operator()(In)->Out");

    auto stage = std::make_unique<StageModel<LayerType, InType, OutType>>(std::forward<Layer>(layer));
    if constexpr (module_like_<LayerType>()) {
      PLUSS_CHECK(stage->module_ptr() != nullptr, "pluss::nn::Schedule: attempted to add an empty module holder");
    }
    return stage;
  }

  static std::string type_name_(const std::type_info& info) {
    return info.name();
  }

  std::size_t validate_index_for_insert_(std::size_t index) const {
    PLUSS_CHECK(index <= entries_.size(), "pluss::nn::Schedule::insert index out of range");
    return index;
  }

  void check_index_(std::size_t index, const char* op_name) const {
    PLUSS_CHECK(index < entries_.size(), std::string("pluss::nn::Schedule::") + op_name + " index out of range");
  }

  std::optional<std::size_t> find_index_(std::string_view name) const noexcept {
    for (std::size_t i = 0; i < entries_.size(); ++i) {
      if (entries_[i].name == name) return i;
    }
    return std::nullopt;
  }

  void validate_unique_name_(std::string_view name) const {
    PLUSS_CHECK(!name.empty(), "pluss::nn::Schedule stage name cannot be empty");
    PLUSS_CHECK(!contains(name), "pluss::nn::Schedule duplicate stage name '" + std::string(name) + "'");
  }

  void validate_chain_at_(std::size_t index, const StageBase& stage) const {
    if (index > 0) {
      const auto& prev = *entries_[index - 1].stage;
      PLUSS_CHECK(
          prev.output_type() == stage.input_type(),
          "pluss::nn::Schedule type mismatch: previous stage output " + type_name_(prev.output_type()) +
              " is incompatible with new stage input " + type_name_(stage.input_type()));
    }
    if (index < entries_.size()) {
      const auto& next = *entries_[index].stage;
      PLUSS_CHECK(
          stage.output_type() == next.input_type(),
          "pluss::nn::Schedule type mismatch: new stage output " + type_name_(stage.output_type()) +
              " is incompatible with next stage input " + type_name_(next.input_type()));
    }
  }

  void rebuild_module_records_() {
    clear_module_records_();
    for (auto& entry : entries_) {
      if (Module* child = entry.stage->module_ptr()) {
        module_record(entry.name, *child);
      }
    }
  }

  void append_stage_(std::string name, StagePtr stage) {
    validate_unique_name_(name);
    validate_chain_at_(entries_.size(), *stage);
    entries_.push_back(StageEntry{std::move(name), std::move(stage)});
    rebuild_module_records_();
  }

  void insert_stage_(std::size_t index, std::string name, StagePtr stage) {
    validate_index_for_insert_(index);
    validate_unique_name_(name);
    validate_chain_at_(index, *stage);
    entries_.insert(entries_.begin() + static_cast<std::ptrdiff_t>(index), StageEntry{std::move(name), std::move(stage)});
    rebuild_module_records_();
  }

  template <class... InputTypes>
  static std::any make_input_any_(InputTypes&&... inputs) {
    if constexpr (sizeof...(InputTypes) == 1) {
      using InputType = std::decay_t<std::tuple_element_t<0, std::tuple<InputTypes...>>>;
      return std::any(InputType(std::forward<InputTypes>(inputs)...));
    } else {
      using InputTuple = std::tuple<std::decay_t<InputTypes>...>;
      return std::any(InputTuple(std::forward<InputTypes>(inputs)...));
    }
  }

  std::vector<StageEntry> entries_{};
  std::size_t next_auto_name_{0};
};

class Schedule : public ModuleHolder<ScheduleImpl> {
public:
  using ModuleHolder<ScheduleImpl>::ModuleHolder;
  using Impl [[maybe_unused]] = ScheduleImpl;

  template <class Layer>
  Schedule& append(Layer&& layer) {
    this->operator->()->append(std::forward<Layer>(layer));
    return *this;
  }

  template <class Layer>
  Schedule& append(std::string name, Layer&& layer) {
    this->operator->()->append(std::move(name), std::forward<Layer>(layer));
    return *this;
  }

  template <class In, class Out, class Layer>
  Schedule& append_as(Layer&& layer) {
    this->operator->()->template append_as<In, Out>(std::forward<Layer>(layer));
    return *this;
  }

  template <class In, class Out, class Layer>
  Schedule& append_as(std::string name, Layer&& layer) {
    this->operator->()->template append_as<In, Out>(std::move(name), std::forward<Layer>(layer));
    return *this;
  }

  template <class Layer>
  Schedule& append(pluss::RangeView<Layer> layers) {
    this->operator->()->template append<Layer>(layers);
    return *this;
  }

  template <class Layer>
  Schedule& append(std::initializer_list<Layer> layers) {
    this->operator->()->template append<Layer>(layers);
    return *this;
  }

  template <class Layer>
  Schedule& insert(std::size_t index, Layer&& layer) {
    this->operator->()->insert(index, std::forward<Layer>(layer));
    return *this;
  }

  template <class Layer>
  Schedule& insert(std::size_t index, std::string name, Layer&& layer) {
    this->operator->()->insert(index, std::move(name), std::forward<Layer>(layer));
    return *this;
  }

  template <class In, class Out, class Layer>
  Schedule& insert_as(std::size_t index, Layer&& layer) {
    this->operator->()->template insert_as<In, Out>(index, std::forward<Layer>(layer));
    return *this;
  }

  template <class In, class Out, class Layer>
  Schedule& insert_as(std::size_t index, std::string name, Layer&& layer) {
    this->operator->()->template insert_as<In, Out>(index, std::move(name), std::forward<Layer>(layer));
    return *this;
  }

  template <class ReturnType = Series, class... InputTypes>
  ReturnType run(InputTypes&&... inputs) const {
    return this->operator->()->template run<ReturnType>(std::forward<InputTypes>(inputs)...);
  }

  Schedule& extend(const Schedule& other) {
    this->operator->()->extend(*other);
    return *this;
  }
};

} // namespace pluss::ai::nn
