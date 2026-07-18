import * as path from 'path';
import { ParseEmailResult } from './types.js';

// Load the native addon from build/Release/parser.node
// prebuild-install downloads it there from GitHub releases on npm install
declare const __dirname: string;
const addonPath = path.resolve(__dirname, '..', 'build', 'Release', 'parser.node');

// eslint-disable-next-line @typescript-eslint/no-require-imports
const addon = require(addonPath);

export async function parseEmail(buffer: Buffer): Promise<ParseEmailResult> {
  return addon.parseEmail(buffer);
}

export function cleanHtml(html: string): string {
  return addon.cleanHtml(html);
}

export function removeSignature(html: string): string {
  return addon.removeSignature(html);
}

export function removeReplies(html: string): string {
  return addon.removeReplies(html);
}

export function removeDividers(html: string): string {
  return addon.removeDividers(html);
}

// Types
export type { IFile, ParseEmailResult } from './types.js';
