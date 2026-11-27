// The PlussII notebook engine: clang's incremental interpreter, like clang-repl, with one difference that matters in a
// notebook: an error a cell throws and does not catch is reported and the engine carries on, so the variables and
// functions of earlier cells stay, as they do in other notebooks. clang-repl ends instead, and everything is lost.
//
// It reads what the kernel sends one line at a time, as clang-repl does: "%lib <path>" loads a shared library and
// anything else is compiled and run. Compiler arguments come after the program's name; "-Xcc" before one is accepted and
// dropped, so the kernel passes the same arguments it would give clang-repl. install_kernel.py builds it against the
// LLVM that provides clang-repl, with PLUSS_RESOURCE_DIR set to that clang's resource folder (its built-in headers).
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Interpreter/Interpreter.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <exception>
#include <iostream>
#include <string>
#include <vector>

#ifndef PLUSS_RESOURCE_DIR
#define PLUSS_RESOURCE_DIR ""
#endif

namespace {

// What the kernel reads as "the cell threw this": the same markers the prelude's last-resort handler writes.
void report_thrown(const std::string& what) {
  std::cerr << "\x1e<<pluss-uncaught>>" << what << "\x1e<<pluss-uncaught-end>>" << std::endl;
}

bool report_error(llvm::Error error) {
  if (!error) return false;
  llvm::logAllUnhandledErrors(std::move(error), llvm::errs(), "error: ");
  llvm::errs().flush();
  return true;
}

}  // namespace

int main(int argc, const char** argv) {
  llvm::InitializeNativeTarget();
  llvm::InitializeNativeTargetAsmPrinter();

  std::vector<std::string> owned;
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]) != "-Xcc") owned.emplace_back(argv[i]);
  }
  bool has_resource_dir = false;
  for (const auto& arg : owned) has_resource_dir = has_resource_dir || arg.rfind("-resource-dir", 0) == 0;
  if (!has_resource_dir && *PLUSS_RESOURCE_DIR != '\0') {
    owned.emplace_back("-resource-dir");
    owned.emplace_back(PLUSS_RESOURCE_DIR);
  }
  std::vector<const char*> args;
  for (const auto& arg : owned) args.push_back(arg.c_str());

  clang::IncrementalCompilerBuilder builder;
  builder.SetCompilerArgs(args);
  auto compiler = builder.CreateCpp();
  if (!compiler) {
    report_error(compiler.takeError());
    return 1;
  }
  auto interpreter = clang::Interpreter::create(std::move(*compiler));
  if (!interpreter) {
    report_error(interpreter.takeError());
    return 1;
  }

  for (std::string line; std::getline(std::cin, line);) {
    if (line.empty()) continue;
    if (line == "%quit") break;
    try {
      if (line.rfind("%lib ", 0) == 0) {
        report_error((*interpreter)->LoadDynamicLibrary(line.substr(5).c_str()));
      } else {
        report_error((*interpreter)->ParseAndExecute(line));
      }
    } catch (const std::exception& e) {
      report_thrown(e.what());
    } catch (...) {
      report_thrown("an exception that is not a std::exception");
    }
    std::cout.flush();
    std::cerr.flush();
  }
  return 0;
}
