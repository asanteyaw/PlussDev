#include <iostream>
#include <pluss/pluss.h>

int main() {
   // Lex sentence = "The quick brown fox jumps over the lazy dog";

   // // std::string::find(substring, position)
   // size_t found = sentence->find("fox", 10);
   // std::cout << "Found 'fox' at index: " << found << "\n";  // ✅ should be 16

   // // std::string::replace(pos, len, new_str)
   // sentence->replace(4, 5, "slow");  // replaces "quick" with "slow"
   // std::cout << "After replace: " << sentence << "\n";

   // // std::string::substr(pos, len)
   // Lex sub = sentence->substr(10, 9);
   // std::cout << "Substr(10, 9): " << sub << "\n";

   // // std::string::compare(pos1, len1, str2, pos2, len2)
   // Lex a = "abcdefghij";
   // Lex b = "cde";
   // int cmp = a->compare(2, 3, b->c_str(), 0, 3);
   // std::cout << "Compare result: " << cmp << "\n";  // should be 0 if equal

   // // std::string::insert(pos, count, char)
   // Lex noisy = "HellWorld";
   // noisy->insert(4, 1, 'o');  // insert 'o' at index 4
   // std::cout << "After insert: " << noisy << "\n";

   // // std::string::find_first_of
   // size_t vowel = sentence->find_first_of("aeiou");
   // std::cout << "First vowel at: " << vowel << "\n";
   return 0;
}