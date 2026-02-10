/**
 * This file contains some general registration test cases.
 * More detailed test cases containing different APIs for registering kernels
 * are found in other files in this directory.
 */

#include <gtest/gtest.h>

// This file intentionally tests some deprecated APIs
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <PlussQit/core/boxing/impl/test_helpers.h>
#include <PlussQit/core/op_registration/op_registration.h>
#include <torch/library.h>
#include <PlussQit/core/Series.h>
#include <functional>

#include <PlussQit/core/LegacyTypeDispatch.h>

#include <algorithm>

using MSharp::RegisterOperators;
using MSharp::OperatorKernel;
using MSharp::OperatorHandle;
using MSharp::Dispatcher;
using MSharp::IValue;
using MSharp::DispatchKey;

using torch::Library;
using torch::CppFunction;

using pluss::Series;

namespace {

struct DummyKernel final : OperatorKernel {
  void operator()(Series) {}
};

struct MockKernel final : OperatorKernel {
  MockKernel(bool* called): called_(called) {}

  void operator()(Series) {
    *called_ = true;
  }
private:
  bool* called_;
};

TEST(OperatorRegistrationTest, whenRegisteringWithSchemaBeforeKernelInOptionsObject_thenCanBeCalled) {
  bool called = false;
  auto registrar = MSharp::RegisterOperators().op(MSharp::RegisterOperators::options().schema("_test::dummy(Series dummy) -> ()").catchAllKernel<MockKernel>(&called));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, whenRegisteringWithSchemaAfterKernelInOptionsObject_thenCanBeCalled) {
  bool called = false;
  auto registrar = MSharp::RegisterOperators().op(MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called).schema("_test::dummy(Series dummy) -> ()"));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, whenRegisteringWithNameBeforeKernelInOptionsObject_thenCanBeCalled) {
  bool called = false;
  auto registrar = MSharp::RegisterOperators().op(MSharp::RegisterOperators::options().schema("_test::dummy").catchAllKernel<MockKernel>(&called));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, whenRegisteringWithNameAfterKernelInOptionsObject_thenCanBeCalled) {
  bool called = false;
  auto registrar = MSharp::RegisterOperators().op(MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called).schema("_test::dummy"));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, whenRegisteringWithoutSchema_thenFails) {
  expectThrows<MSharp::Error>([] {
    MSharp::RegisterOperators().op(MSharp::RegisterOperators::options().catchAllKernel<DummyKernel>());
  }, "In operator registration: Tried to register an operator without specifying a schema or operator name.");
}

TEST(OperatorRegistrationTest, whenCallingOpWithWrongDispatchKey_thenFails) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().kernel<DummyKernel>(MSharp::DispatchKey::CPU));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  }, "Could not run '_test::dummy' with arguments from the 'CUDA'"
  " backend.");
}

TEST(OperatorRegistrationTest, givenOpWithCatchallKernel_whenCallingOp_thenCallsCatchallKernel) {
  bool called = false;
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

// TODO Rewrite (since this is now allowed) and reenable
// TEST(OperatorRegistrationTest, givenOpWithCatchallKernel_whenRegisteringDispatchedKernel_thenFails) {
//   bool called = false;
//   auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called));
//   expectThrows<MSharp::Error>([&] {
//     MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().kernel<MockKernel>(MSharp::DispatchKey::CPU, &called));
//   }, "for an operator which already has a catch-all kernel registered");
// }

// TEST(OperatorRegistrationTest, givenOpWithCatchallKernel_whenRegisteringDispatchedKernelInSameOpCall_thenFails) {
//   bool called = false;
//   expectThrows<MSharp::Error>([&] {
//     auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
//       .catchAllKernel<MockKernel>(&called)
//       .kernel<MockKernel>(MSharp::DispatchKey::CPU, &called));
//   }, "for an operator which already has a catch-all kernel registered");
// }

TEST(OperatorRegistrationTest, givenOpWithDispatchedKernelOutOfScope_whenRegisteringCatchallKernelAndCallingOp_thenCallsCatchallKernel) {
  bool called = false;
  {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().kernel<MockKernel>(MSharp::DispatchKey::CPU, &called));
  }

  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called);
}

// TODO Rewrite (since this is now allowed) and reenable
// TEST(OperatorRegistrationTest, givenOpWithDispatchedKernel_whenRegisteringCatchallKernel_thenFails) {
//   bool called = false;
//   auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().kernel<MockKernel>(MSharp::DispatchKey::CPU, &called));
//   expectThrows<MSharp::Error>([&] {
//     MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called));
//   }, "Tried to register a catch-all kernel for an operator which already has kernels for dispatch keys CPU. An operator can only have either a catch-all kernel or kernels with dispatch keys. The operator schema is _test::dummy");
// }
//
// TEST(OperatorRegistrationTest, givenOpWithDispatchedKernel_whenRegisteringCatchallKernelInSameOpCall_thenFails) {
//   bool called = false;
//   expectThrows<MSharp::Error>([&] {
//     auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
//       .kernel<MockKernel>(MSharp::DispatchKey::CPU, &called)
//       .catchAllKernel<MockKernel>(&called));
//   }, "Tried to register a catch-all kernel for an operator which already has kernels for dispatch keys CPU. An operator can only have either a catch-all kernel or kernels with dispatch keys. The operator schema is _test::dummy");
// }

TEST(OperatorRegistrationTest, givenOpWithCatchallKernelOutOfScope_whenRegisteringDispatchedKernelAndCallingOp_thenCallsCatchallKernel) {
  bool called = false;
  {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().catchAllKernel<MockKernel>(&called));
  }

  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options().kernel<MockKernel>(MSharp::DispatchKey::CPU, &called));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  EXPECT_FALSE(called);
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, givenOpWithoutKernels_whenRegisteringWithSchema_thenOnlyRegistersSchema) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()");

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered
  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  }, "Could not run '_test::dummy' with arguments from the 'CPU'"
  " backend.");
}

TEST(OperatorRegistrationTest, givenOpWithoutKernels_whenRegisteringWithoutSchema_thenFails) {
  expectThrows<MSharp::Error>([&] {
    MSharp::RegisterOperators().op("_test::dummy");
  }, "Cannot infer operator schema in registration of operator _test::dummy because there is no kernel specified.");
}

TEST(OperatorRegistrationTest, givenOpWithoutKernels_whenRunningOutOfScope_thenSchemaIsGone) {
  {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()");
  }

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  EXPECT_FALSE(op.has_value());
}

TEST(OperatorRegistrationTest, givenOpWithoutKernelsWithoutTensorInputs_whenRegistering_thenRegisters) {
  // as long as we don't register non-catchall kernels, ops without series arguments are fine
  auto registrar = MSharp::RegisterOperators().op("_test::dummy() -> ()");

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered
}

TEST(OperatorRegistrationTest, givenMultipleKernelsWithSameDispatchKey_whenRegisteringInSameOpCall_thenFails) {
  expectThrows<MSharp::Error>([&] {
    auto registrar = MSharp::RegisterOperators()
        .op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
            .kernel<DummyKernel>(MSharp::DispatchKey::CPU)
            .kernel<DummyKernel>(MSharp::DispatchKey::CPU));
  }, "In operator registration: Tried to register multiple kernels with same dispatch key CPU for operator schema _test::dummy");
}

TEST(OperatorRegistrationTest, givenMultipleCatchallKernels_whenRegisteringInSameOpCall_thenFails) {
  expectThrows<MSharp::Error>([&] {
    auto registrar = MSharp::RegisterOperators()
        .op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
            .catchAllKernel<DummyKernel>()
            .catchAllKernel<DummyKernel>());
  }, "Tried to register multiple catch-all kernels for operator schema _test::dummy");
}

TEST(OperatorRegistrationTest, whenRegisteringCPUTensorType_thenCanOnlyCallUnboxedWithCPUDispatchKey) {
  bool called_kernel_cpu = false;
  auto registrar= MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<MockKernel>(MSharp::DispatchKey::CPU, &called_kernel_cpu));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered

  // Ensure that dispatcher doesn't take the dispatch key from the series but from the direct argument instead.
  called_kernel_cpu = false;
  callOpUnboxedWithPrecomputedDispatchKeySet<void, Series>(*op, MSharp::DispatchKeySet(MSharp::DispatchKey::CPU), dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called_kernel_cpu);

  // Ensure that disptach key from series is not used here.
  called_kernel_cpu = false;
  expectThrows<MSharp::Error>([&] {
    callOpUnboxedWithPrecomputedDispatchKeySet<void, Series>(*op, MSharp::DispatchKeySet(MSharp::DispatchKey::CUDA), dummyTensor(MSharp::DispatchKey::CPU));
  }, "Could not run '_test::dummy' with arguments from the 'CUDA'"
  " backend.");
}

std::string expectedMessageForBackend(DispatchKey key) {
  std::string key_str(MSharp::toString(key));
  return "Could not run '_test::dummy' with arguments from the '" + key_str + "' backend";
}

TEST(OperatorRegistrationTest, whenRegisteringMultipleKernelsInSameOpCallAndCalling_thenCallsCorrectKernel) {
  bool called_kernel1 = false;
  bool called_kernel2 = false;
  auto registrar0 = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<MockKernel>(MSharp::DispatchKey::CPU, &called_kernel1)
    .kernel<MockKernel>(MSharp::DispatchKey::CUDA, &called_kernel2));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered

  called_kernel1 = called_kernel2 = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  EXPECT_TRUE(called_kernel1);
  EXPECT_FALSE(called_kernel2);

  called_kernel1 = called_kernel2 = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_FALSE(called_kernel1);
  EXPECT_TRUE(called_kernel2);

  // Test for out of tree lazy backends- ::Lazy key is now registered to TS backend in tree
  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA}) {
    std::string expectMessage = expectedMessageForBackend(key);
    expectThrows<MSharp::Error>([&] {
      callOp(*op, dummyTensor(key));
    }, expectMessage.c_str());

    // also assert that the error message contains the available series type ids, but don't assert their order
    expectThrows<MSharp::Error>([&] {
      callOp(*op, dummyTensor(key));
    }, "CPU");
    expectThrows<MSharp::Error>([&] {
      callOp(*op, dummyTensor(key));
    }, "CUDA");
  }
}

bool called_stackbased_kernel = false;
void stackBasedKernel(const OperatorHandle&, MSharp::Stack* stack) {
  called_stackbased_kernel = true;
}

TEST(OperatorRegistrationTest, whenRegisteringMultipleKernelsByNameAndNoneCanInferSchema_thenFails) {
  expectThrows<MSharp::Error>([&] {
    auto registrar1 = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
      .kernel<&stackBasedKernel>(MSharp::DispatchKey::CPU)
      .kernel<&stackBasedKernel>(MSharp::DispatchKey::CUDA)
      .kernel<&stackBasedKernel>(MSharp::DispatchKey::XLA)
      .kernel<&stackBasedKernel>(MSharp::DispatchKey::Lazy));
  }, "Cannot infer operator schema for this kind of kernel in registration of operator _test::dummy");
}

TEST(OperatorRegistrationTest, whenRegisteringMultipleKernelsBySchemaAndNoneCanInferSchema_thenSucceeds) {
  bool called_kernel = false;
  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::CPU)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::CUDA)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::XLA)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::Lazy));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  EXPECT_TRUE(called_stackbased_kernel);
  EXPECT_FALSE(called_kernel);

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_TRUE(called_stackbased_kernel);
  EXPECT_FALSE(called_kernel);

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    called_kernel = called_stackbased_kernel = false;
    callOp(*op, dummyTensor(key));
    EXPECT_TRUE(called_stackbased_kernel);
    EXPECT_FALSE(called_kernel);
  }
}

TEST(OperatorRegistrationTest, whenRegisteringMultipleKernelsByNameAndOnlyOneCanInferSchema_thenSucceeds) {
  bool called_kernel = false;
  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::CPU)
    .kernel<MockKernel>(MSharp::DispatchKey::CUDA, &called_kernel)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::XLA)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::Lazy));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  EXPECT_TRUE(called_stackbased_kernel);
  EXPECT_FALSE(called_kernel);

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_FALSE(called_stackbased_kernel);
  EXPECT_TRUE(called_kernel);

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    called_kernel = called_stackbased_kernel = false;
    callOp(*op, dummyTensor(key));
    EXPECT_TRUE(called_stackbased_kernel);
    EXPECT_FALSE(called_kernel);
  }
}

TEST(OperatorRegistrationTest, whenRegisteringMultipleKernelsBySchemaAndOnlyOneCanInferSchema_thenSucceeds) {
  bool called_kernel = false;
  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::CPU)
    .kernel<MockKernel>(MSharp::DispatchKey::CUDA, &called_kernel)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::XLA)
    .kernel<&stackBasedKernel>(MSharp::DispatchKey::Lazy));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value()); // assert schema is registered

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  EXPECT_TRUE(called_stackbased_kernel);
  EXPECT_FALSE(called_kernel);

  called_kernel = called_stackbased_kernel = false;
  callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  EXPECT_FALSE(called_stackbased_kernel);
  EXPECT_TRUE(called_kernel);

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    called_kernel = called_stackbased_kernel = false;
    callOp(*op, dummyTensor(key));
    EXPECT_TRUE(called_stackbased_kernel);
    EXPECT_FALSE(called_kernel);
  }
}

struct DummyKernelWithIntParam final : OperatorKernel {
  void operator()(Series, int64_t) {}
};

TEST(OperatorRegistrationTest, whenRegisteringMismatchingKernelsInSameOpCall_thenFails) {
  bool called_kernel = false;
  expectThrows<MSharp::Error>([&] {
    auto registrar1 = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
      .kernel<DummyKernelWithIntParam>(MSharp::DispatchKey::CPU)
      .kernel<MockKernel>(MSharp::DispatchKey::CUDA, &called_kernel));
  }, "Mismatch in kernel C++ signatures");
}

void backend_fallback_kernel(const MSharp::OperatorHandle& op, MSharp::Stack* stack) {
  (*stack)[1] = (*stack)[1].toStringRef() + op.schema().name();
}

TEST(OperatorRegistrationTest, whenRegisteringBackendFallbackKernel_thenCanBeCalled) {
  auto registrar = MSharp::Dispatcher::singleton().registerFallback(MSharp::DispatchKey::CPU, MSharp::KernelFunction::makeFromBoxedFunction<&backend_fallback_kernel>(), "");

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()");
  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  EXPECT_EQ("hello _test::dummy", stack[1].toStringRef());
}

TEST(OperatorRegistrationTest, whenRegisteringBackendFallbackKernelForWrongBackend_thenCannotBeCalled) {
  auto registrar = MSharp::Dispatcher::singleton().registerFallback(MSharp::DispatchKey::CUDA, MSharp::KernelFunction::makeFromBoxedFunction<&backend_fallback_kernel>(), "");

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()");
  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  expectThrows<MSharp::Error>([&] {
    auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  }, "Could not run '_test::dummy' with arguments from the 'CPU' backend.");
}

bool called = false;

TEST(OperatorRegistrationTest, whenRegisteringBackendFallbackKernelAndRegularKernelForDifferentBackend_thenRegularKernelCanBeCalled) {
  auto registrar = MSharp::Dispatcher::singleton().registerFallback(MSharp::DispatchKey::CPU, MSharp::KernelFunction::makeFromBoxedFunction<&backend_fallback_kernel>(), "");

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()", MSharp::RegisterOperators::options()
      .kernel(MSharp::DispatchKey::CUDA, [] (Series, std::string) {
        called = true;
      }));
  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA), "hello ");
  EXPECT_TRUE(called);
}

TEST(OperatorRegistrationTest, whenRegisteringBackendFallbackKernelAndRegularKernelForDifferentBackend_thenFallbackKernelCanBeCalled) {
  auto registrar = MSharp::Dispatcher::singleton().registerFallback(MSharp::DispatchKey::CPU, MSharp::KernelFunction::makeFromBoxedFunction<&backend_fallback_kernel>(), "");

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()", MSharp::RegisterOperators::options()
      .kernel(MSharp::DispatchKey::CUDA, [] (Series, std::string) {
        called = true;
      }));
  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  EXPECT_FALSE(called);
  EXPECT_EQ("hello _test::dummy", stack[1].toStringRef());
}

TEST(OperatorRegistrationTest, whenRegisteringBackendFallbackKernelAndRegularKernelForSameBackend_thenCallsRegularKernel) {
  auto registrar = MSharp::Dispatcher::singleton().registerFallback(MSharp::DispatchKey::CPU, MSharp::KernelFunction::makeFromBoxedFunction<&backend_fallback_kernel>(), "");

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()", MSharp::RegisterOperators::options()
      .kernel(MSharp::DispatchKey::CPU, [] (Series, std::string) {
        called = true;
      }));
  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  EXPECT_TRUE(called);
}

bool called_autograd = false;
bool called_nonautograd = false;

void nonautograd_kernel(Series a) {
  called_nonautograd = true;
}

void autograd_kernel(Series a) {
  called_autograd = true;
}

TEST(OperatorRegistrationTest, whenRegisteringAutogradKernel_thenCanCallAutogradKernel) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<decltype(autograd_kernel), &autograd_kernel>(DispatchKey::Autograd));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  called_autograd = false;
  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  }, "Could not run '_test::dummy' with arguments from the 'CPU'"
  " backend.");

  op->typed<void(Series)>().call(dummyTensor(DispatchKey::CPU, /*requires_grad=*/true));
  EXPECT_TRUE(called_autograd);
}

TEST(OperatorRegistrationTest, whenRegisteringAutogradKernelWithRegularKernel_thenCanCallAutogradKernel) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<decltype(nonautograd_kernel), nonautograd_kernel>(DispatchKey::CPU)
    .kernel<decltype(autograd_kernel), &autograd_kernel>(DispatchKey::Autograd));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  called_nonautograd = called_autograd = false;
  op->typed<void (Series)>().call(dummyTensor(DispatchKey::CPU, /*requires_grad=*/true));
  EXPECT_FALSE(called_nonautograd);
  EXPECT_TRUE(called_autograd);
}

TEST(
    OperatorRegistrationTest,
    whenRegisteringAutogradKernelWithCatchAllKernel_thenCanCallCatchallKernel) {
  auto registrar = MSharp::RegisterOperators().op(
      "_test::dummy(Series dummy) -> ()",
      MSharp::RegisterOperators::options()
          .catchAllKernel<decltype(nonautograd_kernel), nonautograd_kernel>()
          .kernel<decltype(autograd_kernel), &autograd_kernel>(
              DispatchKey::Autograd));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  // catchAll now maps to CompositeImplicitAutograd which has
  // higher precedence than Autograd
  called_nonautograd = called_autograd = false;
  op->typed<void(Series)>().call(
      dummyTensor(DispatchKey::CPU, /*requires_grad=*/true));
  EXPECT_TRUE(called_nonautograd);
  EXPECT_FALSE(called_autograd);

  called_nonautograd = called_autograd = false;
  op->typed<void(Series)>().call(dummyTensor(DispatchKey::CPU));
  EXPECT_TRUE(called_nonautograd);
  EXPECT_FALSE(called_autograd);
}

TEST(OperatorRegistrationTest, AutogradBackendOverridesAutogradKernel) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<decltype(nonautograd_kernel), &nonautograd_kernel>(DispatchKey::AutogradCPU)
    .kernel<decltype(autograd_kernel), &autograd_kernel>(DispatchKey::Autograd));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  }, "Could not run '_test::dummy' with arguments from the 'CPU'"
  " backend.");

  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
  }, "Could not run '_test::dummy' with arguments from the 'CUDA'"
  " backend.");

  called_nonautograd = called_autograd = false;
  op->typed<void (Series)>().call(dummyTensor(DispatchKey::CPU, /*requires_grad=*/true));
  EXPECT_TRUE(called_nonautograd);
  EXPECT_FALSE(called_autograd);

  called_nonautograd = called_autograd = false;
  op->typed<void (Series)>().call(dummyTensor(DispatchKey::CUDA, /*requires_grad=*/true));
  EXPECT_TRUE(called_autograd);
  EXPECT_FALSE(called_nonautograd);
}

void LazyBackendsAutogradOverridesAutogradKernel(DispatchKey key) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
    .kernel<decltype(nonautograd_kernel), &nonautograd_kernel>(MSharp::getAutogradKeyFromBackend(toBackendComponent(key)))
    .kernel<decltype(autograd_kernel), &autograd_kernel>(DispatchKey::Autograd));

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());

  std::string expectedMessage = expectedMessageForBackend(key);
  expectThrows<MSharp::Error>([&] {
    callOp(*op, dummyTensor(key));
  }, expectedMessage.c_str());

  called_nonautograd = called_autograd = false;
  op->typed<void (Series)>().call(dummyTensor(key, /*requires_grad=*/true));
  EXPECT_TRUE(called_nonautograd);
  EXPECT_FALSE(called_autograd);

  called_nonautograd = called_autograd = false;
  op->typed<void (Series)>().call(dummyTensor(DispatchKey::CPU, /*requires_grad=*/true));
  EXPECT_TRUE(called_autograd);
  EXPECT_FALSE(called_nonautograd);
}

// no longer test ::Lazy key here
// since it is now registered to TS backend in-tree and thus behaves differently,
// does not throw the expected 'could not run..' messages
TEST(OperatorRegistrationTest, AutogradXLAOverridesAutogradKernel) {
  LazyBackendsAutogradOverridesAutogradKernel(DispatchKey::XLA);
}

void whenRegisterWithLazyBackendsAndCatchAll_AutogradLazyBackendsIsNotFilled(DispatchKey key) {
  {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
      .catchAllKernel<decltype(nonautograd_kernel), nonautograd_kernel>());

    auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
    ASSERT_TRUE(op.has_value());

    called_nonautograd = called_autograd = false;
    op->typed<void (Series)>().call(dummyTensor(key, /*requires_grad=*/true));
    EXPECT_TRUE(called_nonautograd);
    EXPECT_FALSE(called_autograd);

    called_nonautograd = called_autograd = false;
    op->typed<void (Series)>().call(dummyTensor(key));
    EXPECT_FALSE(called_autograd);
    EXPECT_TRUE(called_nonautograd);
  }
  {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy(Series dummy) -> ()", MSharp::RegisterOperators::options()
      .kernel<decltype(autograd_kernel), &autograd_kernel>(key)
      .catchAllKernel<decltype(nonautograd_kernel), nonautograd_kernel>());

    auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
    ASSERT_TRUE(op.has_value());

    // When there's direct registration to XLA / Lazy backend, Autograd{XLA, Lazy} doesn't pick up catchAll
    // kernel in precompute but just keep fallthrough kernel from backend fallback.
    // Thus it falls through Autograd{XLA, Lazy} and reaches the kernel at XLA / Lazy key.
    called_nonautograd = called_autograd = false;
    op->typed<void (Series)>().call(dummyTensor(key, /*requires_grad=*/true));
    EXPECT_FALSE(called_nonautograd);
    EXPECT_TRUE(called_autograd);

    called_nonautograd = called_autograd = false;
    op->typed<void (Series)>().call(dummyTensor(key));
    EXPECT_TRUE(called_autograd);
    EXPECT_FALSE(called_nonautograd);
  }
}

TEST(OperatorRegistrationTest, whenRegisterWithXLAKernelAndCatchAll_AutogradXLAIsNotFilled) {
  whenRegisterWithLazyBackendsAndCatchAll_AutogradLazyBackendsIsNotFilled(DispatchKey::XLA);
}

TEST(OperatorRegistrationTest, whenRegisterWithLazyKernelAndCatchAll_AutogradLazyIsNotFilled) {
  whenRegisterWithLazyBackendsAndCatchAll_AutogradLazyBackendsIsNotFilled(DispatchKey::Lazy);
}

TEST(OperatorRegistrationTest, whenregisteringwithinvalidoverloadname) {
  expectThrows<MSharp::Error>([] {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy.default", MSharp::RegisterOperators::options()
      .kernel(DispatchKey::CPU, [] (const int64_t&) {}));
  }, "default is not a legal overload name for aten operators");
  expectThrows<MSharp::Error>([] {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy.__name__", MSharp::RegisterOperators::options()
      .kernel(DispatchKey::CPU, [] (const int64_t&) {}));
  }, "__name__ is not a legal overload name for aten operators");
}

TEST(OperatorRegistrationTest, givenLambdaKernel_whenRegisteringWithMismatchingCppSignatures_thenFails) {
  expectThrows<MSharp::Error>([] {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
      .kernel(DispatchKey::CPU, [] (const int64_t&) {})
      .kernel(DispatchKey::CUDA, [] (int64_t) {}));
  }, "Mismatch in kernel C++ signatures");
}

TEST(OperatorRegistrationTest, givenLambdaKernel_whenRegisteringCatchAllAndBackendWithMismatchingCppSignatures_thenFails) {
  expectThrows<MSharp::Error>([] {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
      .kernel(DispatchKey::CPU, [] (const int64_t&) {})
      .catchAllKernel([] (int64_t) {}));
  }, "Mismatch in kernel C++ signatures");
}

TEST(OperatorRegistrationTest, givenLambdaKernel_whenRegisteringBackendAndCatchAllWithMismatchingCppSignatures_thenFails) {
  expectThrows<MSharp::Error>([] {
    auto registrar = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
      .catchAllKernel([] (const int64_t&) {})
      .kernel(DispatchKey::CPU, [] (int64_t) {}));
  }, "Mismatch in kernel C++ signatures");
}

TEST(OperatorRegistrationTest, givenLambdaKernel_whenAccessingWithMismatchingCppSignatures_thenFails) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
    .kernel(DispatchKey::CPU, [] (int64_t) {}));
  expectThrows<MSharp::Error>([] {
    MSharp::Dispatcher::singleton().findSchemaOrThrow("_test::dummy", "")
      .typed<void(const int64_t&)>();
  }, "Tried to access or call an operator with a wrong signature.\n  operator: _test::dummy(int _0) -> ()");
}

TEST(OperatorRegistrationTest, givenLambdaKernel_whenAccessingCatchAllWithMismatchingCppSignatures_thenFails) {
  auto registrar = MSharp::RegisterOperators().op("_test::dummy", MSharp::RegisterOperators::options()
    .catchAllKernel([] (int64_t) {}));
  expectThrows<MSharp::Error>([] {
    MSharp::Dispatcher::singleton().findSchemaOrThrow("_test::dummy", "")
      .typed<void(const int64_t&)>();
  }, "Tried to access or call an operator with a wrong signature.\n  operator: _test::dummy(int _0) -> ()");
}

TEST(OperatorRegistrationTest, givenTorchLibrary_whenRegisteringWithMismatchingCppSignatures_thenFails) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("dummy(int a) -> ()");
  m.impl("dummy", DispatchKey::CPU, [] (int64_t) {});
  expectThrows<MSharp::Error>([&] {
    m.impl("dummy", DispatchKey::CUDA, [] (const int64_t&) {});
  }, "Mismatch in kernel C++ signatures");
}

TEST(OperatorRegistrationTest, givenTorchLibrary_whenAccessingWithMismatchingCppSignatures_thenFails) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("dummy(int a) -> ()");
  m.impl("dummy", DispatchKey::CPU, [] (int64_t) {});
  expectThrows<MSharp::Error>([] {
    MSharp::Dispatcher::singleton().findSchemaOrThrow("_test::dummy", "")
      .typed<void(const int64_t&)>();
  }, "Tried to access or call an operator with a wrong signature.\n  operator: _test::dummy(int a) -> ()");
}

TEST(OperatorRegistrationTest, givenTorchLibrary_whenAccessingCatchAllWithMismatchingCppSignatures_thenFails) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("dummy(int a) -> ()", [] (int64_t) {});
  expectThrows<MSharp::Error>([] {
    MSharp::Dispatcher::singleton().findSchemaOrThrow("_test::dummy", "")
      .typed<void(const int64_t&)>();
  }, "Tried to access or call an operator with a wrong signature.\n  operator: _test::dummy(int a) -> ()");
}

/**
 * This is used to check that a given type works correctly when passed as input
 * to or as output from a kernel.
 *
 * Call ArgTypeTestKernel<Input, Output>::test(input, inputExpectation, output, outputExpectation, schema)
 * to test that a kernel with `Input` as input type and `Output` as output types,
 * when called with `input` fulfills `inputExpectation` inside the kernel, then
 * returns `output` and the returned value fulfills `outputExpectation`.
 *
 * `inputExpectation` and `outputExpectation` should be lambdas that run
 * googletest expect macros (or use other ways to assert the expectation is met).
 *
 * Optionally, you can specify the argument list part of a function schema
 * (e.g. "(Series a) -> Series") as an additional argument to use when
 * registering the kernel. In this case, the operator registration logic will
 * check that the kernel function signature matches the one you specified.
 */
struct TestModernAPI final {};
struct TestLegacyAPI final {};
struct TestModernAndLegacyAPI final {};

template<class InputType, class OutputType = InputType>
struct ArgTypeTestKernel final : OperatorKernel {
  explicit ArgTypeTestKernel(InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output)
  : input_(std::move(input)), inputExpectation_(std::move(inputExpectation)), output_(std::move(output)) {}

  OutputType operator()(InputType input) const {
    inputExpectation_(std::move(input));
    return output_;
  }

  static void test(TestModernAndLegacyAPI, InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output, std::function<void(const MSharp::Stack&)> outputExpectation, const std::string& schema) {
    test(TestModernAPI(), input, inputExpectation, output, outputExpectation, schema);
    test(TestLegacyAPI(), input, inputExpectation, output, outputExpectation, schema);
  }

  static void test(TestModernAPI, InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output, std::function<void(const MSharp::Stack&)> outputExpectation, const std::string& schema) {
    return test_([&] {
      return MSharp::RegisterOperators().op("_test::my_op" + schema, MSharp::RegisterOperators::options().catchAllKernel<ArgTypeTestKernel>(input, inputExpectation, output));
    }, input, inputExpectation, output, outputExpectation, schema);
  }

  static void test(TestLegacyAPI, InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output, std::function<void(const MSharp::Stack&)> outputExpectation, const std::string& schema) {
    return test_([&] {
      return MSharp::RegisterOperators().op("_test::my_op" + schema, [=] (InputType input) -> OutputType {
        inputExpectation(std::move(input));
        return output;
      });
    }, input, inputExpectation, output, outputExpectation, schema);
  }

private:
  static void test_(std::function<MSharp::RegisterOperators()> registration, InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output, std::function<void(const MSharp::Stack&)> outputExpectation, const std::string& schema) {
    auto registry = registration();
    auto op = Dispatcher::singleton().findSchema({"_test::my_op", ""});
    ASSERT_TRUE(op.has_value()); // assert schema is registered
    auto actualOutput = callOp(*op, input);
    outputExpectation(actualOutput);
  }

  InputType input_;
  std::function<void(const InputType&)> inputExpectation_;
  OutputType output_;
  std::string schema_;
};

template<class InputType, class OutputType = InputType>
struct testArgTypes final {
  template<class APIType = TestModernAndLegacyAPI>
  static void test(InputType input, std::function<void(const InputType&)> inputExpectation, OutputType output, std::function<void(const IValue&)> outputExpectation, const std::string& schema) {
    // Test with explicitly specified schema
    ArgTypeTestKernel<InputType, OutputType>::test(
      APIType(), input, inputExpectation, output, [&] (const MSharp::Stack& output) {
        EXPECT_EQ(1, output.size());
        outputExpectation(output[0]);
      }, schema
    );

    // Test with inferred schema
    ArgTypeTestKernel<InputType, OutputType>::test(
      APIType(), input, inputExpectation, output, [&] (const MSharp::Stack& output) {
        EXPECT_EQ(1, output.size());
        outputExpectation(output[0]);
      }, ""
    );

    // Test taking argument and returning nothing
    ArgTypeTestKernel<InputType, std::tuple<>>::test(
      APIType(), input, inputExpectation, {}, [] (const MSharp::Stack&) {}, ""
    );

    // Test taking argument and returning multiple outputs
    ArgTypeTestKernel<InputType, std::tuple<int64_t, OutputType>>::test(
      APIType(), input, inputExpectation, std::tuple<int64_t, OutputType>{3, output}, [&] (const MSharp::Stack& output) {
        EXPECT_EQ(2, output.size());
        EXPECT_EQ(3, output[0].toInt());
        outputExpectation(output[1]);
      }, ""
    );
  }
};

TEST(OperatorRegistrationTest, testAvailableArgTypes) {
  // TODO Test Scalar

  // primitive types
  testArgTypes<double>::test(
    1.5, [] (const double& v) {EXPECT_EQ(1.5, v);},
    2.5, [] (const IValue& v) {EXPECT_EQ(2.5, v.toDouble());},
    "(float a) -> float");
  testArgTypes<int64_t>::test(
    1, [] (const int64_t& v) {EXPECT_EQ(1, v);},
    2, [] (const IValue& v) {EXPECT_EQ(2, v.toInt());},
    "(int a) -> int");
  testArgTypes<bool>::test(
    true, [] (const bool& v) {EXPECT_EQ(true, v);},
    false, [] (const IValue& v) {EXPECT_EQ(false, v.toBool());},
    "(bool a) -> bool");
  testArgTypes<bool>::test(
    false, [] (const bool& v) {EXPECT_EQ(false, v);},
    true, [] (const IValue& v) {EXPECT_EQ(true, v.toBool());},
    "(bool a) -> bool");
  testArgTypes<std::string>::test(
    "string1", [] (const std::string& v) {EXPECT_EQ("string1", v);},
    "string2", [] (const IValue& v) {EXPECT_EQ("string2", v.toStringRef());},
    "(str a) -> str");
  testArgTypes<Series>::test(
    dummyTensor(MSharp::DispatchKey::CPU), [] (const Series& v) {EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v));},
    dummyTensor(MSharp::DispatchKey::CUDA), [] (const IValue& v) {EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.toTensor()));},
    "(Series a) -> Series");


  // optional types (with has_value() == true)
  testArgTypes<std::optional<double>>::test(
    std::optional<double>(1.5), [] (const std::optional<double>& v) {EXPECT_EQ(1.5, v.value());},
    std::optional<double>(2.5), [] (const IValue& v) {EXPECT_EQ(2.5, v.toDouble());},
    "(float? a) -> float?");
  testArgTypes<std::optional<int64_t>>::test(
    std::optional<int64_t>(1), [] (const std::optional<int64_t>& v) {EXPECT_EQ(1, v.value());},
    std::optional<int64_t>(2), [] (const IValue& v) {EXPECT_EQ(2, v.toInt());},
    "(int? a) -> int?");
  testArgTypes<std::optional<bool>>::test(
    std::optional<bool>(true), [] (const std::optional<bool>& v) {EXPECT_EQ(true, v.value());},
    std::optional<bool>(false), [] (const IValue& v) {EXPECT_EQ(false, v.toBool());},
    "(bool? a) -> bool?");
  testArgTypes<std::optional<bool>>::test(
    std::optional<bool>(false), [] (const std::optional<bool>& v) {EXPECT_EQ(false, v.value());},
    std::optional<bool>(true), [] (const IValue& v) {EXPECT_EQ(true, v.toBool());},
    "(bool? a) -> bool?");
  testArgTypes<std::optional<std::string>>::test(
    std::optional<std::string>("string1"), [] (const std::optional<std::string>& v) {EXPECT_EQ("string1", v.value());},
    std::optional<std::string>("string2"), [] (const IValue& v) {EXPECT_EQ("string2", v.toStringRef());},
    "(str? a) -> str?");
  testArgTypes<std::optional<Series>>::test(
    std::optional<Series>(dummyTensor(MSharp::DispatchKey::CPU)), [] (const std::optional<Series>& v) {EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.value()));},
    std::optional<Series>(dummyTensor(MSharp::DispatchKey::CUDA)), [] (const IValue& v) {EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.toTensor()));},
    "(Series? a) -> Series?");


  // optional types (with has_value() == false)
  testArgTypes<std::optional<double>>::test(
    std::optional<double>(std::nullopt), [] (const std::optional<double>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<double>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(float? a) -> float?");
  testArgTypes<std::optional<int64_t>>::test(
    std::optional<int64_t>(std::nullopt), [] (const std::optional<int64_t>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<int64_t>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(int? a) -> int?");
  testArgTypes<std::optional<bool>>::test(
    std::optional<bool>(std::nullopt), [] (const std::optional<bool>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<bool>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(bool? a) -> bool?");
  testArgTypes<std::optional<bool>>::test(
    std::optional<bool>(std::nullopt), [] (const std::optional<bool>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<bool>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(bool? a) -> bool?");
  testArgTypes<std::optional<std::string>>::test(
    std::optional<std::string>(std::nullopt), [] (const std::optional<std::string>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<std::string>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(str? a) -> str?");
  testArgTypes<std::optional<Series>>::test(
    std::optional<Series>(std::nullopt), [] (const std::optional<Series>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<Series>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(Series? a) -> Series?");


  // list types (with empty list)
  testArgTypes<MSharp::List<double>>::test(
    MSharp::List<double>(), [] (const MSharp::List<double>& v) {EXPECT_EQ(0, v.size());},
    MSharp::List<double>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<double>>().size());},
    "(float[] a) -> float[]");
  testArgTypes<MSharp::List<int64_t>>::test(
    MSharp::List<int64_t>(), [] (const MSharp::List<int64_t>& v) {EXPECT_EQ(0, v.size());},
    MSharp::List<int64_t>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<int64_t>>().size());},
    "(int[] a) -> int[]");
  testArgTypes<MSharp::List<bool>>::test(
    MSharp::List<bool>(), [] (const MSharp::List<bool>& v) {EXPECT_EQ(0, v.size());},
    MSharp::List<bool>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<bool>>().size());},
    "(bool[] a) -> bool[]");
  testArgTypes<MSharp::List<std::string>>::test(
    MSharp::List<std::string>(), [] (const MSharp::List<std::string>& v) {EXPECT_EQ(0, v.size());},
    MSharp::List<std::string>(), [] (const IValue& v) {EXPECT_EQ(0, v.toListRef().size());},
    "(str[] a) -> str[]");


  // list types (with non-empty list)
  testArgTypes<MSharp::List<double>>::test(
    MSharp::List<double>({1.5, 2.5}), [] (const MSharp::List<double>& v) {expectListEquals({1.5, 2.5}, v);},
    MSharp::List<double>({3.5, 4.5}), [] (const IValue& v) {expectListEquals({3.5, 4.5}, v.to<MSharp::List<double>>());},
    "(float[] a) -> float[]");
  testArgTypes<MSharp::List<int64_t>>::test(
    MSharp::List<int64_t>({1, 2}), [] (const MSharp::List<int64_t>& v) {expectListEquals({1, 2}, v);},
    MSharp::List<int64_t>({3, 4}), [] (const IValue& v) {expectListEquals({3, 4}, v.to<MSharp::List<int64_t>>());},
    "(int[] a) -> int[]");
  testArgTypes<MSharp::List<bool>>::test(
    MSharp::List<bool>({true, false}), [] (const MSharp::List<bool>& v) {expectListEquals({true, false}, v);},
    MSharp::List<bool>({true, false}), [] (const IValue& v) {expectListEquals({true, false}, v.to<MSharp::List<bool>>());},
    "(bool[] a) -> bool[]");
  testArgTypes<MSharp::List<std::string>>::test(
    MSharp::List<std::string>({"first", "second"}), [] (const MSharp::List<std::string>& v) {expectListEquals({"first", "second"}, v);},
    MSharp::List<std::string>({"first", "second"}), [] (const IValue& v) {
      EXPECT_EQ(2, v.toListRef().size());
      EXPECT_EQ("first", v.toListRef()[0].toStringRef());
      EXPECT_EQ("second", v.toListRef()[1].toStringRef());
    },
    "(str[] a) -> str[]");
  testArgTypes<MSharp::List<Series>>::test(
    MSharp::List<Series>({dummyTensor(MSharp::DispatchKey::CPU), dummyTensor(MSharp::DispatchKey::CUDA)}), [] (const MSharp::List<Series>& v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.get(0)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.get(1)));
    },
    MSharp::List<Series>({dummyTensor(MSharp::DispatchKey::CUDA), dummyTensor(MSharp::DispatchKey::CPU)}), [] (const IValue& v) {
      EXPECT_EQ(2, v.to<MSharp::List<pluss::Series>>().size());
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(0)));
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(1)));
    },
    "(Series[] a) -> Series[]");

  // ArrayRef list types (with empty list)
  testArgTypes<MSharp::ArrayRef<double>, MSharp::List<double>>::test(
    MSharp::ArrayRef<double>(), [] (MSharp::ArrayRef<double> v) {EXPECT_EQ(0, v.size());},
    MSharp::List<double>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<double>>().size());},
    "(float[] a) -> float[]");
  testArgTypes<MSharp::ArrayRef<int64_t>, MSharp::List<int64_t>>::test(
    MSharp::ArrayRef<int64_t>(), [] (MSharp::ArrayRef<int64_t> v) {EXPECT_EQ(0, v.size());},
    MSharp::List<int64_t>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<int64_t>>().size());},
    "(int[] a) -> int[]");
  testArgTypes<MSharp::ArrayRef<std::string>, MSharp::List<std::string>>::test(
    MSharp::ArrayRef<std::string>(), [] (MSharp::ArrayRef<std::string> v) {EXPECT_EQ(0, v.size());},
    MSharp::List<std::string>(), [] (const IValue& v) {EXPECT_EQ(0, v.toListRef().size());},
    "(str[] a) -> str[]");


  // list types (with non-empty list)
  testArgTypes<MSharp::ArrayRef<double>, MSharp::List<double>>::test(
    MSharp::ArrayRef<double>({1.5, 2.5}), [] (MSharp::ArrayRef<double> v) {expectListEquals({1.5, 2.5}, v);},
    MSharp::List<double>({3.5, 4.5}), [] (const IValue& v) {expectListEquals({3.5, 4.5}, v.to<MSharp::List<double>>());},
    "(float[] a) -> float[]");
  testArgTypes<MSharp::ArrayRef<int64_t>, MSharp::List<int64_t>>::test(
    MSharp::ArrayRef<int64_t>({1, 2}), [] (MSharp::ArrayRef<int64_t> v) {expectListEquals({1, 2}, v);},
    MSharp::List<int64_t>({3, 4}), [] (const IValue& v) {expectListEquals({3, 4}, v.to<MSharp::List<int64_t>>());},
    "(int[] a) -> int[]");
  testArgTypes<MSharp::ArrayRef<std::string>, MSharp::List<std::string>>::test(
    MSharp::ArrayRef<std::string>({"first", "second"}), [] (MSharp::ArrayRef<std::string> v) {expectListEquals({"first", "second"}, v);},
    MSharp::List<std::string>({"first", "second"}), [] (const IValue& v) {
      EXPECT_EQ(2, v.toListRef().size());
      EXPECT_EQ("first", v.toListRef()[0].toStringRef());
      EXPECT_EQ("second", v.toListRef()[1].toStringRef());
    },
    "(str[] a) -> str[]");
  testArgTypes<MSharp::ArrayRef<Series>, MSharp::List<Series>>::test(
    MSharp::ArrayRef<Series>({dummyTensor(MSharp::DispatchKey::CPUTensorId), dummyTensor(MSharp::DispatchKey::CUDATensorId)}), [] (MSharp::ArrayRef<Series> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPUTensorId, extractDispatchKey(v[0]));
      EXPECT_EQ(MSharp::DispatchKey::CUDATensorId, extractDispatchKey(v[1]));
    },
    MSharp::List<Series>({dummyTensor(MSharp::DispatchKey::CUDATensorId), dummyTensor(MSharp::DispatchKey::CPUTensorId)}), [] (const IValue& v) {
      EXPECT_EQ(2, v.to<MSharp::List<pluss::Series>>().size());
      EXPECT_EQ(MSharp::DispatchKey::CUDATensorId, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(0)));
      EXPECT_EQ(MSharp::DispatchKey::CPUTensorId, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(1)));
    },
    "(Series[] a) -> Series[]");


  // std::array list types (with empty list)
  testArgTypes<std::array<double, 0>>::test(
    std::array<double, 0>(), [] (std::array<double, 0> v) {},
    std::array<double, 0>(), [] (const IValue& v) {EXPECT_EQ(0, (v.to<MSharp::List<double>>().size()));},
    "(float[0] a) -> float[0]");
  testArgTypes<std::array<int64_t, 0>>::test(
    std::array<int64_t, 0>(), [] (std::array<int64_t, 0> v) {},
    std::array<int64_t, 0>(), [] (const IValue& v) {EXPECT_EQ(0, (v.to<MSharp::List<int64_t>>().size()));},
    "(int[0] a) -> int[0]");
  testArgTypes<std::array<bool, 0>>::test(
    std::array<bool, 0>(), [] (std::array<bool, 0> v) {},
    std::array<bool, 0>(), [] (const IValue& v) {EXPECT_EQ(0, (v.to<std::array<bool, 0>>().size()));},
    "(bool[0] a) -> bool[0]");
  testArgTypes<std::array<std::string, 0>>::test(
    std::array<std::string, 0>(), [] (std::array<std::string, 0> v) {EXPECT_EQ(0, v.size());},
    std::array<std::string, 0>(), [] (const IValue& v) {EXPECT_EQ(0, v.toListRef().size());},
    "(str[0] a) -> str[0]");


  // std::array list types (with non-empty list)
  testArgTypes<std::array<double, 2>>::test(
    std::array<double, 2>({1.5, 2.5}), [] (std::array<double, 2> v) {expectListEquals({1.5, 2.5}, v);},
    std::array<double, 2>({3.5, 4.5}), [] (const IValue& v) {expectListEquals({3.5, 4.5}, v.to<std::array<double, 2>>());},
    "(float[2] a) -> float[2]");
  testArgTypes<std::array<int64_t, 2>>::test(
    std::array<int64_t, 2>({1, 2}), [] (std::array<int64_t, 2> v) {expectListEquals({1, 2}, v);},
    std::array<int64_t, 2>({3, 4}), [] (const IValue& v) {expectListEquals({3, 4}, v.to<std::array<int64_t, 2>>());},
    "(int[2] a) -> int[2]");
  testArgTypes<std::array<bool, 2>>::test(
    std::array<bool, 2>({true, false}), [] (std::array<bool, 2> v) {expectListEquals({true, false}, v);},
    std::array<bool, 2>({true, false}), [] (const IValue& v) {expectListEquals({true, false}, v.to<std::array<bool, 2>>());},
    "(bool[2] a) -> bool[2]");
  testArgTypes<std::array<std::string, 2>>::test(
    std::array<std::string, 2>({"first", "second"}), [] (std::array<std::string, 2> v) {expectListEquals({"first", "second"}, v);},
    std::array<std::string, 2>({"first", "second"}), [] (const IValue& v) {
      EXPECT_EQ(2, v.toListRef().size());
      EXPECT_EQ("first", v.toListRef()[0].toStringRef());
      EXPECT_EQ("second", v.toListRef()[1].toStringRef());
    },
    "(str[2] a) -> str[2]");
  testArgTypes<std::array<Series, 2>>::test(
    std::array<Series, 2>({dummyTensor(MSharp::DispatchKey::CPUTensorId), dummyTensor(MSharp::DispatchKey::CUDATensorId)}), [] (std::array<Series, 2> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPUTensorId, extractDispatchKey(v[0]));
      EXPECT_EQ(MSharp::DispatchKey::CUDATensorId, extractDispatchKey(v[1]));
    },
    std::array<Series, 2>({dummyTensor(MSharp::DispatchKey::CUDATensorId), dummyTensor(MSharp::DispatchKey::CPUTensorId)}), [] (const IValue& v) {
      EXPECT_EQ(2, v.to<MSharp::List<pluss::Series>>().size());
      EXPECT_EQ(MSharp::DispatchKey::CUDATensorId, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(0)));
      EXPECT_EQ(MSharp::DispatchKey::CPUTensorId, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(1)));
    },
    "(Series[2] a) -> Series[2]");


  // deprecated list types (with empty list)
  testArgTypes<std::vector<double>>::test<TestLegacyAPI>(
    std::vector<double>(), [] (const std::vector<double>& v) {EXPECT_EQ(0, v.size());},
    std::vector<double>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<double>>().size());},
    "(float[] a) -> float[]");
  testArgTypes<std::vector<int64_t>>::test<TestLegacyAPI>(
    std::vector<int64_t>(), [] (const std::vector<int64_t>& v) {EXPECT_EQ(0, v.size());},
    std::vector<int64_t>(), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<int64_t>>().size());},
    "(int[] a) -> int[]");
  //Note: vector<bool> is not supported, use List<bool> instead.
  testArgTypes<std::vector<std::string>>::test<TestLegacyAPI>(
    std::vector<std::string>(), [] (const std::vector<std::string>& v) {EXPECT_EQ(0, v.size());},
    std::vector<std::string>(), [] (const IValue& v) {EXPECT_EQ(0, v.toListRef().size());},
    "(str[] a) -> str[]");


  // deprecated list types (with non-empty list)
  testArgTypes<std::vector<double>>::test<TestLegacyAPI>(
    std::vector<double>({1.5, 2.5}), [] (const std::vector<double>& v) {expectListEquals({1.5, 2.5}, v);},
    std::vector<double>({3.5, 4.5}), [] (const IValue& v) {expectListEquals({3.5, 4.5}, v.to<MSharp::List<double>>());},
    "(float[] a) -> float[]");
  testArgTypes<std::vector<int64_t>>::test<TestLegacyAPI>(
    std::vector<int64_t>({1, 2}), [] (const std::vector<int64_t>& v) {expectListEquals({1, 2}, v);},
    std::vector<int64_t>({3, 4}), [] (const IValue& v) {expectListEquals({3, 4}, v.to<MSharp::List<int64_t>>());},
    "(int[] a) -> int[]");
  //Note: vector<bool> is not supported, use List<bool> instead.
  testArgTypes<std::vector<std::string>>::test<TestLegacyAPI>(
    std::vector<std::string>({"first", "second"}), [] (const std::vector<std::string>& v) {expectListEquals({"first", "second"}, v);},
    std::vector<std::string>({"first", "second"}), [] (const IValue& v) {
      EXPECT_EQ(2, v.toListRef().size());
      EXPECT_EQ("first", v.toListRef()[0].toStringRef());
      EXPECT_EQ("second", v.toListRef()[1].toStringRef());
    },
    "(str[] a) -> str[]");
  testArgTypes<std::vector<Series>>::test<TestLegacyAPI>(
    std::vector<Series>({dummyTensor(MSharp::DispatchKey::CPU), dummyTensor(MSharp::DispatchKey::CUDA)}), [] (const std::vector<Series>& v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.at(0)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.at(1)));
    },
    std::vector<Series>({dummyTensor(MSharp::DispatchKey::CUDA), dummyTensor(MSharp::DispatchKey::CPU)}), [] (const IValue& v) {
      EXPECT_EQ(2, v.to<MSharp::List<pluss::Series>>().size());
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(0)));
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.to<MSharp::List<pluss::Series>>().get(1)));
    },
    "(Series[] a) -> Series[]");

  // Test optional of list (with nullopt)
  testArgTypes<std::optional<MSharp::List<int64_t>>>::test(
    std::optional<MSharp::List<int64_t>>(std::nullopt), [] (const std::optional<MSharp::List<int64_t>>& v) {EXPECT_FALSE(v.has_value());},
    std::optional<MSharp::List<int64_t>>(std::nullopt), [] (const IValue& v) {EXPECT_TRUE(v.isNone());},
    "(int[]? a) -> int[]?");

  // Test optional of list (with empty list)
  testArgTypes<std::optional<MSharp::List<int64_t>>>::test(
    std::optional<MSharp::List<int64_t>>(MSharp::List<int64_t>({})), [] (const std::optional<MSharp::List<int64_t>>& v) {EXPECT_EQ(0, v.value().size());},
    std::optional<MSharp::List<int64_t>>(MSharp::List<int64_t>({})), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<int64_t>>().size());},
    "(int[]? a) -> int[]?");

  // Test optional of list (with values)
  testArgTypes<std::optional<MSharp::List<int64_t>>>::test(
    std::optional<MSharp::List<int64_t>>(MSharp::List<int64_t>({1, 2})), [] (const std::optional<MSharp::List<int64_t>>& v) {expectListEquals({1, 2}, v.value());},
    std::optional<MSharp::List<int64_t>>(MSharp::List<int64_t>({3, 4})), [] (const IValue& v) {expectListEquals({3, 4}, v.to<MSharp::List<int64_t>>());},
    "(int[]? a) -> int[]?");

  // Test list of optional (with empty list)
  testArgTypes<MSharp::List<::std::optional<int64_t>>>::test(
    MSharp::List<::std::optional<int64_t>>(MSharp::List<::std::optional<int64_t>>({})), [] (const MSharp::List<::std::optional<int64_t>>& v) {EXPECT_EQ(0, v.size());},
    MSharp::List<::std::optional<int64_t>>(MSharp::List<::std::optional<int64_t>>({})), [] (const IValue& v) {EXPECT_EQ(0, v.to<MSharp::List<::std::optional<int64_t>>>().size());},
    "(int?[] a) -> int?[]");

  // Test list of optional (with values)
  testArgTypes<MSharp::List<::std::optional<int64_t>>>::test(
    MSharp::List<::std::optional<int64_t>>(MSharp::List<::std::optional<int64_t>>({3, std::nullopt, 2})), [] (const MSharp::List<::std::optional<int64_t>>& v) {expectListEquals<std::optional<int64_t>>({3, std::nullopt, 2}, v);},
    MSharp::List<::std::optional<int64_t>>(MSharp::List<::std::optional<int64_t>>({3, std::nullopt, 2})), [] (const IValue& v) {expectListEquals<std::optional<int64_t>>({3, std::nullopt, 2}, v.to<MSharp::List<::std::optional<int64_t>>>());},
    "(int?[] a) -> int?[]");

  // dict types
  MSharp::Dict<std::string, std::string> str_dict;
  str_dict.insert("key1", "value1");
  str_dict.insert("key2", "value2");
  testArgTypes<MSharp::Dict<std::string, std::string>>::test(
    str_dict, [] (MSharp::Dict<std::string, std::string> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ("value1", v.at("key1"));
      EXPECT_EQ("value2", v.at("key2"));
    },
    str_dict, [] (const IValue& v) {
      MSharp::Dict<std::string, std::string> dict = MSharp::impl::toTypedDict<std::string, std::string>(v.toGenericDict());
      EXPECT_EQ(2, dict.size());
      EXPECT_EQ("value1", dict.at("key1"));
      EXPECT_EQ("value2", dict.at("key2"));
    },
    "(Dict(str, str) a) -> Dict(str, str)");
  MSharp::Dict<int64_t, Series> tensor_dict;
  tensor_dict.insert(1, dummyTensor(MSharp::DispatchKey::CPU));
  tensor_dict.insert(2, dummyTensor(MSharp::DispatchKey::CUDA));
  testArgTypes<MSharp::Dict<int64_t, Series>>::test(
    tensor_dict, [] (MSharp::Dict<int64_t, Series> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.at(1)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.at(2)));
    },
    tensor_dict, [] (const IValue& v) {
      MSharp::Dict<int64_t, Series> dict = MSharp::impl::toTypedDict<int64_t, Series>(v.toGenericDict());
      EXPECT_EQ(2, dict.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(dict.at(1)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(dict.at(2)));
    },
    "(Dict(int, Series) a) -> Dict(int, Series)");

  // deprecated dict types
  std::unordered_map<std::string, std::string> str_map;
  str_map.emplace("key1", "value1");
  str_map.emplace("key2", "value2");
  testArgTypes<std::unordered_map<std::string, std::string>>::test<TestLegacyAPI>(
    str_map, [] (std::unordered_map<std::string, std::string> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ("value1", v.at("key1"));
      EXPECT_EQ("value2", v.at("key2"));
    },
    str_map, [] (const IValue& v) {
      MSharp::Dict<std::string, std::string> dict = MSharp::impl::toTypedDict<std::string, std::string>(v.toGenericDict());
      EXPECT_EQ(2, dict.size());
      EXPECT_EQ("value1", dict.at("key1"));
      EXPECT_EQ("value2", dict.at("key2"));
    },
    "(Dict(str, str) a) -> Dict(str, str)");
  std::unordered_map<int64_t, Series> tensor_map;
  tensor_map.emplace(1, dummyTensor(MSharp::DispatchKey::CPU));
  tensor_map.emplace(2, dummyTensor(MSharp::DispatchKey::CUDA));
  testArgTypes<std::unordered_map<int64_t, Series>>::test<TestLegacyAPI>(
    tensor_map, [] (std::unordered_map<int64_t, Series> v) {
      EXPECT_EQ(2, v.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(v.at(1)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(v.at(2)));
    },
    tensor_map, [] (const IValue& v) {
      MSharp::Dict<int64_t, Series> dict = MSharp::impl::toTypedDict<int64_t, Series>(v.toGenericDict());
      EXPECT_EQ(2, dict.size());
      EXPECT_EQ(MSharp::DispatchKey::CPU, extractDispatchKey(dict.at(1)));
      EXPECT_EQ(MSharp::DispatchKey::CUDA, extractDispatchKey(dict.at(2)));
    },
    "(Dict(int, Series) a) -> Dict(int, Series)");

  // weird deeply nested type
  using DeeplyNestedType = MSharp::List<MSharp::Dict<std::string, MSharp::List<::std::optional<MSharp::Dict<int64_t, std::string>>>>>;
  auto makeDeeplyNestedObject = [] () -> DeeplyNestedType {
    MSharp::Dict<int64_t, std::string> inner3;
    inner3.insert(1, "1");
    MSharp::List<::std::optional<MSharp::Dict<int64_t, std::string>>> inner2;
    inner2.push_back(std::move(inner3));
    MSharp::Dict<std::string, MSharp::List<::std::optional<MSharp::Dict<int64_t, std::string>>>> inner1;
    inner1.insert("key", std::move(inner2));
    MSharp::List<MSharp::Dict<std::string, MSharp::List<::std::optional<MSharp::Dict<int64_t, std::string>>>>> result;
    result.push_back(inner1);
    return result;
  };
  testArgTypes<DeeplyNestedType>::test(
    makeDeeplyNestedObject(), [] (const DeeplyNestedType& v) {EXPECT_EQ("1", v.get(0).at("key").get(0).value().at(1));},
    makeDeeplyNestedObject(), [] (const IValue& v) {EXPECT_EQ("1", v.to<DeeplyNestedType>().get(0).at("key").get(0).value().at(1));},
    "(Dict(str, Dict(int, str)?[])[] a) -> Dict(str, Dict(int, str)?[])[]");
}

TEST(NewOperatorRegistrationTest, erroroutwithinvalidoverloadname) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  expectThrows<MSharp::Error>([&] {
   m.def("dummy.default(Series self) -> Series");
  }, "default is not a legal overload name for aten operators");
  expectThrows<MSharp::Error>([&] {
   m.def("dummy.__name__(Series self) -> Series");
  }, "__name__ is not a legal overload name for aten operators");
}

TEST(NewOperatorRegistrationTest, testBasics) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("dummy(Series self) -> Series");
  m.def("dummy1(Series self) -> Series");
  m.def("dummy2(Series self) -> Series");
  m.def("dummy3(Series self, Series other) -> Series", [](const Series& self, const Series& other) { return self; });
  m.def("dummy4", [](const Series& self, const Series& other) { return other; });
  m.impl("dummy", MSharp::DeviceType::CPU, [](const Series& self) { return self; });
  m.impl("dummy", MSharp::DeviceType::XLA, [](const Series& self) { return self; });
  m.impl("dummy", MSharp::DeviceType::Lazy, [](const Series& self) { return self; });
  // Internal API
  m.impl("dummy2", MSharp::DispatchKey::CPU, [](const Series& self) { return self; });
  m.impl("dummy2", MSharp::DispatchKey::XLA, [](const Series& self) { return self; });
  m.impl("dummy2", MSharp::DispatchKey::Lazy, [](const Series& self) { return self; });

  ASSERT_TRUE(Dispatcher::singleton().findSchema({"_test::dummy", ""}).has_value());
  // Should have a schema even if there are no impls
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"_test::dummy1", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"_test::dummy2", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"_test::dummy3", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"_test::dummy4", ""}).has_value());
}

TEST(NewOperatorRegistrationTest, importTopLevel) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("def1(Series self) -> Series");
  m.def("def2(Series self) -> Series", [](const Series& x) { return x; });
  m.def("def3", [](const Series& x) { return x; });

  auto m2 = MAKE_TORCH_LIBRARY_IMPL(test, CatchAll);
  m2.impl("impl1", [](const Series& x) { return x; });

  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def1", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def2", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def3", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findOp({"test::def1", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findOp({"test::def2", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findOp({"test::def3", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findOp({"test::impl1", ""}).has_value());
}

TEST(NewOperatorRegistrationTest, overload) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series self) -> Series");
  m.def("fn.overload1(Series self, Series other) -> Series");
  m.def("fn.overload2(Series self, Series other, Series alpha) -> Series");

  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::fn", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::fn", "overload1"}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::fn", "overload2"}).has_value());
}

TEST(NewOperatorRegistrationTest, importNamespace) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("def1(Series self) -> Series");
  m.def("def2(Series self) -> Series", [](const Series& x) { return x; });
  m.def("def3", [](const Series& x) { return x; });
  m.impl("impl1", [](const Series& x) { return x; });
  expectThrows<MSharp::Error>([&] {
    m.def("retest::def1(Series self) -> Series");
  }, "");

  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def1", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def2", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def3", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findOp({"test::impl1", ""}).has_value());
}

TEST(NewOperatorRegistrationTest, schema) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("def1(Series self) -> Series");
  m.def(torch::schema("def2(Series self) -> Series"));
  m.def(torch::schema("def3(Series self) -> Series", MSharp::AliasAnalysisKind::PURE_FUNCTION));
  m.def(torch::jit::parseSchema("def4(Series self) -> Series"));

  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def1", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def2", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def3", ""}).has_value());
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def4", ""}).has_value());

  EXPECT_EQ(Dispatcher::singleton().findSchema({"test::def1", ""})->schema().aliasAnalysis(), MSharp::AliasAnalysisKind::FROM_SCHEMA);
  EXPECT_EQ(Dispatcher::singleton().findSchema({"test::def2", ""})->schema().aliasAnalysis(), MSharp::AliasAnalysisKind::FROM_SCHEMA);
  EXPECT_EQ(Dispatcher::singleton().findSchema({"test::def3", ""})->schema().aliasAnalysis(), MSharp::AliasAnalysisKind::PURE_FUNCTION);
  ASSERT_TRUE(Dispatcher::singleton().findSchema({"test::def4", ""})->schema().isDefaultAliasAnalysisKind());
}

TEST(NewOperatorRegistrationTest, whenRegisteringBackendFallbackKernelAndCatchallKernelForSameBackend_thenCallsFallbackKernel) {
  auto m1 = MAKE_TORCH_LIBRARY_IMPL(_, CPU);
  m1.fallback(CppFunction::makeFromBoxedFunction<&backend_fallback_kernel>());

  bool called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series t, str input) -> ()");
  m.impl("fn", [&] (Series, std::string) { called = true; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  // CatchAll now maps to CompositeImplicitAutograd and has higher precedence than backend fallback.
  EXPECT_TRUE(called);
}

TEST(NewOperatorRegistrationTest, whenRegisteringAutogradKernelWithRegularKernel_thenCanCallRegularKernel) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series dummy) -> ()");
  m.impl("fn", MSharp::DispatchKey::CPU, nonautograd_kernel);
  m.impl("fn", MSharp::DispatchKey::Autograd, autograd_kernel);

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  called_nonautograd = called_autograd = false;
  callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_TRUE(called_nonautograd);
  EXPECT_FALSE(called_autograd);
}

TEST(NewOperatorRegistrationTest, dispatchWithCompositeImplicitAutogradKernel) {
  bool math_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; }));

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    ASSERT_FALSE(math_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(math_called);
  }

  {
    math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    math_called = false;
    callOp(*op, dummyTensor(key));
    ASSERT_TRUE(math_called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    math_called = false;
    callOp(*op, dummyTensor(key, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
  }

  {
    math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU));
    ASSERT_TRUE(math_called);
  }

  {
    math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchWithCompositeImplicitAutogradAndAutogradKernel) {
  bool math_called = false;
  bool autograd_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::Autograd, [&](const Series& x) { autograd_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  // CompositeImplicitAutograd has higher precedence than Autograd
  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(autograd_called);
  }

  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(autograd_called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchWithCompositeImplicitAutogradAndCatchAllKernel) {
  bool math_called = false;
  bool catchall_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; }));
  m.impl("fn", [&](const Series& x) { catchall_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  // catchAll now maps to CompositeImplicitAutograd, which means we have two registrations to CompositeImplicitAutograd key.
  // The last registration is used.
  {
    catchall_called = math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_FALSE(math_called);
    ASSERT_TRUE(catchall_called);
  }

  {
    catchall_called = math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_FALSE(math_called);
    ASSERT_TRUE(catchall_called);
  }
}

TEST(NewOperatorRegistrationTest, AutogradBackendOverridesCompositeImplicitAutogradKernel) {
  bool math_called = false;
  bool autograd_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::AutogradCPU, [&](const Series& x) { autograd_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(autograd_called);
  }

  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);
    ASSERT_FALSE(math_called);
  }

  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(autograd_called);
  }

  {
    math_called = autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(autograd_called);
  }
}

TEST(NewOperatorRegistrationTest, BackendOverridesCompositeImplicitAutogradKernel) {
  bool math_called = false;
  bool backend_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::CPU, [&](const Series& x) { backend_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    math_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(math_called);
  }

  {
    // Fallthrough AutogradCPU and reaches CPU
    math_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(math_called);
  }

  {
    math_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(backend_called);
  }

  {
    math_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(backend_called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchWithCompositeExplicitAutogradKernel) {
  bool called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeExplicitAutograd, [&](const Series& x) { called = true; return x; }));

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    ASSERT_FALSE(called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(called);
  }

  {
    called = false;
    // AutogradCPU is fallthrough, calls CPU kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    called = false;
    callOp(*op, dummyTensor(key));
    ASSERT_TRUE(called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    called = false;
    // Autograd{XLA, Lazy} is fallthrough, calls XLA / Lazy kernel
    callOp(*op, dummyTensor(key, /*requires_grad=*/true));
    ASSERT_TRUE(called);
  }

  {
    called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU));
    ASSERT_TRUE(called);
  }

  {
    called = false;
    // AutogradCPU is fallthrough, calls CPU kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU, /*requires_grad=*/true));
    ASSERT_TRUE(called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchWithCompositeExplicitAutogradAndCompositeImplicitAutogradKernel) {
  bool backend_called = false;
  bool math_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeExplicitAutograd, [&](const Series& x) { backend_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    backend_called = math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(math_called);
  }

  {
    backend_called = math_called = false;
    // AutogradCPU is fallthrough, calls CPU kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_FALSE(math_called);
    ASSERT_TRUE(backend_called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    backend_called = math_called = false;
    callOp(*op, dummyTensor(key));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(math_called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    backend_called = math_called = false;
    // Autograd{XLA, Lazy} is fallthrough, calls XLA / Lazy kernel
    callOp(*op, dummyTensor(key, /*requires_grad=*/true));
    ASSERT_FALSE(math_called);
    ASSERT_TRUE(backend_called);
  }

  {
    backend_called = math_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(math_called);
  }

  {
    backend_called = math_called = false;
    // AutogradOther is fallthrough, calls SparseCPU kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::SparseCPU, /*requires_grad=*/true));
    ASSERT_FALSE(math_called);
    ASSERT_TRUE(backend_called);
  }
}

TEST(NewOperatorRegistrationTest, BackendOverridesCompositeExplicitAutogradKernel) {
  bool default_called = false;
  bool backend_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CompositeExplicitAutograd, [&](const Series& x) { default_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::CPU, [&](const Series& x) { backend_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    default_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(default_called);
  }

  {
    default_called = backend_called = false;
    // AutogradCPU is fallthrough, calls CPU kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(backend_called);
    ASSERT_FALSE(default_called);
  }

  {
    default_called = backend_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
    ASSERT_TRUE(default_called);
    ASSERT_FALSE(backend_called);
  }

  {
    default_called = backend_called = false;
    // AutogradCUDA is fallthrough, calls CUDA kernel
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA, /*requires_grad=*/true));
    ASSERT_TRUE(default_called);
    ASSERT_FALSE(backend_called);
  }
}


TEST(NewOperatorRegistrationTest, dispatch) {
  bool cpu_called = false;
  bool cuda_called = false;
  bool autograd_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn_cpu", torch::dispatch(MSharp::DispatchKey::CPU, [&](const Series& x) { cpu_called = true; return x; }));
  m.def("fn_cuda", torch::dispatch(MSharp::kCUDA, [&](const Series& x) { cuda_called = true; return x; }));
  m.def("fn_autograd", torch::dispatch(MSharp::kAutograd, [&](const Series& x) { autograd_called = true; return x; }));

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn_cpu", ""});
    ASSERT_TRUE(op.has_value());
    ASSERT_FALSE(cpu_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(cpu_called);
  }

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn_cuda", ""});
    ASSERT_TRUE(op.has_value());
    ASSERT_FALSE(cuda_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
    ASSERT_TRUE(cuda_called);
  }

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn_autograd", ""});
    ASSERT_TRUE(op.has_value());
    ASSERT_FALSE(autograd_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);
  }

  for (MSharp::DispatchKey key : {MSharp::DispatchKey::XLA, MSharp::DispatchKey::Lazy}) {
    autograd_called = false;
    auto op = Dispatcher::singleton().findSchema({"test::fn_autograd", ""});
    ASSERT_TRUE(op.has_value());
    callOp(*op, dummyTensor(key, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchAutogradPrecedence) {
  bool cpu_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::CPU, [&](const Series& x) { cpu_called = true; return x; }));

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    ASSERT_FALSE(cpu_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(cpu_called);
  }

  {
    // AutogradCPU is fallthrough, use CPU kernel
    cpu_called = false;
    auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(cpu_called);
  }

  bool autograd_called = false;
  m.impl("fn", MSharp::kAutograd, [&](const Series& x) { autograd_called = true; return x; });

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);
  }

  // Autograd backend kernel has higher precedence than Autograd alias.
  bool autogradcpu_called = false;
  m.impl("fn", MSharp::DispatchKey::AutogradCPU, [&](const Series& x) { autogradcpu_called = true; return x; });

  {
    auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(autogradcpu_called);
  }
}

TEST(NewOperatorRegistrationTest, throwsWhenRegisterToBackendMapsToAutogradOther) {
  // NOLINTNEXTLINE(cppcoreguidelines-init-variables)
  bool fpga_called, math_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::FPGA, [&](const Series& x) { fpga_called = true; return x; }));
  m.impl("fn", MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    callOp(*op, dummyTensor(MSharp::DispatchKey::FPGA));
    ASSERT_TRUE(fpga_called);
  }

  {
    expectThrows<MSharp::Error>([&] {
      callOp(*op, dummyTensor(MSharp::DispatchKey::FPGA, /*requires_grad=*/true));
    }, "test::fn has kernels registered to both CompositeImplicitAutograd and a backend mapped to AutogradOther.");
  }
}

TEST(NewOperatorRegistrationTest, dispatchMultipleTensors) {
  bool privateuse1_called = false;
  bool catchall_called = false;
  // Similar to in-tree AutogradCPU/AutogradCUDA etc, out-of-tree backends usually register
  // a fallthrough kernel for AutogradPrivateUse1.
  auto m1 = MAKE_TORCH_LIBRARY_IMPL(_, AutogradPrivateUse1);
  m1.fallback(CppFunction::makeFallthrough());

  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn", torch::dispatch(MSharp::DispatchKey::PrivateUse1, [&](const Series& x, const Series& y) { privateuse1_called = true; return x; }));
  m.impl("fn", [&](const Series& x, const Series& y) { catchall_called = true; return x; });

  {
    auto op = Dispatcher::singleton().findOp({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    callOp(*op, dummyTensor(MSharp::DispatchKey::PrivateUse1), dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(privateuse1_called);
  }

  {
    auto op = Dispatcher::singleton().findOp({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    ASSERT_FALSE(catchall_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(catchall_called);
  }

  {
    auto op = Dispatcher::singleton().findOp({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    catchall_called = false;
    callOp(*op,
           dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true),
           dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(catchall_called);
  }

  {
    auto op = Dispatcher::singleton().findOp({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    catchall_called = false;
    privateuse1_called = false;
    callOp(*op,
           dummyTensor(MSharp::DispatchKey::PrivateUse1, /*requires_grad=*/true),
           dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_FALSE(catchall_called);
    ASSERT_TRUE(privateuse1_called);
  }

  m.impl("fn", MSharp::DispatchKey::AutogradPrivateUse1, [&](const Series& x, const Series& y) { privateuse1_called = true; return x; });

  {
    auto op = Dispatcher::singleton().findOp({"test::fn", ""});
    ASSERT_TRUE(op.has_value());
    privateuse1_called = false;
    callOp(*op,
           dummyTensor(MSharp::DispatchKey::PrivateUse1, /*requires_grad=*/true),
           dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(privateuse1_called);
  }
}

TEST(NewOperatorRegistrationTest, registerCompositeImplicitAutogradWithCPUKernel_andCallAutogradOtherKernel_callsComposite) {
  bool math_called = false;
  bool cpu_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series dummy) -> Series");
  m.impl("fn", MSharp::DispatchKey::CPU, [&](const Series& x) { cpu_called = true; return x; });
  m.impl("fn", MSharp::DispatchKey::CompositeImplicitAutograd, [&](const Series& x) { math_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    math_called = cpu_called = false;
    // Meta should redispatch to the AutogradOther backend,
    // which the composite kernel should be registered to.
    callOp(*op, dummyTensor(MSharp::DispatchKey::Meta, /*requires_grad=*/true));
    ASSERT_TRUE(math_called);
    ASSERT_FALSE(cpu_called);
  }
}

TEST(NewOperatorRegistrationTest, dispatchMultiple) {
  bool cpu_called = false;
  bool cuda_called = false;
  bool autograd_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series self) -> Series");
  // NB: Direct use of DispatchKey is discouraged; use the DeviceType
  // k-synonyms instead
  m.impl("fn", MSharp::DispatchKey::CPU, [&](const Series& x) { cpu_called = true; return x; });
  m.impl("fn", MSharp::kCUDA, [&](const Series& x) { cuda_called = true; return x; });
  m.impl("fn", MSharp::kAutograd, [&](const Series& x) { autograd_called = true; return x; });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  {
    ASSERT_FALSE(cpu_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
    ASSERT_TRUE(cpu_called);

    ASSERT_FALSE(cuda_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA));
    ASSERT_TRUE(cuda_called);
  }

  {
    ASSERT_FALSE(autograd_called);
    callOp(*op, dummyTensor(MSharp::DispatchKey::CPU, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);

    autograd_called = false;
    callOp(*op, dummyTensor(MSharp::DispatchKey::CUDA, /*requires_grad=*/true));
    ASSERT_TRUE(autograd_called);
  }
}

TEST(NewOperatorRegistrationTest, fallback) {
  auto m = MAKE_TORCH_LIBRARY_IMPL(_, CPU);
  m.fallback(CppFunction::makeFromBoxedFunction<&backend_fallback_kernel>());

  auto registrar1 = MSharp::RegisterOperators().op("_test::dummy(Series dummy, str input) -> ()");

  auto op = Dispatcher::singleton().findSchema({"_test::dummy", ""});
  ASSERT_TRUE(op.has_value());
  auto stack = callOp(*op, dummyTensor(MSharp::DispatchKey::CPU), "hello ");
  EXPECT_EQ("hello _test::dummy", stack[1].toStringRef());
}

TEST(NewOperatorRegistrationTest, BackendSelectRedispatchesToCPU) {
  bool cpu_called = false;
  bool backend_generic_called = false;
  auto m = MAKE_TORCH_LIBRARY(test);
  auto after_backend_select = MSharp::DispatchKeySet(MSharp::DispatchKeySet::FULL_AFTER, MSharp::DispatchKey::BackendSelect);
  m.def("fn(Series self) -> Series");
  m.impl("fn", MSharp::kCPU, [&](const Series& x) { cpu_called = true; return x; });
  m.impl("fn", MSharp::DispatchKey::BackendSelect, [&](MSharp::DispatchKeySet ks, const Series& x) {
     backend_generic_called = true;
     auto op = MSharp::Dispatcher::singleton().findSchema({"test::fn", ""}).value().typed<Series (const Series&)>();
     return MSharp::Dispatcher::singleton().redispatch<Series, const Series&>(op, ks & after_backend_select, x);
   });

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());
  callOp(*op, dummyTensor(MSharp::DispatchKey::CPU));
  ASSERT_TRUE(cpu_called);
  ASSERT_TRUE(backend_generic_called);
}

TEST(NewOperatorRegistrationTest, TorchLibraryTwiceIsError) {
  {
    auto m = MAKE_TORCH_LIBRARY(test);
    expectThrows<MSharp::Error>([] {
      auto m2 = MAKE_TORCH_LIBRARY(test);
    }, "Only a single TORCH_LIBRARY");
  }
  // Ensure it's ok after deregistering
  auto m = MAKE_TORCH_LIBRARY(test);
}

Series dummy_fn(const Series& x) {
  return x;
}

TEST(NewOperatorRegistrationTest, CppFunction) {
  // Just show off the possible ways to register functions
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn1", &dummy_fn);
  // C++ will implicitly convert function to function pointer
  // c.f. https://en.cppreference.com/w/cpp/language/implicit_conversion#Function_to_pointer
  m.def("fn2", dummy_fn);
  m.def("fn3", [](const Series& x) { return x; });
  // These require explicit schema
  m.def("fn4(Series x) -> Series", CppFunction::makeFallthrough());
  m.def("fn5(Series x) -> Series", CppFunction::makeFromUnboxedFunction(dummy_fn));
  m.def("fn6(Series x) -> Series", CppFunction::makeFromBoxedFunction<&backend_fallback_kernel>());
}

// Some internal tests that have to be done from C++

struct OpRegistrationListenerForDelayedListenerTest : public MSharp::OpRegistrationListener {
  int64_t num_registers_ = 0;
  int64_t num_deregisters_ = 0;
  void onOperatorRegistered(const OperatorHandle& op) override {
    num_registers_++;
  }
  void onOperatorDeregistered(const OperatorHandle& op) override {
    num_deregisters_++;
  }
};

TEST(NewOperatorRegistrationTest, testDelayedListener) {
  auto listener = std::make_unique<OpRegistrationListenerForDelayedListenerTest>();
  auto listener_ptr = listener.get();
  auto registry = Dispatcher::singleton().addRegistrationListener(std::move(listener));
  int64_t initial_num_registers = listener_ptr->num_registers_;
  int64_t initial_num_deregisters = listener_ptr->num_deregisters_;
  auto op = Dispatcher::singleton().findOp({"_test::dummy", ""});
  ASSERT_FALSE(op.has_value());
  auto m1 = MAKE_TORCH_LIBRARY_IMPL(_test, CPU);
  m1.impl("dummy", [](const Series& self) { return self; });
  EXPECT_EQ(initial_num_registers, listener_ptr->num_registers_);
  {
    auto m2 = MAKE_TORCH_LIBRARY(_test);
    m2.def("dummy(Series self) -> Series");
    EXPECT_EQ(initial_num_registers + 1, listener_ptr->num_registers_);
  }
  EXPECT_EQ(initial_num_deregisters + 1, listener_ptr->num_deregisters_);
}

TEST(NewOperatorRegistrationTest, testImplNoDefGetsCaught) {
  auto danglingImpls = Dispatcher::singleton().findDanglingImpls();
  std::string error_str = "Discovered operators that have been registered through the dispatcher"
                          " without explicitly specifying their schemas. Please do so using"
                          " the TORCH_LIBRARY macro. Suspect operators:\n";
  for (auto& op : danglingImpls) {
      auto& op_name = op.operator_name();
      error_str += "\t" + op_name.name;
      if (op_name.overload_name != "") {
          error_str += "." + op_name.overload_name;
      }
      error_str += "\n";
  }
  ASSERT_EQ(danglingImpls.size(), 0) << error_str;
}

bool called_kernel_cpu = false;
bool called_kernel_autograd = false;
bool called_kernel_tracing = false;

void cpu_kernel(Series) {
  called_kernel_cpu = true;
}

// autograd kernel that redispatches. Explicitly takes in and updates the DispatchKeySet
void autograd_kernel_redispatching_with_DispatchKeySet(MSharp::DispatchKeySet ks, Series a) {
  called_kernel_autograd = true;
  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  auto updatedDispatchKeySet = ks & MSharp::DispatchKeySet(MSharp::DispatchKeySet::FULL_AFTER, MSharp::DispatchKey::AutogradOther);
  callOpUnboxedWithPrecomputedDispatchKeySet<void, Series>(*op, updatedDispatchKeySet, a);
}

// autograd kernel that redispatches. Does not take in a DispatchKeySet
void autograd_kernel_redispatching_without_DispatchKeySet(MSharp::DispatchKeySet ks, Series a) {
  called_kernel_autograd = true;
  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  auto updatedDispatchKeySet = ks & MSharp::DispatchKeySet(MSharp::DispatchKeySet::FULL_AFTER, MSharp::DispatchKey::AutogradOther);
  callOpUnboxedWithPrecomputedDispatchKeySet<void, Series>(*op, updatedDispatchKeySet, a);
}

// tracing kernel that redispatches. Explicitly takes in and updates the DispatchKeySet
void tracing_kernel_redispatching_with_DispatchKeySet(MSharp::DispatchKeySet ks, Series a) {
  called_kernel_tracing = true;
  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  auto updatedDispatchKeySet = ks & MSharp::DispatchKeySet(MSharp::DispatchKeySet::FULL_AFTER, MSharp::DispatchKey::Tracer);
  callOpUnboxedWithPrecomputedDispatchKeySet<void, Series>(*op, updatedDispatchKeySet, a);
}

TEST(OperatorRegistrationTest, callKernelsWithDispatchKeySetConvention_call_redispatchesToLowerPriorityKernels) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series dummy) -> ()");
  m.impl("fn", MSharp::DispatchKey::CPU, cpu_kernel);
  m.impl("fn", MSharp::DispatchKey::AutogradCPU, autograd_kernel_redispatching_with_DispatchKeySet);
  m.impl("fn", MSharp::DispatchKey::Tracer, tracing_kernel_redispatching_with_DispatchKeySet);

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  called_kernel_cpu = called_kernel_autograd = called_kernel_tracing = false;
  auto tracing_autograd_cpu_set = MSharp::DispatchKeySet()
                                    .add(MSharp::DispatchKey::Tracer)
                                    .add(MSharp::DispatchKey::AutogradCPU)
                                    .add(MSharp::DispatchKey::CPU);

  // call Tracing -> call Autograd -> call CPU
  callOpUnboxed<void, Series>(*op, dummyTensor(tracing_autograd_cpu_set, true));
  EXPECT_TRUE(called_kernel_tracing);
  EXPECT_TRUE(called_kernel_autograd);
  EXPECT_TRUE(called_kernel_cpu);
}

TEST(OperatorRegistrationTest, callKernelsWithDispatchKeySetConvention_callBoxed_redispatchesToLowerPriorityKernels) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series dummy) -> ()");
  m.impl("fn", MSharp::DispatchKey::CPU, cpu_kernel);
  m.impl("fn", MSharp::DispatchKey::AutogradCPU, autograd_kernel_redispatching_with_DispatchKeySet);
  m.impl("fn", MSharp::DispatchKey::Tracer, tracing_kernel_redispatching_with_DispatchKeySet);

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  called_kernel_cpu = called_kernel_autograd = called_kernel_tracing = false;
  auto tracing_autograd_cpu_set = MSharp::DispatchKeySet()
                                    .add(MSharp::DispatchKey::Tracer)
                                    .add(MSharp::DispatchKey::AutogradCPU)
                                    .add(MSharp::DispatchKey::CPU);

  // call Tracing -> call Autograd -> call CPU
  callOp<Series>(*op, dummyTensor(tracing_autograd_cpu_set, true));
  EXPECT_TRUE(called_kernel_tracing);
  EXPECT_TRUE(called_kernel_autograd);
  EXPECT_TRUE(called_kernel_cpu);
}

TEST(OperatorRegistrationTest, callKernelsWithDispatchKeySetConvention_mixedCallingConventions_redispatchesToLowerPriorityKernels) {
  auto m = MAKE_TORCH_LIBRARY(test);
  m.def("fn(Series dummy) -> ()");
  m.impl("fn", MSharp::DispatchKey::CPU, cpu_kernel);
  // the tracing kernel takes in a DispatchKeySet, but the autograd kernel does not
  // the dispatcher should handle correctly plumbing its DispatchKeySet to tracing and not autograd.
  m.impl("fn", MSharp::DispatchKey::AutogradCPU, autograd_kernel_redispatching_without_DispatchKeySet);
  m.impl("fn", MSharp::DispatchKey::Tracer, tracing_kernel_redispatching_with_DispatchKeySet);

  auto op = Dispatcher::singleton().findSchema({"test::fn", ""});
  ASSERT_TRUE(op.has_value());

  called_kernel_cpu = called_kernel_autograd = called_kernel_tracing = false;
  auto tracing_autograd_cpu_set = MSharp::DispatchKeySet()
                                    .add(MSharp::DispatchKey::Tracer)
                                    .add(MSharp::DispatchKey::AutogradCPU)
                                    .add(MSharp::DispatchKey::CPU);

  // call Tracing -> call Autograd -> call CPU
  callOpUnboxed<void, Series>(*op, dummyTensor(tracing_autograd_cpu_set, true));
  EXPECT_TRUE(called_kernel_tracing);
  EXPECT_TRUE(called_kernel_autograd);
  EXPECT_TRUE(called_kernel_cpu);
}

TEST(OperatorRegistrationTest, getRegistrationsForDispatchKey) {
  // should return every registered op
  auto all_ops = Dispatcher::singleton().getRegistrationsForDispatchKey(std::nullopt);
  // should return every registered op with a cpu kernel
  auto cpu_ops = Dispatcher::singleton().getRegistrationsForDispatchKey(MSharp::DispatchKey::CPU);
  ASSERT_TRUE(all_ops.size() > 0);
  ASSERT_TRUE(cpu_ops.size() > 0);

  auto cmp_lambda = [](const MSharp::OperatorName a, const MSharp::OperatorName& b) -> bool {
      return MSharp::toString(a) < MSharp::toString(b);
  };

  std::sort(all_ops.begin(), all_ops.end(), cmp_lambda);
  std::sort(cpu_ops.begin(), cpu_ops.end(), cmp_lambda);
  ASSERT_TRUE(std::includes(all_ops.begin(), all_ops.end(), cpu_ops.begin(), cpu_ops.end(), cmp_lambda));
}

Series symint_op(const Series& self, int64_t length) {
  return self.clone();
}

TEST(OperatorRegistrationTest, TestSymNonSymCompatibility) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("_test::symint_op(Series self, SymInt length) -> Series");
  auto m_cpu = MAKE_TORCH_LIBRARY_IMPL(_test, CPU);
  m_cpu.impl("symint_op", MSharp::DispatchKey::CPU, TORCH_FN(symint_op));

  auto opHandle = MSharp::Dispatcher::singleton().findSchemaOrThrow(
      "_test::symint_op", "");

  opHandle.typed<Series(const Series&, int64_t)>().call(dummyTensor(MSharp::DispatchKey::CPU), 4);
  opHandle.typed<Series(const Series&, MSharp::SymInt)>().call(dummyTensor(MSharp::DispatchKey::CPU), MSharp::SymInt(4));

  expectThrows<MSharp::Error>([&] {
    opHandle.typed<Series(const Series&, const MSharp::SymInt&)>().call(dummyTensor(MSharp::DispatchKey::CPU), MSharp::SymInt(4));
  }, "Tried to access or call an operator with a wrong signature");
}

Series symint_op2(const Series& self, MSharp::SymInt length) {
  return self.clone();
}

TEST(OperatorRegistrationTest, TestSymSymCompatibility) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("_test::symint_op(Series self, SymInt length) -> Series");
  auto m_cpu = MAKE_TORCH_LIBRARY_IMPL(_test, CPU);
  m_cpu.impl("symint_op", MSharp::DispatchKey::CPU, TORCH_FN(symint_op2));

  auto opHandle = MSharp::Dispatcher::singleton().findSchemaOrThrow(
      "_test::symint_op", "");

  opHandle.typed<Series(const Series&, int64_t)>().call(dummyTensor(MSharp::DispatchKey::CPU), 4);
  opHandle.typed<Series(const Series&, MSharp::SymInt)>().call(dummyTensor(MSharp::DispatchKey::CPU), MSharp::SymInt(4));
  // TODO: We should reject this on principle, but today it accidentally works
  // due to going through the boxed calling convention.
  //
  // First, we attempt to test if const SymInt& has SymInt. It does not,
  // because we only accept something as SymInt if it has exactly SymInt in
  // its signature. So we check if there is a non-symint kernel. But there is
  // no non-SymInt kernel, because we only registered a real SymInt kernel.
  // When this occurs, we fall back to the boxed calling convention.  And the
  // boxed calling convention can deal with const SymInt& fine, as during
  // boxing it will just create a SymInt to push onto the argument stack and
  // everything is fine.
  opHandle.typed<Series(const Series&, const MSharp::SymInt&)>().call(dummyTensor(MSharp::DispatchKey::CPU), MSharp::SymInt(4));
}

Series symint_op3(const Series& self, const MSharp::SymInt& length) {
  return self.clone();
}

TEST(OperatorRegistrationTest, TestSymSymRefCompatibility) {
  auto m = MAKE_TORCH_LIBRARY(_test);
  m.def("_test::symint_op(Series self, SymInt length) -> Series");
  auto m_cpu = MAKE_TORCH_LIBRARY_IMPL(_test, CPU);

  expectThrows<MSharp::Error>([&] {
    m_cpu.impl("symint_op", MSharp::DispatchKey::CPU, TORCH_FN(symint_op3));
  }, "doesn't match the expected function schema");
}

}

#pragma GCC diagnostic pop
