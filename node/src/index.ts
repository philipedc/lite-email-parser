import { createRequire } from 'module';
import * as path from 'path';
import { IFile, ParseEmailResult } from './types.js';

declare const require: any;
declare const __dirname: string;

const _require = typeof require !== 'undefined'
  ? require
  : createRequire(process.cwd());

const getDirname = (): string => {
  if (typeof __dirname !== 'undefined') {
    return __dirname;
  }
  const err = new Error();
  const stackLines = err.stack?.split('\n') || [];
  for (const line of stackLines) {
    const match = line.match(/(?:at\s+|\()((?:file:\/\/)?\/[^:]+)/);
    if (match) {
      const p = match[1];
      if (p.includes('index.js') || p.includes('index.ts')) {
        let cleanPath = p;
        if (cleanPath.startsWith('file://')) {
          const fileURLToPath = _require('url').fileURLToPath;
          cleanPath = fileURLToPath(cleanPath);
        }
        return path.dirname(cleanPath);
      }
    }
  }
  return process.cwd();
};

const dirname = getDirname();

let addonPath = path.resolve(dirname, '../build/Release/parser.node');
if (!_require('fs').existsSync(addonPath)) {
  addonPath = path.resolve(dirname, '../../build/Release/parser.node');
}

const addon = _require(addonPath);

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
