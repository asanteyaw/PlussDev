const STOP_WORDS = new Set([
  'a', 'an', 'and', 'as', 'by', 'do', 'for', 'from', 'how', 'i', 'in',
  'is', 'of', 'on', 'or', 'pluss', 'plussii', 'the', 'to', 'use', 'with',
]);

const normalize = value => String(value ?? '').normalize('NFKD').toLowerCase();
const terms = value => [...new Set(normalize(value).match(/[\p{L}\p{N}_]+/gu) ?? [])]
  .filter(term => term.length > 1 && !STOP_WORDS.has(term));

export function createDocsCatalog(readText, { baseUrl = 'https://plussible.org', refreshMs = 300_000 } = {}) {
  let snapshot;
  let inFlight;
  let expiresAt = 0;
  const pages = new Map();

  async function load() {
    if (snapshot && Date.now() < expiresAt) return snapshot;
    if (inFlight) return inFlight;
    inFlight = (async () => {
      const revisionText = await readText('docs-source-revision.txt');
      const revision = revisionText.trim();
      if (!revision) throw new Error('Published docs revision is missing');
      if (snapshot?.revision === revision) {
        expiresAt = Date.now() + refreshMs;
        return snapshot;
      }
      const manifestText = await readText('docs-manifest.json');
      const entries = JSON.parse(manifestText);
      if (!Array.isArray(entries)) throw new Error('Published docs index is invalid');
      pages.clear();
      snapshot = {
        revision,
        entries,
        bySlug: new Map(entries.map(entry => [entry.slug, entry])),
      };
      expiresAt = Date.now() + refreshMs;
      return snapshot;
    })();
    try { return await inFlight; } finally { inFlight = undefined; }
  }

  function requireRevision(actual, requested) {
    if (requested && requested !== 'current' && requested !== actual) {
      throw new Error(`Requested docs revision ${requested} is unavailable; published revision is ${actual}`);
    }
  }

  async function status() {
    const current = await load();
    return { revision: current.revision, pages: current.entries.length, scope: 'currently published docs' };
  }

  async function search(query, { space, limit = 8, revision } = {}) {
    const current = await load();
    requireRevision(current.revision, revision);
    const requested = terms(query);
    if (!requested.length) throw new Error('Provide a specific API name or computing task to search');
    const phrase = normalize(query).trim();
    const canonicalPhrase = phrase.replace(/[^\p{L}\p{N}_]+/gu, ' ').trim().replace(/\s+/g, ' ');
    const scored = [];
    for (const entry of current.entries) {
      if (space && entry.space !== space) continue;
      const title = normalize(entry.title);
      const canonicalTitle = title.replace(/[^\p{L}\p{N}_]+/gu, ' ').trim().replace(/\s+/g, ' ');
      const slug = normalize(entry.slug);
      const source = normalize(entry.source);
      let score = canonicalTitle === canonicalPhrase ? 100 :
        canonicalTitle.includes(canonicalPhrase) ? 50 : title.includes(phrase) ? 50 : 0;
      let hits = 0;
      for (const term of requested) {
        if (title.includes(term)) { score += 12; hits++; }
        else if (slug.includes(term)) { score += 6; hits++; }
        else if (source.includes(term)) { score += 4; hits++; }
      }
      if (!hits) continue;
      score += hits === requested.length ? 12 : 0;
      scored.push({ entry, score });
    }
    scored.sort((a, b) => b.score - a.score || a.entry.title.localeCompare(b.entry.title));
    const count = Math.max(1, Math.min(20, Math.trunc(limit)));
    return {
      revision: current.revision,
      results: scored.slice(0, count).map(({ entry }) => ({
        title: entry.title,
        slug: entry.slug,
        space: entry.space,
        url: `${baseUrl}/docs/${entry.slug}`,
      })),
    };
  }

  async function read(slug, { revision, offset = 0, maxChars = 20_000 } = {}) {
    const current = await load();
    requireRevision(current.revision, revision);
    const entry = current.bySlug.get(slug);
    if (!entry) throw new Error(`No published docs page has slug ${slug}`);
    const cacheKey = `${current.revision}:${slug}`;
    let body = pages.get(cacheKey);
    if (body === undefined) {
      body = await readText(`docs-content/${entry.source}`);
      pages.set(cacheKey, body);
      if (pages.size > 64) pages.delete(pages.keys().next().value);
    } else {
      pages.delete(cacheKey);
      pages.set(cacheKey, body);
    }
    const start = Math.max(0, Math.min(body.length, Math.trunc(offset)));
    const length = Math.max(1_000, Math.min(30_000, Math.trunc(maxChars)));
    return {
      revision: current.revision,
      title: entry.title,
      slug,
      url: `${baseUrl}/docs/${slug}`,
      text: body.slice(start, start + length),
      nextOffset: start + length < body.length ? start + length : null,
    };
  }

  return { status, search, read };
}
