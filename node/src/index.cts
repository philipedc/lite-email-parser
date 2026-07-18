import * as path from 'path';
import { IFile, ParseEmailResult } from './types.js';

// Load native addon - CJS uses __dirname
const packageRoot = path.resolve(__dirname, '..');
const addon = require(path.join(packageRoot, 'build', 'Release', 'parser.node'));

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

export function replaceSrc(html: string, files: IFile[]): string {
  return addon.replaceSrc(html, files);
}

// Types
export type { IFile, ParseEmailResult } from './types.js';
