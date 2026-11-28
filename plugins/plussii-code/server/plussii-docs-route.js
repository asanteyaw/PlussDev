// Copy this file as frontend/pages/api/plussii-docs.js with its sibling .mjs modules.
import { toNodeHandler } from '@modelcontextprotocol/node';
import { createPlussiiDocsHandler } from './docs-mcp.mjs';

export const config = { api: { bodyParser: false, responseLimit: false } };

const handlers = new Map();

function originFor(request) {
  const host = String(request.headers.host || '').toLowerCase();
  const deployed = process.env.VERCEL_URL?.toLowerCase();
  const production = host === 'plussible.org' || host === 'www.plussible.org';
  const preview = deployed && host === deployed;
  const local = !deployed && /^localhost:\d+$/.test(host);
  if (!production && !preview && !local) return null;
  const origin = request.headers.origin;
  if (origin && origin !== `https://${host}` && origin !== `http://${host}` &&
      origin !== 'https://chatgpt.com') return null;
  return local ? `http://${host}` : `https://${host}`;
}

function handlerFor(origin) {
  let handler = handlers.get(origin);
  if (handler) return handler;
  const readText = async name => {
    const response = await fetch(`${origin}/${name}`, { signal: AbortSignal.timeout(8000) });
    if (!response.ok) throw new Error(`Published documentation is unavailable (${response.status})`);
    return response.text();
  };
  handler = toNodeHandler(createPlussiiDocsHandler(readText, { baseUrl: 'https://plussible.org' }));
  handlers.set(origin, handler);
  return handler;
}

export default async function plussiiDocs(request, response) {
  const origin = originFor(request);
  if (!origin) return response.status(403).end();
  return handlerFor(origin)(request, response);
}
