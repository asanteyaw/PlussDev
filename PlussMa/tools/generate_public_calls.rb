#!/usr/bin/env ruby

require "fileutils"
require "json"
require "open3"
require "pathname"
require "shellwords"
require "tmpdir"
require "yaml"

ROOT = Pathname.new(__dir__).join("../..").expand_path
PROBE = ROOT.join("PlussMa/tools/public_calls_probe.cpp")
OUTPUT = ROOT.join("PlussMa/public_calls.md")
BUILD = Pathname.new(ENV.fetch("PLUSS_BUILD_DIR", ROOT.join("build_cpp_env").to_s))
COMPILE_DB = BUILD.join("compile_commands.json")
CLANG_DOC = ENV.fetch("CLANG_DOC", "/opt/homebrew/opt/llvm/bin/clang-doc")

abort "missing compile database: #{COMPILE_DB}" unless COMPILE_DB.file?
abort "missing clang-doc: #{CLANG_DOC}" unless File.executable?(CLANG_DOC)

def command_arguments(entry)
  args = entry["arguments"] || Shellwords.split(entry.fetch("command"))
  source = entry.fetch("file")
  kept = []
  skip = false
  args.each_with_index do |arg, index|
    if skip
      skip = false
      next
    end
    if arg == "-o"
      skip = true
      next
    end
    next if arg == "-c" || arg == source || File.expand_path(arg, entry["directory"]) == File.expand_path(source)
    kept << arg
  end
  kept + ["-c", PROBE.to_s]
end

database = JSON.parse(COMPILE_DB.read)
entry = database.find { |row| row.fetch("file").include?("/PlussMa/Eng/") }
abort "compile database has no PlussMa Engineering translation unit" unless entry

documents = []
Dir.mktmpdir("plussma-public-calls") do |directory|
  temp = Pathname.new(directory)
  temp.join("compile_commands.json").write(JSON.pretty_generate([
    {
      "directory" => entry.fetch("directory"),
      "arguments" => command_arguments(entry),
      "file" => PROBE.to_s,
    },
  ]))

  docs = temp.join("docs")
  command = [CLANG_DOC, "--format=yaml", "--public", "--output=#{docs}",
             "-p", temp.to_s, PROBE.to_s]
  stdout, stderr, status = Open3.capture3(*command, chdir: ROOT.to_s)
  abort "clang-doc failed:\n#{stdout}\n#{stderr}" unless status.success?
  documents = Dir[docs.join("*.yaml").to_s].map do |file|
    YAML.safe_load(File.read(file), [], [], false)
  rescue Psych::SyntaxError => error
    warn "ignoring malformed clang-doc output #{file}: #{error.message}"
    nil
  end.compact
end

PUBLIC_PREFIXES = [
  "pluss::subsp::natsci",
  "pluss::subsp::eng",
  "pluss::subsp::math::qp",
  "pluss::subsp::math::bezier",
  "pluss::subsp::math::multigrid",
  "pluss::subsp::math::operators::expansion",
].freeze

PUBLIC_PATHS = [
  "PlussMa/include/pluss/subsp/natsci/",
  "PlussMa/include/pluss/subsp/eng/",
  "PlussMa/include/pluss/subsp/math/qp.h",
  "PlussMa/include/pluss/subsp/math/bezier.h",
  "PlussMa/include/pluss/subsp/math/multigrid.h",
  "PlussMa/include/pluss/subsp/math/operators.h",
].freeze

def namespace_of(item)
  Array(item["Namespace"]).first&.fetch("QualName", nil)
end

def qualified_name(item)
  parent = item.dig("Parent", "QualName") || namespace_of(item)
  [parent, item["Name"]].compact.reject(&:empty?).join("::")
end

def public_namespace?(name)
  segments = name&.split("::") || []
  internal_namespace = segments.any? do |segment|
    segment == "detail" || segment.end_with?("_detail")
  end
  name && PUBLIC_PREFIXES.any? { |prefix| name == prefix || name.start_with?("#{prefix}::") } &&
    !internal_namespace
end

def declaration_paths(item)
  paths = []
  paths << item.dig("DefLocation", "Filename")
  Array(item["Location"]).each { |location| paths << location["Filename"] }
  paths.compact
end

def public_header_declaration?(item)
  declaration_paths(item).any? { |path| PUBLIC_PATHS.any? { |prefix| path.start_with?(prefix) } }
end

def type_name(type)
  node = type
  node = node["Type"] while node.is_a?(Hash) && node["Type"].is_a?(Hash)
  node && (node["QualName"] || node["Name"]) || "void"
end

def call_signature(call)
  result = type_name(call["ReturnType"])
  owner = call.dig("Parent", "QualName") || namespace_of(call)
  name = [owner, call["Name"]].compact.join("::")
  params = Array(call["Params"]).map do |param|
    text = type_name(param["Type"])
    text += " #{param["Name"]}" if param["Name"] && !param["Name"].empty?
    text += " = #{param["DefaultValue"]}" if param.key?("DefaultValue")
    text
  end
  "#{result} #{name}(#{params.join(", ")})"
end

namespaces = {}
records = {}
enums = {}
functions = {}

documents.each do |document|
  next unless document.is_a?(Hash)
  document_name = qualified_name(document)
  document_namespace = namespace_of(document)

  if public_namespace?(document_name) && document["TagType"] && public_header_declaration?(document)
    records[document_name] ||= document
  end

  Array(document["ChildNamespaces"]).each do |child|
    name = qualified_name(child)
    namespaces[name] = child if public_namespace?(name)
  end

  Array(document["ChildEnums"]).each do |child|
    name = qualified_name(child)
    next unless public_namespace?(namespace_of(child)) && public_header_declaration?(child)
    enums[name] ||= child
  end

  Array(document["ChildRecords"]).each do |child|
    name = qualified_name(child)
    next unless public_namespace?(namespace_of(child)) && public_header_declaration?(child)
    records[name] ||= child
  end

  unless document["TagType"]
    Array(document["ChildFunctions"]).each do |child|
      name = qualified_name(child)
      next unless public_namespace?(namespace_of(child)) && public_header_declaration?(child)
      next if child["Access"] && child["Access"] != "Public"
      next if call_signature(child).include?("detail::")
      functions[child["USR"] || call_signature(child)] = child
    end
  end

  if public_namespace?(document_name) && !document["TagType"]
    namespaces[document_name] ||= document
  elsif public_namespace?(document_namespace) && document["TagType"]
    records[document_name] ||= document if public_header_declaration?(document)
  end
end

# ---- intent layer (tools/public_intent.yaml): drop helpers the author does not intend as API, remember them for the "internal by intent" section
INTENT = YAML.safe_load(File.read(__dir__ + "/public_intent.yaml"), [], [], false) || {}
INTERNAL = {}
Array(INTENT["internal"]).each do |entry|
  Array(entry["names"]).each { |n| INTERNAL[[entry["header"], n]] = entry["reason"] }
end
FRONT_DOORS = INTENT["front_doors"] || {}
natsci_header = lambda do |item|
  path = declaration_paths(item).find { |p| p.start_with?("PlussMa/include/pluss/subsp/natsci/") }
  path&.sub("PlussMa/include/pluss/subsp/natsci/", "")
end
hidden = []
intent_internal = lambda do |item, name|
  header = natsci_header.call(item)
  reason = header && INTERNAL[[header, name]]
  hidden << [header, name, reason] if reason
  reason
end
records.delete_if { |qualified, record| intent_internal.call(record, qualified.split("::").last) }
functions.delete_if { |_, function| intent_internal.call(function, function["Name"]) }

families = Hash.new { |hash, key| hash[key] = {records: [], enums: [], functions: []} }

family_for = lambda do |name|
  if name.start_with?("pluss::subsp::natsci")
    tail = name.delete_prefix("pluss::subsp::natsci").sub(/^::/, "")
    first = tail.split("::").first
    ["NatSci", first ? "pluss::subsp::natsci::#{first}" : "pluss::subsp::natsci"]
  elsif name.start_with?("pluss::subsp::eng")
    tail = name.delete_prefix("pluss::subsp::eng").sub(/^::/, "")
    first = tail.split("::").first
    ["Engineering", first ? "pluss::subsp::eng::#{first}" : "pluss::subsp::eng"]
  elsif name.start_with?("pluss::subsp::math::qp")
    ["Engineering shared owner", "pluss::subsp::math::qp"]
  elsif name.start_with?("pluss::subsp::math::operators::expansion")
    ["Shared math owners", "pluss::subsp::math::operators::expansion"]
  else
    ["Shared math owners", name.split("::")[0, 4].join("::")]
  end
end

records.each do |name, record|
  section, family = family_for.call(name)
  families[[section, family]][:records] << [name, record]
end
enums.each do |name, enum|
  section, family = family_for.call(name)
  families[[section, family]][:enums] << [name, enum]
end
functions.each_value do |function|
  name = qualified_name(function)
  section, family = family_for.call(name)
  families[[section, family]][:functions] << function
end

display_name = lambda do |name|
  name
end

# every front door must be a declared public call (a typo or a symbol later hidden/removed fails the run instead of misleading the docs)
known = {}
functions.each_value { |f| known[display_name.call(qualified_name(f))] = true }
records.each_key { |n| known[display_name.call(n)] = true }
FRONT_DOORS.each do |module_name, names|
  Array(names).each do |n|
    abort "front door #{module_name}::#{n} is not a public call in the generated inventory" unless known["#{module_name}::#{n}"]
  end
end

lines = []
lines << "# PlussMa Public Calls"
lines << ""
lines << "> Generated by `PlussMa/tools/generate_public_calls.rb`. Do not edit the inventories by hand."
lines << ""
lines << "This file is the documentation boundary for the Natural Sciences and Engineering code implemented so far. A declaration is a documentation target only when it appears below. Installed-header reachability alone does not make a helper public. In particular, `detail::*`, private and protected members, source-only declarations, friend-only construction hooks, and implementation records are internal and must not appear in API prose."
lines << ""
lines << "## Canonical entry points"
lines << ""
lines << "| Area | Canonical public spelling | Installed umbrella |"
lines << "| --- | --- | --- |"
lines << "| Natural Sciences | `pluss::subsp::natsci::*` | `<pluss/ma/natsci.h>` |"
lines << "| Engineering | `pluss::subsp::eng::*` | `<pluss/ma/eng.h>` |"
lines << "| Dense QP shared by Engineering MPC | `pluss::subsp::math::qp::*` | `<pluss/subsp/math/qp.h>` |"
lines << "| Structured-grid multigrid shared by NatSci fluids | `pluss::subsp::math::multigrid::*` | `<pluss/subsp/math/multigrid.h>` |"
lines << ""
lines << "Neither area currently defines a root accessor object. If a later tranche adds one, its owning type, accessor name, accessor type, and every intended accessor method must appear in this file before documentation is written."
lines << ""

["NatSci", "Engineering", "Engineering shared owner", "Shared math owners"].each do |section|
  selected = families.select { |(name, _), _| name == section }
  next if selected.empty?
  lines << "## #{section}"
  lines << ""
  selected.keys.sort_by { |(_, family)| family }.each do |key|
    family = key[1]
    inventory = families[key]
    lines << "### `#{family}`"
    lines << ""
    doors = FRONT_DOORS.select { |module_name, _| module_name == family || module_name.start_with?("#{family}::") }
    unless doors.empty?
      lines << "Front doors (intended starting points):"
      lines << ""
      doors.sort.each { |module_name, names| lines << "- `#{module_name}`: #{Array(names).map { |n| "`#{n}`" }.join(", ")}" }
      lines << ""
    end

    unless inventory[:enums].empty?
      lines << "| Public enum | Values |"
      lines << "| --- | --- |"
      inventory[:enums].sort_by(&:first).each do |name, enum|
        values = Array(enum["Members"]).map { |member| "`#{member["Name"]}`" }.join(", ")
        lines << "| `#{display_name.call(name)}` | #{values} |"
      end
      lines << ""
    end

    unless inventory[:records].empty?
      lines << "| Public type | Public fields | Public methods |"
      lines << "| --- | --- | --- |"
      inventory[:records].sort_by(&:first).each do |name, record|
        fields = Array(record["Members"]).select { |member| !member["Access"] || member["Access"] == "Public" }.map do |member|
          "`#{member["Name"]}`"
        end
        methods = Array(record["ChildFunctions"]).select do |method|
          (!method["Access"] || method["Access"] == "Public") && public_header_declaration?(method)
        end.map { |method| "`#{method["Name"]}()`" }.uniq.sort
        lines << "| `#{display_name.call(name)}` | #{fields.empty? ? "-" : fields.join(", ")} | #{methods.empty? ? "-" : methods.join(", ")} |"
      end
      lines << ""
    end

    unless inventory[:functions].empty?
      lines << "| Public free-call overload | Header |"
      lines << "| --- | --- |"
      inventory[:functions].sort_by { |function| [qualified_name(function), call_signature(function)] }.each do |function|
        header = declaration_paths(function).find { |path| PUBLIC_PATHS.any? { |prefix| path.start_with?(prefix) } }
        signature = display_name.call(call_signature(function)).gsub("|", "\\|")
        lines << "| `#{signature}` | `#{header}` |"
      end
      lines << ""
    end
  end
end

unless hidden.empty?
  lines << "## Internal by intent (visible in headers, never document)"
  lines << ""
  lines << "Recorded in `PlussMa/tools/public_intent.yaml`. These compile as public C++ but are helpers, validators or constant accessors."
  lines << ""
  lines << "| Header | Symbol | Reason |"
  lines << "| --- | --- | --- |"
  hidden.uniq.sort.each { |header, name, reason| lines << "| `#{header}` | `#{name}` | #{reason} |" }
  lines << ""
end

lines << "## Maintenance rule"
lines << ""
lines << "Every NatSci or Engineering implementation tranche must regenerate this file after its public headers compile. Documentation authors should use the canonical `pluss::subsp::*` spelling shown here and should not infer public status from a source symbol or a friend declaration. Intent is recorded in `PlussMa/tools/public_intent.yaml`: a header added by a tranche must list its internal helpers and its front doors there in the same change, and `Internal by intent` above names what stays out of prose."
lines << ""

OUTPUT.write(lines.join("\n"))
puts "wrote #{OUTPUT.relative_path_from(ROOT)} (#{records.size} types, #{enums.size} enums, #{functions.size} overloads)"
