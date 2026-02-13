#!/usr/bin/env python3
from pathlib import Path
import re
DECL=Path('PlussQit/src/CodeM/core/SeriesMethods.inc')
IMPL=Path('PlussQit/src/CodeM/core/SeriesMethods.cpp.inc')
text_decl=DECL.read_text(encoding='utf-8')
text_impl=IMPL.read_text(encoding='utf-8')

def extract_decls(t):
    p=re.compile(r"\bSeries\b\s*(?:&\s*)?([A-Za-z0-9_]+)\s*\((.*?)\)\s*([^;]*);", re.S)
    out=[]
    for m in p.finditer(t):
        name=m.group(1)
        params=m.group(2).strip()
        cnt=0 if not params else len([p for p in re.split(r',\s*(?![^()]*\))', params) if p.strip()])
        out.append({'name':name,'params':params,'cnt':cnt,'raw':m.group(0).strip()})
    return out

def extract_impls(t):
    p=re.compile(r"Series(?:\s*&)?\s+Series::([A-Za-z0-9_]+)\s*\((.*?)\)\s*([^\{]*?)\{", re.S)
    out=[]
    for m in p.finditer(t):
        name=m.group(1)
        params=m.group(2).strip()
        cnt=0 if not params else len([p for p in re.split(r',\s*(?![^()]*\))', params) if p.strip()])
        raw = m.group(0).strip()
        out.append({'name':name,'params':params,'cnt':cnt,'raw':raw})
    return out

decls=extract_decls(text_decl)
impls=extract_impls(text_impl)
print('declaration count=',len(decls))
print('implementation count=',len(impls))

decl_set=set((d['name'],d['cnt']) for d in decls)
impl_set=set((i['name'],i['cnt']) for i in impls)

impl_not_decl=[i for i in impls if (i['name'],i['cnt']) not in decl_set]

decl_not_impl=[d for d in decls if (d['name'],d['cnt']) not in impl_set]

print('\nimpl entries without matching decl (name,param_count)=',len(impl_not_decl))
for i in impl_not_decl[:200]:
    print('-', i['name'], 'cnt=', i['cnt'], 'params=', i['params'])

print('\ndecl entries without matching impl (name,param_count)=',len(decl_not_impl))
for d in decl_not_impl[:200]:
    print('-', d['name'], 'cnt=', d['cnt'], 'params=', d['params'])
