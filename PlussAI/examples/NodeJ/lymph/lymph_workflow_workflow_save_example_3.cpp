#include <pluss/pluss.h>
#include <pluss/ai/lymph.h>

#include <filesystem>
#include <iostream>

int main() {
  namespace lymph = pluss::ai::lymph;
  namespace workflow = lymph::workflow;

  // save_policy/load_policy are the standalone sidecar helpers Workflow::
  // save/load call internally -- useful directly when a Policy needs to
  // travel independently of any particular saved model bundle.
  workflow::Policy policy{};
  policy.drop({"customer_id"});
  policy.require({"income", "balance"});
  policy.reject_any_missing({"income"});

  const std::string path =
      (std::filesystem::temp_directory_path() / "lymph_workflow_save_example_3.policy_only").string();
  workflow::save_policy(path, policy);

  const workflow::Policy reloaded = workflow::load_policy(path);
  std::cout << "drop_columns match:    " << std::boolalpha
            << (reloaded.drop_columns == policy.drop_columns) << "\n";
  std::cout << "require_columns match: "
            << (reloaded.require_columns == policy.require_columns) << "\n";
  std::cout << "filter_nulls match:    " << (reloaded.filter_nulls == policy.filter_nulls) << "\n";

  std::filesystem::remove(path + ".policy");

  return 0;
}
