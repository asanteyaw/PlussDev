#pragma once

#include <PlussQit/core/function.h>
#include <PlussQit/core/ivalue.h>
#include <MSharp/util/Exception.h>
#include <MSharp/util/intrusive_ptr.h>
#include <functional>
#include <utility>

namespace torch::jit {

struct BuiltinOpFunction : public Function {
  BuiltinOpFunction(
      MSharp::QualifiedName qualname,
      MSharp::FunctionSchema schema,
      std::function<void(Stack&)> callable,
      std::string doc_string = "")
      : name_(std::move(qualname)),
        callable_(std::move(callable)),
        schema_(std::move(schema)),
        doc_string_(std::move(doc_string)) {
    TORCH_INTERNAL_ASSERT(schema_.returns().size() == 1);
  }

  std::string_view doc_string() const override {
    return doc_string_;
  }

  void run(Stack& stack) override {
    callable_(stack);
  }

  MSharp::intrusive_ptr<MSharp::ivalue::Future> runAsync(
      Stack& stack,
      TaskLauncher /* not used */) override {
    run(stack);
    auto res = MSharp::make_intrusive<MSharp::ivalue::Future>(stack.front().type());
    res->markCompleted(std::move(stack.front()));
    return res;
  }

  const MSharp::QualifiedName& qualname() const override {
    return name_;
  }

  // if this isn't yet defined, run its method_creator function
  void ensure_defined() override {
    // nop
  }

  const MSharp::FunctionSchema& getSchema() const override {
    return schema_;
  }

  size_t num_inputs() const override {
    return schema_.arguments().size();
  }

  Function& setSchema(MSharp::FunctionSchema schema) override {
    schema_ = std::move(schema);
    return *this;
  }

  bool call(
      Stack& stack,
      std::optional<size_t>,
      MSharp::function_ref<void(const Code&)>) override {
    run(stack);
    return false;
  }

  bool call(Stack& stack, MSharp::function_ref<void(const mobile::Code&)>)
      override {
    run(stack);
    return false;
  }

  ~BuiltinOpFunction() override = default;

 private:
  MSharp::QualifiedName name_;

  std::function<void(Stack&)> callable_;

  MSharp::FunctionSchema schema_;

  std::string doc_string_;
};

} // namespace torch::jit
