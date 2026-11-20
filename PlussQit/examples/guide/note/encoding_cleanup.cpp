// Clean text scraped from the web: undo HTML and URL escaping, repair wrongly decoded characters, remove invisible
// characters and bring everything to one Unicode form so that equal-looking strings compare equal.
#include <pluss/pluss.h>

#include <iostream>

static void show(const char *label, const pluss::Note &n) { std::cout << label << ": " << pluss::note::print(n) << "\n"; }

int main() {
  auto scraped = pluss::make_note({"Fish &amp; Chips &#8212; &#163;5 &copy;", "caf%C3%A9%20au%20lait", "naïve​ word", "Café", "plain text"});
  show("scraped", scraped);

  // 1. Undo the escaping layers in order: URL percent-escapes, then HTML entities. html_unescape knows the five markup entities and
  //    numeric references (&#8212;); a named entity such as &copy; is left exactly as written, never dropped.
  auto decoded = scraped.url_decode().html_unescape();
  show("decoded", decoded);

  // 2. Remove zero-width characters: they are invisible, and they break equality, sorting and search.
  auto visible = decoded.strip_zero_width();
  show("without zero-width characters", visible);

  // 3. "Cafe" followed by a combining accent and "Café" with one precomposed character look the same but are different bytes.
  auto composed = visible.to_nfc();
  auto lengths_before = visible.lengths_chars();
  auto lengths_after = composed.lengths_chars();
  show("length in characters before NFC", lengths_before);
  show("length in characters after NFC ", lengths_after);

  // 4. Equality after normalisation: these two now match.
  auto a = pluss::make_note("Café").to_nfc();
  auto b = pluss::make_note("Café").to_nfc();
  const auto same = a.regex().match("Café");
  std::cout << "composed 'Cafe + accent' equals 'Café' after NFC: " << (same.bools()[0] ? "yes" : "no") << " (" << pluss::note::print(b.lengths_chars()) << ")\n";

  // 5. Round trip a value through a text-safe encoding, as when storing bytes in a text column.
  auto secret = pluss::make_note("report-2026-11-13");
  auto encoded = secret.base64_encode();
  show("base64", encoded);
  show("hex   ", secret.hex_encode());
  const auto back = encoded.base64_decode().host();
  std::cout << "decoded again: " << back.scalar() << "\n";
}
