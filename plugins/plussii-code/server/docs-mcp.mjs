import { createMcpHandler, McpServer } from '@modelcontextprotocol/server';
import * as z from 'zod/v4';
import { createDocsCatalog } from './docs-catalog.mjs';

const result = value => ({ content: [{ type: 'text', text: JSON.stringify(value) }] });
const failure = error => ({
  isError: true,
  content: [{ type: 'text', text: error instanceof Error ? error.message : 'Documentation lookup failed' }],
});

export function createPlussiiDocsHandler(readText, options = {}) {
  const catalog = createDocsCatalog(readText, options);
  return createMcpHandler(() => {
    const server = new McpServer({ name: 'plussii-docs', version: '0.1.0' });
    server.registerTool('docs_status', {
      description: 'Report the exact revision and page count of the currently published PlussII documentation.',
    }, async () => {
      try { return result(await catalog.status()); } catch (error) { return failure(error); }
    });
    server.registerTool('search_docs', {
      description: 'Search current PlussII documentation titles and paths. Call read_doc on exact matches before writing code. Results include the source revision.',
      inputSchema: z.object({
        query: z.string().min(2).max(160),
        space: z.string().optional(),
        limit: z.number().int().min(1).max(20).optional(),
        revision: z.string().optional(),
      }),
    }, async ({ query, space, limit, revision }) => {
      try { return result(await catalog.search(query, { space, limit, revision })); }
      catch (error) { return failure(error); }
    });
    server.registerTool('read_doc', {
      description: 'Read one exact PlussII documentation page from the published snapshot. Use revision from search_docs to prevent mixing snapshots. Long pages can be continued with nextOffset.',
      inputSchema: z.object({
        slug: z.string().min(1).max(512),
        revision: z.string().optional(),
        offset: z.number().int().min(0).optional(),
        maxChars: z.number().int().min(1000).max(30000).optional(),
      }),
    }, async ({ slug, revision, offset, maxChars }) => {
      try { return result(await catalog.read(slug, { revision, offset, maxChars })); }
      catch (error) { return failure(error); }
    });
    return server;
  }, { responseMode: 'json' });
}
