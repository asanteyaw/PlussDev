import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import path from 'node:path';
import { test } from 'node:test';
import { createDocsCatalog } from './docs-catalog.mjs';

const publicDir = process.env.PLUSSII_DOCS_PUBLIC_DIR;
if (!publicDir) throw new Error('Set PLUSSII_DOCS_PUBLIC_DIR to the site frontend/public directory');
let reads = 0;
const catalog = createDocsCatalog(async name => {
  reads++;
  const fullPath = path.resolve(publicDir, name);
  if (!fullPath.startsWith(path.resolve(publicDir) + path.sep)) throw new Error('Invalid docs path');
  return readFile(fullPath, 'utf8');
});

test('searches the published index without reading every page', async () => {
  const result = await catalog.search('DataFrame describe');
  assert.ok(result.revision);
  assert.ok(result.results.some(page => page.slug === 'mic/api/dataframe/describe'));
  assert.equal(reads, 2);
  await catalog.search('Series mean');
  assert.equal(reads, 2);
  const exact = await catalog.search('Series mean');
  assert.equal(exact.results[0].slug, 'qit/api/series/mean');
});

test('reads only selected pages and pins the revision', async () => {
  const status = await catalog.status();
  const page = await catalog.read('qit/api/series/mean', { revision: status.revision });
  assert.match(page.text, /Series::mean/);
  assert.equal(page.nextOffset, null);
  const before = reads;
  await catalog.read('qit/api/series/mean');
  assert.equal(reads, before);
  await assert.rejects(catalog.read('qit/api/series/mean', { revision: 'obsolete' }), /unavailable/);
});

test('refresh checks the small revision marker before downloading the index again', async () => {
  let revision = 'first';
  const requested = [];
  const fast = createDocsCatalog(async name => {
    requested.push(name);
    if (name === 'docs-source-revision.txt') return revision;
    if (name === 'docs-manifest.json') return '[]';
    throw new Error(name);
  }, { refreshMs: 0 });
  await fast.status();
  await fast.status();
  assert.deepEqual(requested, [
    'docs-source-revision.txt', 'docs-manifest.json', 'docs-source-revision.txt',
  ]);
  revision = 'second';
  assert.equal((await fast.status()).revision, 'second');
  assert.equal(requested.at(-1), 'docs-manifest.json');
});
