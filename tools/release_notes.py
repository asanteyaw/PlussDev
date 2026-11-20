#!/usr/bin/env python3
"""Draft release notes from the Git history since the latest Steady tag."""

import argparse
import collections
import re
import subprocess
from pathlib import Path


def git(*arguments: str) -> str:
    return subprocess.check_output(['git', *arguments], text=True).strip()


def latest_steady_tag(head: str) -> str:
    try:
        return git('describe', '--tags', '--abbrev=0', '--match', 'v[0-9]*', head)
    except subprocess.CalledProcessError as error:
        raise SystemExit('No Steady tag found; provide --base explicitly.') from error


def category(subject: str) -> str:
    text = subject.lower()
    if re.search(r'\b(api|signature|accessor|deprecat|rename)\b', text):
        return 'API changes'
    if re.search(r'\b(fix|repair|correct|resolve|bug|regression|portability)\b', text):
        return 'Fixes'
    if re.search(r'\b(add|introduce|implement|support|feature|extend|enhance)\b', text):
        return 'Features'
    if re.search(r'\b(doc|docs|guide|reference|example)\b', text):
        return 'Documentation'
    if re.search(r'\b(build|cmake|ci|release|package|workflow)\b', text):
        return 'Build and distribution'
    return 'Other changes'


def draft(base: str, head: str) -> str:
    commits = []
    for line in git('log', '--no-merges', '--format=%h%x09%s', f'{base}..{head}').splitlines():
        if '\t' in line:
            commits.append(line.split('\t', 1))
    groups: dict[str, list[str]] = collections.defaultdict(list)
    for short_hash, subject in commits:
        groups[category(subject)].append(f'- {subject} (`{short_hash}`)')

    areas = collections.Counter()
    for path in git('diff', '--no-renames', '--name-only', f'{base}..{head}').splitlines():
        root = path.split('/', 1)[0]
        if root in {'PlussQit', 'PlussMic', 'PlussMa', 'PlussPl', 'PlussAI', 'PlussTils'}:
            areas[root] += 1
        elif root in {'docs', 'cmake', '.github', 'tools'}:
            areas[root] += 1
    lines = [f'# Release notes draft: {base} to {head}', '',
             'Review this draft against the tested archives before publishing. Commit subjects are grouped automatically.', '']
    for heading in ('Features', 'Fixes', 'API changes', 'Documentation', 'Build and distribution', 'Other changes'):
        if groups[heading]:
            lines.extend([f'## {heading}', '', *groups[heading], ''])
    lines.extend(['## Changed areas', ''])
    lines.extend(f'- {area}: {count} changed files' for area, count in areas.most_common())
    lines.extend(['', f'Compare: https://github.com/asanteyaw/PlussDev/compare/{base}...{head}', ''])
    return '\n'.join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--base', help='Steady tag or commit to compare (default: latest ancestor Steady tag)')
    parser.add_argument('--head', default='HEAD')
    parser.add_argument('--output', default='-', help='Draft Markdown path, or - for stdout')
    arguments = parser.parse_args()
    base = arguments.base or latest_steady_tag(arguments.head)
    content = draft(base, arguments.head)
    if arguments.output == '-':
        print(content, end='')
    else:
        Path(arguments.output).write_text(content, encoding='utf-8')


if __name__ == '__main__':
    main()
