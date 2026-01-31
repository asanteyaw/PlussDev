#include <pluss/Note.h>
#include <internals/aux/lexicon/text.h>

namespace lexicon {

static pluss::Text node_to_pluss_text(const TextStructure::TextNode& node) {
  if (node.is_leaf()) {
    return pluss::Text(std::get<pluss::Chord>(node.value));
  }
  const auto& vec = std::get<std::vector<TextStructure::TextNode>>(node.value);
  std::vector<pluss::Text> children;
  children.reserve(vec.size());
  for (const auto& n : vec) children.push_back(node_to_pluss_text(n));
  return pluss::Text(std::move(children));
}

pluss::Note TextStructure::convert_to_note() const {
  try {
    pluss::Text root_text = node_to_pluss_text(root_node());
    return pluss::Note::wrap_note_impl(pluss::intrusive_ptr<pluss::detail::NoteBook>::make(std::move(root_text)));
  } catch (const std::exception& e) {
    std::cerr << "lexicon::TextStructure::convert_to_note error: " << e.what() << std::endl;
    return pluss::Note();
  }
}

} // namespace lexicon
