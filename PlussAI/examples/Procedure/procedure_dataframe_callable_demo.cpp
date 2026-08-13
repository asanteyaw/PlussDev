#include <pluss/ai/procedure.h>
#include <pluss/mic.h>
#include <pluss/pluss.h>

#include <iostream>
#include <vector>

int main() {
  namespace ai = pluss::ai;
  using pluss::table::DataFrame;
  using pluss::table::Thread;

  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame frame({
      {"x", Thread(pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3}, cfg))},
      {"y", Thread(pluss::make_series(std::vector<double>{4.0, 5.0, 6.0}, {3}, cfg))},
  });

  const ai::Procedure proc = ai::procedure::make({
      ai::procedure::step("frame_pass", "raw", "clean", [](const DataFrame& input) {
        return input;
      }),
  });

  const ai::ProcedureResult out = proc.run({ai::procedure::input("raw", frame)});
  std::cout << "rows=" << out.get<DataFrame>("clean").ncases() << "\n";
  std::cout << "columns=" << out.get<DataFrame>("clean").nfeatures() << "\n";
  return 0;
}
