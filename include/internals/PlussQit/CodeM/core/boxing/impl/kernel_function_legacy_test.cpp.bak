#include <gtest/gtest.h>

// This intentionally tests a deprecated API
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

#include <PlussQit/core/boxing/impl/test_helpers.h>
#include <PlussQit/core/op_registration/op_registration.h>
#include <PlussQit/core/Series.h>
#include <torch/csrc/jit/frontend/function_schema_parser.h>

#include <PlussQit/core/LegacyTypeDispatch.h>

/**
 * This file tests the legacy function-based API for registering kernels.
 *
 * > namespace { Series kernel(Series a) {...} }
 * > static auto registry = MSharp::RegisterOperators()
 * >   .op("func(Series a) -> Series", &kernel);
 */

using MSharp::RegisterOperators;
using MSharp::DispatchKey;
using MSharp::Stack;
using std::make_unique;
using MSharp::intrusive_ptr;
using MSharp::Dict;
using pluss::Series;
using std::string;
using std::unique_ptr;

namespace {

int64_t errorKernel(const Series& series, int64_t input) {
  EXPECT_TRUE(false); // this kernel should never be called
  return 0;
}

int64_t incrementKernel(const Series& series, int64_t input) {
  return input + 1;
}

void expectCallsIncrement(DispatchKey dispatch_key) {
  pluss::AutoDispatchBelowAutograd mode;

  // assert that schema and cpu kernel are present
  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::my_op", ""});
  ASSERT_TRUE(op.has_value());
  auto result = callOp(*op, dummyTensor(dispatch_key), 5);
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(6, result[0].toInt());
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernel_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::my_op(Series dummy, int input) -> int", &incrementKernel);
  expectCallsIncrement(DispatchKey::CPU);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernel_whenRegisteredInConstructor_thenCanBeCalled) {
  auto registrar = RegisterOperators("_test::my_op(Series dummy, int input) -> int", &incrementKernel);
  expectCallsIncrement(DispatchKey::CPU);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMultipleOperatorsAndKernels_whenRegisteredInOneRegistrar_thenCallsRightKernel) {
  auto registrar = RegisterOperators()
      .op("_test::my_op(Series dummy, int input) -> int", &incrementKernel)
      .op("_test::error(Series dummy, int input) -> int", &errorKernel);
  expectCallsIncrement(DispatchKey::CPU);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMultipleOperatorsAndKernels_whenRegisteredInMultipleRegistrars_thenCallsRightKernel) {
  auto registrar1 = RegisterOperators().op("_test::my_op(Series dummy, int input) -> int", &incrementKernel);
  auto registrar2 = RegisterOperators().op("_test::error(Series dummy, int input) -> int", &errorKernel);
  expectCallsIncrement(DispatchKey::CPU);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernel_whenRegistrationRunsOutOfScope_thenCannotBeCalledAnymore) {
  {
    auto registrar = RegisterOperators().op("_test::my_op(Series dummy, int input) -> int", &incrementKernel);

    expectCallsIncrement(DispatchKey::CPU);
  }

  // now the registrar is destructed. Assert that the schema is gone.
  expectDoesntFindOperator("_test::my_op");
}

bool was_called = false;

void kernelWithoutOutput(const Series&) {
  was_called = true;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::no_return(Series dummy) -> ()", &kernelWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::no_return", ""});
  ASSERT_TRUE(op.has_value());
  was_called = false;
  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_TRUE(was_called);
  EXPECT_EQ(0, result.size());
}

std::tuple<> kernelWithZeroOutputs(const Series&) {
  was_called = true;
  return std::make_tuple();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithZeroOutputs_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::zero_outputs(Series dummy) -> ()", &kernelWithZeroOutputs);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::zero_outputs", ""});
  ASSERT_TRUE(op.has_value());
  was_called = false;
  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_TRUE(was_called);
  EXPECT_EQ(0, result.size());
}

int64_t kernelWithIntOutput(Series, int64_t a, int64_t b) {
  return a + b;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::int_output(Series dummy, int a, int b) -> int", &kernelWithIntOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::int_output", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU), 3, 6);
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(9, result[0].toInt());
}

Series kernelWithTensorOutput(const Series& input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::returning_tensor(Series input) -> Series", &kernelWithTensorOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::returning_tensor", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[0].toTensor()));

  result = callOp(*op, dummyTensor(DispatchKey::CUDA));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[0].toTensor()));
}

std::vector<Series> kernelWithTensorListOutput(const Series& input1, const Series& input2, const Series& input3) {
  return {input1, input2, input3};
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorListOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::list_output(Series input1, Series input2, Series input3) -> Series[]", &kernelWithTensorListOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::list_output", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CUDA), dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(3, result[0].toTensorVector().size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[0].toTensorVector()[0]));
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[0].toTensorVector()[1]));
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[0].toTensorVector()[2]));
}

std::vector<int64_t> kernelWithIntListOutput(const Series&, int64_t input1, int64_t input2, int64_t input3) {
  return {input1, input2, input3};
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntListOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::list_output(Series dummy, int input1, int input2, int input3) -> int[]", &kernelWithIntListOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::list_output", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU), 2, 4, 6);
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(3, result[0].toIntVector().size());
  EXPECT_EQ(2, result[0].toIntVector()[0]);
  EXPECT_EQ(4, result[0].toIntVector()[1]);
  EXPECT_EQ(6, result[0].toIntVector()[2]);
}

std::tuple<Series, int64_t, std::vector<Series>, std::optional<int64_t>, Dict<string, Series>> kernelWithMultipleOutputs(Series) {
  Dict<string, Series> dict;
  dict.insert("first", dummyTensor(DispatchKey::CPU));
  dict.insert("second", dummyTensor(DispatchKey::CUDA));
  return std::tuple<Series, int64_t, std::vector<Series>, std::optional<int64_t>, Dict<string, Series>>(
    dummyTensor(DispatchKey::CUDA),
    5,
    {dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CUDA)},
    std::optional<int64_t>(std::in_place, 0),
    dict
  );
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithMultipleOutputs_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
     .op("_test::multiple_outputs(Series dummy) -> (Series, int, Series[], int?, Dict(str, Series))", &kernelWithMultipleOutputs);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::multiple_outputs", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(5, result.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[0].toTensor()));
  EXPECT_EQ(5, result[1].toInt());
  EXPECT_EQ(2, result[2].toTensorVector().size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[2].toTensorVector()[0]));
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[2].toTensorVector()[1]));
  EXPECT_EQ(0, result[3].toInt());
  auto result_dict = MSharp::impl::toTypedDict<string, Series>(result[4].toGenericDict());
  EXPECT_EQ(2, result_dict.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result_dict.at("first")));
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result_dict.at("second")));
}

Series kernelWithTensorInputByReferenceWithOutput(const Series& input1) {
  return input1;
}

Series kernelWithTensorInputByValueWithOutput(Series input1) {
  return input1;
}
TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorInputByReference_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_input(Series input) -> Series", &kernelWithTensorInputByReferenceWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_input", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[0].toTensor()));

  result = callOp(*op, dummyTensor(DispatchKey::CUDA));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[0].toTensor()));
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorInputByValue_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_input(Series input) -> Series", &kernelWithTensorInputByValueWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_input", ""});
  ASSERT_TRUE(op.has_value());

  auto result = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(result[0].toTensor()));

  result = callOp(*op, dummyTensor(DispatchKey::CUDA));
  EXPECT_EQ(1, result.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(result[0].toTensor()));
}

Series captured_input;

void kernelWithTensorInputByReferenceWithoutOutput(const Series& input1) {
  captured_input = input1;
}

void kernelWithTensorInputByValueWithoutOutput(Series input1) {
  captured_input = input1;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorInputByReference_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_input(Series input) -> ()", &kernelWithTensorInputByReferenceWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(captured_input));

  outputs = callOp(*op, dummyTensor(DispatchKey::CUDA));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(captured_input));
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorInputByValue_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_input(Series input) -> ()", &kernelWithTensorInputByValueWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(DispatchKey::CPU, extractDispatchKey(captured_input));

  outputs = callOp(*op, dummyTensor(DispatchKey::CUDA));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(captured_input));
}

int64_t captured_int_input = 0;

void kernelWithIntInputWithoutOutput(Series, int64_t input1) {
  captured_int_input = input1;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::int_input(Series dummy, int input) -> ()", &kernelWithIntInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::int_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_int_input = 0;
  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), 3);
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(3, captured_int_input);
}

int64_t kernelWithIntInputWithOutput(Series, int64_t input1) {
  return input1 + 1;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::int_input(Series dummy, int input) -> int", &kernelWithIntInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::int_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), 3);
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(4, outputs[0].toInt());
}

int64_t captured_input_list_size = 0;

void kernelWithIntListInputWithoutOutput(Series, const std::vector<int64_t>& input1) {
  captured_input_list_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntListInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::int_list_input(Series dummy, int[] input) -> ()", &kernelWithIntListInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::int_list_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_input_list_size = 0;
  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), MSharp::List<int64_t>({2, 4, 6}));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(3, captured_input_list_size);
}

int64_t kernelWithIntListInputWithOutput(Series, const std::vector<int64_t>& input1) {
  return input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithIntListInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::int_list_input(Series dummy, int[] input) -> int", &kernelWithIntListInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::int_list_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), MSharp::List<int64_t>({2, 4, 6}));
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(3, outputs[0].toInt());
}

void kernelWithTensorListInputWithoutOutput(const std::vector<Series>& input1) {
  captured_input_list_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorListInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> ()", &kernelWithTensorListInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_input_list_size = 0;
  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(2, captured_input_list_size);
}

int64_t kernelWithTensorListInputWithOutput(const std::vector<Series>& input1) {
  return input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithTensorListInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> int", &kernelWithTensorListInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(2, outputs[0].toInt());
}

void kernelWithLegacyTensorVectorInputWithoutOutput(const std::vector<Series>& input1) {
  captured_input_list_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithLegacyTensorVectorInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> ()", &kernelWithLegacyTensorVectorInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_input_list_size = 0;
  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(2, captured_input_list_size);
}

int64_t kernelWithLegacyTensorVectorInputWithOutput(const std::vector<Series>& input1) {
  return input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithLegacyTensorVectorInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> int", &kernelWithLegacyTensorVectorInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(2, outputs[0].toInt());
}

void kernelWithLegacyTensorListInputWithoutOutput(std::vector<Series> input1) {
  captured_input_list_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithLegacyTensorListInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> ()", &kernelWithLegacyTensorListInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_input_list_size = 0;
  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(2, captured_input_list_size);
}

int64_t kernelWithLegacyTensorListInputWithOutput(std::vector<Series> input1) {
  return input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithLegacyTensorListInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::tensor_list_input(Series[] input) -> int", &kernelWithLegacyTensorListInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::tensor_list_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, MSharp::List<Series>({dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CPU)}));
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(2, outputs[0].toInt());
}

std::vector<std::string> kernelWithStringListOutput(std::vector<std::string> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithStringListOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::stringlist_output(str[] input) -> str[]", &kernelWithStringListOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::stringlist_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::List<std::string> list({"value1", "value2"});
  auto outputs = callOp(*op, list);
  EXPECT_EQ(1, outputs.size());
  auto output = std::move(outputs[0]).toList();

  EXPECT_EQ(2, output.size());
  EXPECT_EQ("value1", output.get(0).toStringRef());
  EXPECT_EQ("value2", output.get(1).toStringRef());
}

int captured_dict_size = 0;

void kernelWithDictInputWithoutOutput(Dict<string, Series> input1) {
  captured_dict_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithDictInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_input(Dict(str, Series) input) -> ()", &kernelWithDictInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_dict_size = 0;
  Dict<string, Series> dict;
  dict.insert("key1", dummyTensor(DispatchKey::CPU));
  dict.insert("key2", dummyTensor(DispatchKey::CUDA));
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(2, captured_dict_size);
}

string kernelWithDictInputWithOutput(Dict<string, string> input1) {
  return input1.at("key2");
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithDictInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_input(Dict(str, str) input) -> str", &kernelWithDictInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_input", ""});
  ASSERT_TRUE(op.has_value());

  Dict<string, string> dict;
  dict.insert("key1", "value1");
  dict.insert("key2", "value2");
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ("value2", outputs[0].toStringRef());
}

Dict<string, string> kernelWithDictOutput(Dict<string, string> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithDictOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_output(Dict(str, str) input) -> Dict(str, str)", &kernelWithDictOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_output", ""});
  ASSERT_TRUE(op.has_value());

  Dict<string, string> dict;
  dict.insert("key1", "value1");
  dict.insert("key2", "value2");
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  auto output = MSharp::impl::toTypedDict<string, string>(outputs[0].toGenericDict());

  EXPECT_EQ(2, output.size());
  EXPECT_EQ("value1", output.at("key1"));
  EXPECT_EQ("value2", output.at("key2"));
}

void kernelWithUnorderedMapInputWithoutOutput(std::unordered_map<string, Series> input1) {
  captured_dict_size = input1.size();
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithUnorderedMapInput_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_input(Dict(str, Series) input) -> ()", &kernelWithUnorderedMapInputWithoutOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_input", ""});
  ASSERT_TRUE(op.has_value());

  captured_dict_size = 0;
  MSharp::Dict<string, Series> dict;
  dict.insert("key1", dummyTensor(DispatchKey::CPU));
  dict.insert("key2", dummyTensor(DispatchKey::CUDA));
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(0, outputs.size());
  EXPECT_EQ(2, captured_dict_size);
}

string kernelWithUnorderedMapInputWithOutput(std::unordered_map<string, string> input1) {
  return input1.at("key2");
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithUnorderedMapInput_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_input(Dict(str, str) input) -> str", &kernelWithUnorderedMapInputWithOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_input", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, string> dict;
  dict.insert("key1", "value1");
  dict.insert("key2", "value2");
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ("value2", outputs[0].toStringRef());
}

std::unordered_map<string, string> kernelWithUnorderedMapOutput(std::unordered_map<string, string> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithUnorderedMapOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_output(Dict(str, str) input) -> Dict(str, str)", &kernelWithUnorderedMapOutput);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, string> dict;
  dict.insert("key1", "value1");
  dict.insert("key2", "value2");
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  auto output = MSharp::impl::toTypedDict<string, string>(outputs[0].toGenericDict());

  EXPECT_EQ(2, output.size());
  EXPECT_EQ("value1", output.at("key1"));
  EXPECT_EQ("value2", output.at("key2"));
}

std::unordered_map<string, std::vector<int64_t>> kernelWithMapOfIntList(std::unordered_map<string, std::vector<int64_t>> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithMapOfList_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_output(Dict(str, int[]) input) -> Dict(str, int[])", &kernelWithMapOfIntList);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, MSharp::List<int64_t>> dict;
  dict.insert("key1", MSharp::List<int64_t>({10, 20}));
  dict.insert("key2", MSharp::List<int64_t>({30, 40}));
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  auto output = MSharp::impl::toTypedDict<string, MSharp::List<int64_t>>(outputs[0].toGenericDict());

  EXPECT_EQ(2, output.size());
  EXPECT_EQ(2, output.at("key1").size());
  EXPECT_EQ(10, output.at("key1").get(0));
  EXPECT_EQ(20, output.at("key1").get(1));
  EXPECT_EQ(2, output.at("key2").size());
  EXPECT_EQ(30, output.at("key2").get(0));
  EXPECT_EQ(40, output.at("key2").get(1));
}

std::unordered_map<string, std::vector<std::unordered_map<int64_t, string>>> kernelWithMapOfListOfMap(std::unordered_map<string, std::vector<std::unordered_map<int64_t, string>>> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithMapOfListOfMap_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::dict_output(Dict(str, Dict(int,str)[]) input) -> Dict(str, Dict(int,str)[])", &kernelWithMapOfListOfMap);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::dict_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, MSharp::List<MSharp::Dict<int64_t, string>>> dict;
  MSharp::Dict<int64_t, string> dict1;
  dict1.insert(10, "10");
  dict1.insert(20, "20");
  dict.insert("key1", MSharp::List<MSharp::Dict<int64_t, string>>({dict1}));
  MSharp::Dict<int64_t, string> dict2;
  dict2.insert(30, "30");
  dict2.insert(40, "40");
  dict.insert("key2", MSharp::List<MSharp::Dict<int64_t, string>>({dict2}));
  auto outputs = callOp(*op, dict);
  EXPECT_EQ(1, outputs.size());
  auto output = MSharp::impl::toTypedDict<string, MSharp::List<MSharp::Dict<int64_t, string>>>(outputs[0].toGenericDict());

  EXPECT_EQ(2, output.size());
  EXPECT_EQ(1, output.at("key1").size());
  EXPECT_EQ(2, output.at("key1").get(0).size());
  EXPECT_EQ("10", output.at("key1").get(0).at(10));
  EXPECT_EQ("20", output.at("key1").get(0).at(20));
  EXPECT_EQ(2, output.at("key2").get(0).size());
  EXPECT_EQ("30", output.at("key2").get(0).at(30));
  EXPECT_EQ("40", output.at("key2").get(0).at(40));
}

std::vector<std::unordered_map<string, int64_t>> kernelWithListOfMap(std::vector<std::unordered_map<string, int64_t>> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithListOfMap_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::list_output(Dict(str, int)[] input) -> Dict(str, int)[]", &kernelWithListOfMap);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::list_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, int64_t> dict1;
  dict1.insert("1", 1);
  dict1.insert("2", 2);
  MSharp::Dict<string, int64_t> dict2;
  dict2.insert("3", 3);
  dict2.insert("4", 4);
  MSharp::List<MSharp::Dict<string, int64_t>> list({dict1, dict2});
  auto outputs = callOp(*op, list);
  EXPECT_EQ(1, outputs.size());
  MSharp::impl::GenericList output = std::move(outputs[0]).toList();

  EXPECT_EQ(2, output.size());
  EXPECT_EQ(2, output.get(0).toGenericDict().size());
  EXPECT_EQ(1, output.get(0).toGenericDict().at("1").toInt());
  EXPECT_EQ(2, output.get(0).toGenericDict().at("2").toInt());
  EXPECT_EQ(2, output.get(1).toGenericDict().size());
  EXPECT_EQ(3, output.get(1).toGenericDict().at("3").toInt());
  EXPECT_EQ(4, output.get(1).toGenericDict().at("4").toInt());
}

std::vector<std::unordered_map<string, std::vector<int64_t>>> kernelWithListOfMapOfIntList(std::vector<std::unordered_map<string, std::vector<int64_t>>> input) {
  return input;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithListOfMapOfIntList_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators()
      .op("_test::list_output(Dict(str, int[])[] input) -> Dict(str, int[])[]", &kernelWithListOfMapOfIntList);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::list_output", ""});
  ASSERT_TRUE(op.has_value());

  MSharp::Dict<string, MSharp::List<int64_t>> dict1;
  dict1.insert("1", MSharp::List<int64_t>({1, 2}));
  dict1.insert("3", MSharp::List<int64_t>({3, 4}));
  MSharp::Dict<string, MSharp::List<int64_t>> dict2;
  dict2.insert("5", MSharp::List<int64_t>({5, 6}));
  dict2.insert("7", MSharp::List<int64_t>({7, 8}));
  MSharp::List<MSharp::Dict<string, MSharp::List<int64_t>>> list({ dict1, dict2 });
  auto outputs = callOp(*op, list);
  EXPECT_EQ(1, outputs.size());
  MSharp::impl::GenericList output = std::move(outputs[0]).toList();

  EXPECT_EQ(2, output.size());
  EXPECT_EQ(2, output.get(0).toGenericDict().size());
  EXPECT_EQ(2, output.get(0).toGenericDict().at("1").toIntVector().size());
  EXPECT_EQ(1, output.get(0).toGenericDict().at("1").toIntVector()[0]);
  EXPECT_EQ(2, output.get(0).toGenericDict().at("1").toIntVector()[1]);
  EXPECT_EQ(2, output.get(0).toGenericDict().at("3").toIntVector().size());
  EXPECT_EQ(3, output.get(0).toGenericDict().at("3").toIntVector()[0]);
  EXPECT_EQ(4, output.get(0).toGenericDict().at("3").toIntVector()[1]);
  EXPECT_EQ(2, output.get(1).toGenericDict().at("5").toIntVector().size());
  EXPECT_EQ(5, output.get(1).toGenericDict().at("5").toIntVector()[0]);
  EXPECT_EQ(6, output.get(1).toGenericDict().at("5").toIntVector()[1]);
  EXPECT_EQ(2, output.get(1).toGenericDict().at("7").toIntVector().size());
  EXPECT_EQ(7, output.get(1).toGenericDict().at("7").toIntVector()[0]);
  EXPECT_EQ(8, output.get(1).toGenericDict().at("7").toIntVector()[1]);
}

bool called = false;

void kernelWithoutInputs() {
  called = true;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenFallbackKernelWithoutAnyArguments_whenRegistered_thenCanBeCalled) {
  // note: non-fallback kernels without series arguments don't work because there
  // is no way to get the dispatch key. For operators that only have a fallback
  // kernel, this must work for backwards compatibility.
  auto registrar = RegisterOperators()
      .op("_test::no_tensor_args() -> ()", &kernelWithoutInputs);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::no_tensor_args", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto outputs = callOp(*op);
  EXPECT_TRUE(called);
}

int64_t kernelWithoutTensorInputs(int64_t arg) {
  return arg + 1;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenFallbackKernelWithoutTensorArguments_whenRegistered_thenCanBeCalled) {
  // note: non-fallback kernels without series arguments don't work because there
  // is no way to get the dispatch key. For operators that only have a fallback
  // kernel, this must work for backwards compatibility.
  auto registrar = RegisterOperators()
      .op("_test::no_tensor_args(int arg) -> int", &kernelWithoutTensorInputs);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::no_tensor_args", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, 3);
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(4, outputs[0].toInt());
}

std::optional<Series> called_arg2 = std::nullopt;
std::optional<int64_t> called_arg3 = std::nullopt;
std::optional<std::string> called_arg4 = std::nullopt;

void kernelWithOptInputWithoutOutput(Series arg1, const std::optional<Series>& arg2, std::optional<int64_t> arg3, std::optional<std::string> arg4) {
  called = true;
  called_arg2 = arg2;
  called_arg3 = arg3;
  called_arg4 = arg4;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithOptionalInputs_withoutOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::opt_input(Series arg1, Series? arg2, int? arg3, str? arg4) -> ()", &kernelWithOptInputWithoutOutput);
  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::opt_input", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CUDA), MSharp::IValue(), std::string("text"));
  EXPECT_EQ(0, outputs.size());

  EXPECT_TRUE(called);
  EXPECT_TRUE(called_arg2.has_value());
  EXPECT_EQ(extractDispatchKey(*called_arg2), DispatchKey::CUDA);
  EXPECT_FALSE(called_arg3.has_value());
  EXPECT_TRUE(called_arg4.has_value());
  EXPECT_EQ(*called_arg4, "text");

  called = false;
  outputs = callOp(*op, dummyTensor(DispatchKey::CPU), MSharp::IValue(), 4, MSharp::IValue());
  EXPECT_EQ(0, outputs.size());

  EXPECT_TRUE(called);
  EXPECT_FALSE(called_arg2.has_value());
  EXPECT_TRUE(called_arg3.has_value());
  EXPECT_EQ(*called_arg3, 4);
  EXPECT_FALSE(called_arg4.has_value());
}

std::optional<Series> kernelWithOptInputWithOutput(Series arg1, const std::optional<Series>& arg2, std::optional<int64_t> arg3, std::optional<std::string> arg4) {
  called = true;
  called_arg2 = arg2;
  called_arg3 = arg3;
  called_arg4 = arg4;
  return arg2;
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithOptionalInputs_withOutput_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::opt_input(Series arg1, Series? arg2, int? arg3, str? arg4) -> Series?", &kernelWithOptInputWithOutput);
  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::opt_input", ""});
  ASSERT_TRUE(op.has_value());

  called = false;
  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CUDA), MSharp::IValue(), std::string("text"));
  EXPECT_EQ(1, outputs.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(outputs[0].toTensor()));

  EXPECT_TRUE(called);
  EXPECT_TRUE(called_arg2.has_value());
  EXPECT_EQ(extractDispatchKey(*called_arg2), DispatchKey::CUDA);
  EXPECT_FALSE(called_arg3.has_value());
  EXPECT_TRUE(called_arg4.has_value());
  EXPECT_EQ(*called_arg4, "text");

  called = false;
  outputs = callOp(*op, dummyTensor(DispatchKey::CPU), MSharp::IValue(), 4, MSharp::IValue());
  EXPECT_EQ(1, outputs.size());
  EXPECT_TRUE(outputs[0].isNone());

  EXPECT_TRUE(called);
  EXPECT_FALSE(called_arg2.has_value());
  EXPECT_TRUE(called_arg3.has_value());
  EXPECT_EQ(*called_arg3, 4);
  EXPECT_FALSE(called_arg4.has_value());
}

std::tuple<std::optional<Series>, std::optional<int64_t>, std::optional<std::string>>
kernelWithOptInputWithMultipleOutputs(Series arg1, const std::optional<Series>& arg2, std::optional<int64_t> arg3, std::optional<std::string> arg4) {
  return std::make_tuple(arg2, arg3, arg4);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernelWithOptionalInputs_withMultipleOutputs_whenRegistered_thenCanBeCalled) {
  auto registrar = RegisterOperators().op("_test::opt_input(Series arg1, Series? arg2, int? arg3, str? arg4) -> (Series?, int?, str?)", &kernelWithOptInputWithMultipleOutputs);
  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::opt_input", ""});
  ASSERT_TRUE(op.has_value());

  auto outputs = callOp(*op, dummyTensor(DispatchKey::CPU), dummyTensor(DispatchKey::CUDA), MSharp::IValue(), std::string("text"));
  EXPECT_EQ(3, outputs.size());
  EXPECT_EQ(DispatchKey::CUDA, extractDispatchKey(outputs[0].toTensor()));
  EXPECT_TRUE(outputs[1].isNone());
  EXPECT_EQ("text", outputs[2].toStringRef());

  outputs = callOp(*op, dummyTensor(DispatchKey::CPU), MSharp::IValue(), 4, MSharp::IValue());
  EXPECT_EQ(3, outputs.size());
  EXPECT_TRUE(outputs[0].isNone());
  EXPECT_EQ(4, outputs[1].toInt());
  EXPECT_TRUE(outputs[2].isNone());
}

std::string concatKernel(const Series& tensor1, std::string a, const std::string& b, int64_t c) {
  return a + b + std::to_string(c);
}

void expectCallsConcatUnboxed(DispatchKey dispatch_key) {
  pluss::AutoDispatchBelowAutograd mode;

  // assert that schema and cpu kernel are present
  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::my_op", ""});
  ASSERT_TRUE(op.has_value());
  std::string result = callOpUnboxed<std::string, const Series&, std::string, const std::string&, int64_t>(*op, dummyTensor(dispatch_key), "1", "2", 3);
  EXPECT_EQ("123", result);
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernel_whenRegistered_thenCanBeCalledUnboxed) {
  auto registrar = RegisterOperators().op("_test::my_op(Series dummy, str a, str b, int c) -> str", &concatKernel);
  expectCallsConcatUnboxed(DispatchKey::CPU);
}

std::tuple<int64_t, Series> kernelForSchemaInference(Series arg1, int64_t arg2, const std::vector<Series>& arg3) {
  return {};
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenKernel_whenRegisteredWithoutSpecifyingSchema_thenInfersSchema) {
  auto registrar = RegisterOperators()
      .op("_test::no_schema_specified", &kernelForSchemaInference);

  auto op = MSharp::Dispatcher::singleton().findSchema({"_test::no_schema_specified", ""});
  ASSERT_TRUE(op.has_value());

  std::optional<std::string> differences = MSharp::findSchemaDifferences(torch::jit::parseSchema("_test::no_schema_specified(Series arg1, int arg2, Series[] arg3) -> (int, Series)"), op->schema());
  EXPECT_FALSE(differences.has_value());
}

template<class Return, class... Args> struct kernel_func final {
  static Return func(Args...) { return {}; }
};
template<class... Args> struct kernel_func<void, Args...> final {
  static void func(Args...) {}
};

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMismatchedKernel_withDifferentNumArguments_whenRegistering_thenFails) {
  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> int", &kernel_func<int64_t, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg, Series arg2) -> int", &kernel_func<int64_t, Series>::func);
    }, "The number of arguments is different. 2 vs 1"
  );

  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg, Series arg2) -> ()", &kernel_func<void, Series, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch() -> ()", &kernel_func<void, Series, Series>::func);
    }, "The number of arguments is different. 0 vs 2"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> ()", &kernel_func<void, Series, Series>::func);
    }, "The number of arguments is different. 1 vs 2"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg, Series arg2, Series arg3) -> ()", &kernel_func<void, Series, Series>::func);
    }, "The number of arguments is different. 3 vs 2"
  );
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMismatchedKernel_withDifferentArgumentType_whenRegistering_thenFails) {
  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg1, int arg2) -> int", &kernel_func<int64_t, Series, int64_t>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg1, float arg2) -> int", &kernel_func<int64_t, Series, int64_t>::func);
    }, "Type mismatch in argument 2: float vs int"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(int arg1, int arg2) -> int", &kernel_func<int64_t, Series, int64_t>::func);
    }, "Type mismatch in argument 1: int vs Series"
  );
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMismatchedKernel_withDifferentNumReturns_whenRegistering_thenFails) {
  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> int", &kernel_func<int64_t, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> ()", &kernel_func<int64_t, Series>::func);
    }, "The number of returns is different. 0 vs 1"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> (int, int)", &kernel_func<int64_t, Series>::func);
    }, "The number of returns is different. 2 vs 1"
  );

  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> ()", &kernel_func<void, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> Series", &kernel_func<void, Series>::func);
    }, "The number of returns is different. 1 vs 0"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> (Series, Series)", &kernel_func<void, Series>::func);
    }, "The number of returns is different. 2 vs 0"
  );

  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> (Series, Series)", &kernel_func<std::tuple<Series, Series>, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> ()", &kernel_func<std::tuple<Series, Series>, Series>::func);
    }, "The number of returns is different. 0 vs 2"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> Series", &kernel_func<std::tuple<Series, Series>, Series>::func);
    }, "The number of returns is different. 1 vs 2"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> (Series, Series, Series)", &kernel_func<std::tuple<Series, Series>, Series>::func);
    }, "The number of returns is different. 3 vs 2"
  );
}

TEST(OperatorRegistrationTestLegacyFunctionBasedKernel, givenMismatchedKernel_withDifferentReturnTypes_whenRegistering_thenFails) {
  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> int", &kernel_func<int64_t, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> Series", &kernel_func<int64_t, Series>::func);
    }, "Type mismatch in return 1: Series vs int"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> float", &kernel_func<int64_t, Series>::func);
    }, "Type mismatch in return 1: float vs int"
  );

  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> Series", &kernel_func<Series, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> float", &kernel_func<Series, Series>::func);
    }, "Type mismatch in return 1: float vs Series"
  );

  // assert this does not fail because it matches
  RegisterOperators()
      .op("_test::mismatch(Series arg) -> (Series, int)", &kernel_func<std::tuple<Series, int64_t>, Series>::func);

  // and now a set of mismatching schemas
  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> (Series, float)", &kernel_func<std::tuple<Series, int64_t>, Series>::func);
    }, "Type mismatch in return 2: float vs int"
  );

  expectThrows<MSharp::Error>([] {
    RegisterOperators()
        .op("_test::mismatch(Series arg) -> (int, int)", &kernel_func<std::tuple<Series, int64_t>, Series>::func);
    }, "Type mismatch in return 1: int vs Series"
  );
}

}

#pragma GCC diagnostic pop
