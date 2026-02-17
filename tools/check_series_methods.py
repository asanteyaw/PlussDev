#!/usr/bin/env python3
import re
from pathlib import Path

root = Path('/Users/yawasante/software/cxx/PlussMa')
header = root / 'PlussQit' / 'src' / 'CodeM' / 'generated' / 'SeriesMethodsGen.h'
invoke = root / 'PlussQit' / 'src' / 'CodeM' / 'compat' / 'Invoke.cpp'

hdr_text = header.read_text()
inv_text = invoke.read_text()

# extract op strings from header: invoke("op"
hdr_ops = set(re.findall(r"invoke\(\"([a-zA-Z0-9_]+)\"", hdr_text))
# also include invoke_("op" in header for inplace wrappers
hdr_ops |= set(re.findall(r"invoke_\(\"([a-zA-Z0-9_]+)\"", hdr_text))

# extract implemented ops from Invoke.cpp: if (op == "op")
impl_ops = set(re.findall(r"if \(op == \"([a-z0-9_]+)\"\)", inv_text))
# also consider not_impl and other occurrences where op string might be referenced
impl_ops |= set(re.findall(r"not_impl\(\"([a-z0-9_]+)\"", inv_text))

# Compute differences
missing = sorted(hdr_ops - impl_ops)
implemented = sorted(hdr_ops & impl_ops)

print(f"Header ops: {len(hdr_ops)}")
print(f"Implemented ops found in Invoke.cpp: {len(impl_ops)}")
print(f"Methods declared in header: {len(hdr_ops)}")
print(f"Implemented (matched): {len(implemented)}")
print(f"Missing (declared in header but not routed in Invoke.cpp): {len(missing)}")
print('\n--- Missing methods (full list) ---')
for i, m in enumerate(missing, 1):
    print(m)

# Print small sample of implemented ones for sanity
print('\n--- Sample implemented ops ---')
for s in implemented[:80]:
    print(s)
