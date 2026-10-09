# `pluss::ai::nlp::SyntaxStage`

`SyntaxStage` builds a dependency-style structure over each sentence. Every
token receives a head, stored in `Token::chief`, and a grammatical role,
stored in `Token::role`. The head is the index of the token it depends on,
and the root of each sentence points at itself. The stage also groups nominal
words into noun phrases and writes them to `Doc::phrases`. Together these let
the tree accessors on [`Doc`](../core/Doc.md) answer structural questions:
[`children_of`](../core/Doc/children_of.md) lists the dependents of a token,
[`lineage_of`](../core/Doc/lineage_of.md) walks from a token up to its root,
and [`branch_of`](../core/Doc/branch_of.md) returns the contiguous subtree
below a token.

This is a baseline, not a trained parser. It applies fixed attachment rules to
the part-of-speech tags that the tagger wrote, so its quality is the quality of
those tags. It is cheap, deterministic, and transparent. A reader who needs the
reason for an attachment can read the rule that made it, and the rules are
listed under Methods.

The stage reads `Token::pos`, so it must run after a tagger. Without tags every
content word looks the same to it, and the structure it produces is then
shallow. The Practical Examples show both cases.

## Signature

```cpp
pluss::ai::nlp::SyntaxStage::SyntaxStage(
    pluss::ai::nlp::SyntaxOptions options = {});
pluss::ai::nlp::Stage pluss::ai::nlp::stage::syntax(
    pluss::ai::nlp::SyntaxOptions options = {});
```

## Construction

Use `stage::syntax` in a workflow. Construct the concrete class when you want a
typed handle, which is useful when the same options are reused across several
workflows.

The constructor stores the options and does no validation. Every combination of
the three boolean options is valid.

## Options

The options struct is `pluss::ai::nlp::SyntaxOptions`. All three fields default
to `true`.

### `prefer_verbs`

Controls how the root of a sentence is chosen. When `true`, the root is the
first main verb in the sentence, and an auxiliary is used only when the
sentence has no main verb. When `false`, the root is the first nominal word,
and a verb is attached below it.

The choice matters for sentences that begin with a noun. In `Ada writes the
APIs`, a verb root makes `Ada` the subject and `APIs` the object. A nominal
root makes `Ada` the root and turns `writes` into a complement. The verb
preference matches how most declarative sentences are built, which is why it
is the default. Turn it off when the text is a list of noun phrases, such as
titles or headings, where there is no verb to prefer.

When the sentence has no verb, an auxiliary, or a nominal word, the root falls
back to the first non-punctuation token. A sentence made only of punctuation
gets its first token as the root.

### `attach_punct`

Controls whether punctuation tokens are attached to the tree. When `true`, each
punctuation token is attached to a nearby verb or noun, and its role is
`punct`. When `false`, punctuation keeps no role, and its head stays at its own
index, which marks it as unattached.

Leave it on when the tree must cover every token, for example when a subtree
query should include the commas inside a clause. Turn it off when the
punctuation would only clutter a listing of the tree.

### `build_phrases`

Controls whether noun phrases are written to `Doc::phrases`. When `true`, each
nominal word that is not itself a modifier or a compound part starts a phrase.
The phrase extends left over its determiners, adjectives, numbers, and
compounds, and right over compounds that attach to it. Each phrase is a `Span`
labeled `phrase`.

When `false`, `Doc::phrases` is left empty, but the head and role of every
token are still computed. The phrases are a presentation of the structure, so
turning them off changes only the list, not the tree.

## State

The stage keeps only its three options. It does not override `learn`.

`dump_state_lines()` returns `prefer_verbs`, `attach_punct`, and
`build_phrases`, each set to `1` or `0`. `load_state_lines()` accepts those six
lines and rejects any other line with an error that names it.

## Methods

### `name()`

Returns `"syntax"`.

### `apply(Doc&)`

Clears `Doc::phrases`, then sets every token's head to its own index and clears
its role. It then parses each sentence in turn. When the document has no
sentences, the whole token stream is parsed as one sentence, so the stage works
without a sentencizer too.

Parsing a sentence follows these steps, in order, and each step only attaches a
token that no earlier step has attached:

1. Choose the root, as described under `prefer_verbs`.
2. Join runs of two or more adjacent nominal words. Every word in a run except
   the last becomes a `compound` dependent of the last word.
3. Attach modifiers. A determiner (`det`) and a number (`nummod`) attach to the
   nominal word that follows them, or to the root when none follows. An
   adjective (`amod`) attaches to the nominal word after it, then to the nominal
   word before it, and otherwise to the root. An adverb (`advmod`) attaches to
   the nearest verb on its left, then on its right, and otherwise to the root.
4. Attach function words. An auxiliary (`aux`) attaches to the root. A `to` or a
   subordinating conjunction (`mark`) attaches to the root. A coordinating
   conjunction (`cc`) attaches to the nearest verb or nominal word, and the next
   content word becomes its `conj` partner. A preposition (`prep`) attaches to
   the nearest verb or nominal word, and the next nominal word becomes its object
   (`pobj`), with the preposition as its head.
5. Attach content words. A nominal word before the root is a subject
   (`nsubj`), and one after it is an object (`obj`). A verb before the root is
   an auxiliary (`aux`), and one after it is an open complement (`xcomp`). Any
   other word becomes a generic dependent (`dep`) of the root.
6. Attach punctuation, when `attach_punct` is on, to the nearest verb or nominal
   word.
7. Build phrases, when `build_phrases` is on.

Apart from the empty role of an unattached token, the roles above are the only
ones this stage writes.

### `dump_state_lines()`

Returns the three option lines described under State.

### `load_state_lines(lines)`

Replaces the three options from a list of lines. Lines the stage does not
recognize are rejected.

## Practical Examples

These examples run the full pipeline up to the parser, print the tree in the
form a reader can follow, and then show the options that change the root, the
punctuation, and the phrases. The last example shows what happens when the
tagger is absent.

### Example 1

The full pipeline on a sentence with a subject, a verb, a determiner, a
compound, an adverb, a conjunction, and a number. Each token prints its tag,
its head, and its role, and the noun phrases follow.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::tagger(),
                              nlp::stage::sentencize(), nlp::stage::syntax()}};
  const nlp::Doc doc = wf.infer("Ada writes the APIs quickly, and Kofi reads 42 books.");

  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " / " << token.pos << " head=" << token.chief << " role=" << token.role << "\n";
  }
  std::cout << "\nphrases:\n";
  for (const nlp::Span& phrase : doc.phrases) {
    std::cout << "  \"" << phrase.text(doc) << "\"\n";
  }
  return 0;
}
```

Output

```text
Ada / PROPN head=1 role=nsubj
writes / VERB head=1 role=root
the / DET head=3 role=det
APIs / NOUN head=1 role=obj
quickly / ADV head=1 role=advmod
, / PUNCT head=1 role=punct
and / CCONJ head=1 role=cc
Kofi / PROPN head=1 role=conj
reads / VERB head=1 role=xcomp
42 / NUM head=10 role=nummod
books / NOUN head=1 role=obj
. / PUNCT head=8 role=punct

phrases:
  "Ada"
  "the APIs"
  "Kofi"
  "42 books"
```

### Example 2

The root choice with `prefer_verbs` on and off. The same noun-first sentence
gets a different root under each setting, and the role of each token changes
with it. The program prints every token with its head and role, so the
two trees can be compared directly.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  for (const bool verbs : {true, false}) {
    nlp::SyntaxOptions options{.prefer_verbs = verbs};
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::tagger(),
                                nlp::stage::syntax(options)}};
    const nlp::Doc doc = wf.infer("Ada writes the APIs.");

    std::cout << "prefer_verbs = " << (verbs ? "true" : "false") << "\n";
    for (const nlp::Token& token : doc.tokens) {
      std::cout << "  " << token.orth << " head=" << token.chief << " role=" << token.role << "\n";
    }
  }
  return 0;
}
```

Output

```text
prefer_verbs = true
  Ada head=1 role=nsubj
  writes head=1 role=root
  the head=3 role=det
  APIs head=1 role=obj
  . head=1 role=punct
prefer_verbs = false
  Ada head=0 role=root
  writes head=0 role=xcomp
  the head=3 role=det
  APIs head=0 role=obj
  . head=1 role=punct
```

### Example 3

The punctuation and phrase options, each on its own, against the defaults. The
program runs three configurations on the same sentence: the defaults, the
punctuation option off, and the phrase option off. With `attach_punct` off, the
period keeps no role and points at itself, and the phrases are unchanged. With
`build_phrases` off, the phrase list is empty, and the tree is unchanged.

```cpp
#include <pluss/nlp.h>

#include <iostream>
#include <string>
#include <vector>

int main() {
  namespace nlp = pluss::ai::nlp;

  struct Setting {
    std::string name;
    nlp::SyntaxOptions options;
  };
  const std::vector<Setting> settings = {
      {"defaults", {}},
      {"attach_punct = false", {.attach_punct = false}},
      {"build_phrases = false", {.build_phrases = false}},
  };

  for (const Setting& setting : settings) {
    nlp::Workflow wf{.lang = nlp::Language::English,
                     .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::tagger(),
                                nlp::stage::syntax(setting.options)}};
    const nlp::Doc doc = wf.infer("Ada writes the APIs.");
    const nlp::Token& period = doc.tokens.back();

    std::cout << setting.name << "\n";
    std::cout << "  period head=" << period.chief << " role=\"" << period.role << "\"\n";
    std::cout << "  phrases=" << doc.phrases.size() << "\n";
  }
  return 0;
}
```

Output

```text
defaults
  period head=1 role="punct"
  phrases=2
attach_punct = false
  period head=4 role=""
  phrases=2
build_phrases = false
  period head=1 role="punct"
  phrases=0
```

### Example 4

The tree accessors on a parsed sentence. The root's dependents come from
`children_of`, the path from a word up to the root comes from `lineage_of`, which starts
with the word itself,
and the subtree under a token comes from `branch_of`. Each step uses the same
`chief` values that the stage wrote.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::lemmatize(), nlp::stage::tagger(),
                              nlp::stage::syntax()}};
  const nlp::Doc doc = wf.infer("Ada writes the APIs quickly.");

  std::int64_t root = -1;
  for (const nlp::Token& token : doc.tokens) {
    if (token.role == "root") root = token.index;
  }
  std::cout << "root = " << doc.tokens[static_cast<std::size_t>(root)].orth << "\n";

  std::cout << "children of root:";
  for (const std::int64_t child : doc.children_of(root)) {
    std::cout << " " << doc.tokens[static_cast<std::size_t>(child)].orth;
  }
  std::cout << "\n";

  const std::int64_t apis = 3;
  std::cout << "lineage of " << doc.tokens[static_cast<std::size_t>(apis)].orth << ":";
  for (const std::int64_t step : doc.lineage_of(apis)) {
    std::cout << " " << doc.tokens[static_cast<std::size_t>(step)].orth;
  }
  std::cout << "\n";

  std::cout << "branch of root = \"" << doc.branch_of(root).text(doc) << "\"\n";
  return 0;
}
```

Output

```text
root = writes
children of root: Ada APIs quickly .
lineage of APIs: APIs writes
branch of root = "Ada writes the APIs quickly."
```

### Example 5

The stage without a tagger. Every token has an empty tag, so the root falls
back to the first content word and every other content word attaches as a
generic dependent. The structure is still produced, but it does not reflect
grammar, so the example prints it to make that visible.

```cpp
#include <pluss/nlp.h>

#include <iostream>

int main() {
  namespace nlp = pluss::ai::nlp;

  nlp::Workflow wf{.lang = nlp::Language::English,
                   .stages = {nlp::stage::tokenize(), nlp::stage::syntax()}};
  const nlp::Doc doc = wf.infer("Ada writes the APIs.");

  for (const nlp::Token& token : doc.tokens) {
    std::cout << token.orth << " pos=\"" << token.pos << "\" head=" << token.chief
              << " role=\"" << token.role << "\"\n";
  }
  return 0;
}
```

Output

```text
Ada pos="" head=0 role="root"
writes pos="" head=0 role="dep"
the pos="" head=0 role="dep"
APIs pos="" head=0 role="dep"
. pos="" head=0 role="punct"
```

## See Also

- `stage::syntax` -- the factory, documented under Signature, that returns this stage as a `Stage`.
- [`TaggerStage`](TaggerStage.md) -- supplies the tags this stage reads.
- [`Doc::children_of`](../core/Doc/children_of.md), [`Doc::lineage_of`](../core/Doc/lineage_of.md), and [`Doc::branch_of`](../core/Doc/branch_of.md) -- read the tree this stage writes.
- [`Token`](../core/Token.md) -- the `chief` and `role` fields this stage fills.

## Changelog

- **0.1** -- initial documented surface.

<!-- apiref:toctree:start -->

<!-- apiref:toctree:end -->
