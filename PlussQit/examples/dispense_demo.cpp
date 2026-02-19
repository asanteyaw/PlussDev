#include <pluss/pluss.h>

#include <any>
#include <iostream>
#include <string>

struct Foo {
  int grok(std::string name, int age, int height) {
    std::cout << "name=" << name << ", age=" << age << ", height=" << height << "\n";
    return age + height;
  }
};

int main() {
  Foo foo;

  auto r = pluss::dispense(foo, &Foo::grok, pluss::args{"John", 29, 190});
  std::cout << "return: " << std::any_cast<int>(r) << "\n";

  auto r2 = pluss::dispense(foo, &Foo::grok, pluss::pack_args("Jane", 31, 165));
  std::cout << "return (pack_args): " << std::any_cast<int>(r2) << "\n";

  try {
    auto bad_count = pluss::args{"OnlyTwo", 99};
    (void)pluss::dispense(foo, &Foo::grok, bad_count);
  } catch (const std::exception& e) {
    std::cout << "caught expected error: " << e.what() << "\n";
  }

  return 0;
}
