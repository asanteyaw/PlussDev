#!/usr/bin/env python3
import json
import argparse
from pathlib import Path

RET_MAP = {
  'Chord': 'Chord',
  'Note': 'Note',
  'bool': 'bool',
  'u8list': '::std::vector<::std::uint8_t>',
  'i64list': '::std::vector<::std::int64_t>',
  'TokenBatch': 'TokenBatch',
}

ARG_MAP = {
  'string': '::std::string_view',
  'int64': '::std::int64_t',
  'intlist': '::std::span<const ::std::int64_t>',
}

GROUP_TO_SELF = {
  'Chord': 'Chord',
  'Chord->bool': 'Chord',
  'Note': 'Note',
  'Note->u8list': 'Note',
  'Note->i64list': 'Note',
  'Note->TokenBatch': 'Note',
}

GROUP_TO_RET = {
  'Chord': 'Chord',
  'Chord->bool': 'bool',
  'Note': 'Note',
  'Note->u8list': 'u8list',
  'Note->i64list': 'i64list',
  'Note->TokenBatch': 'TokenBatch',
}


def gen_for(target: str, ops: list[dict]) -> str:
  out = []
  out.append('#pragma once')
  out.append('')
  out.append('#include <cstdint>')
  out.append('#include <span>')
  out.append('#include <string_view>')
  out.append('#include <vector>')
  out.append('#include <variant>')
  out.append('')
  out.append('#include "CodeN/compat/Invoke.h"')
  out.append('')
  out.append('namespace pluss::coden {')
  out.append('')

  for op in ops:
    group = op['group']
    if GROUP_TO_SELF.get(group) != target:
      continue

    name = op['name']
    ret_kind = GROUP_TO_RET[group]
    ret = RET_MAP[ret_kind]

    args = op.get('args', [])
    params = []
    call_args = []
    for i, a in enumerate(args):
      ctype = ARG_MAP[a]
      pname = f'a{i}'
      params.append(f'{ctype} {pname}')
      call_args.append(pname)

    params_s = ', '.join(params)
    call_s = ', '.join(call_args)
    if call_s:
      call_s = ', ' + call_s

    out.append(f'inline {ret} {target}::{name}({params_s}) const {{')
    out.append(f'  return ::pluss::coden::_compat::invoke<{ret}>("{name}", *this{call_s});')
    out.append('}')
    out.append('')

  out.append('} // namespace pluss::coden')
  out.append('')
  return '\n'.join(out)


def main():
  ap = argparse.ArgumentParser()
  ap.add_argument('--schema', required=True)
  ap.add_argument('--out-dir', required=True)
  args = ap.parse_args()

  schema = json.loads(Path(args.schema).read_text(encoding='utf-8'))
  ops = schema['ops']

  out_dir = Path(args.out_dir)
  out_dir.mkdir(parents=True, exist_ok=True)

  (out_dir / 'ChordMethodsGen.h').write_text(gen_for('Chord', ops), encoding='utf-8')
  (out_dir / 'NoteMethodsGen.h').write_text(gen_for('Note', ops), encoding='utf-8')


if __name__ == '__main__':
  main()
