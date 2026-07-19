import * as os from 'os';
import { IFile, ParseEmailResult } from './types.js';

// Map of platform-arch to the corresponding scoped package name
const PLATFORM_PACKAGES: Record<string, string> = {
  'linux-x64': '@lite-email-parser-native/linux-x64',
  'linux-arm64': '@lite-email-parser-native/linux-arm64',
  'darwin-x64': '@lite-email-parser-native/darwin-x64',
  'darwin-arm64': '@lite-email-parser-native/darwin-arm64',
  'win32-x64': '@lite-email-parser-native/win32-x64',
};

function loadAddon() {
  const platform = os.platform();
  const arch = os.arch();
  const key = `${platform}-${arch}`;
  const packageName = PLATFORM_PACKAGES[key];

  if (!packageName) {
    throw new Error(
      `lite-email-parser: unsupported platform ${platform}-${arch}. ` +
      `Supported: ${Object.keys(PLATFORM_PACKAGES).join(', ')}`
    );
  }

  try {
    return require(packageName);
  } catch (e) {
    throw new Error(
      `lite-email-parser: failed to load native binary for ${platform}-${arch}. ` +
      `Make sure "${packageName}" is installed. ` +
      `If you're using npm, ensure optionalDependencies are not disabled.\n` +
      `Original error: ${(e as Error).message}`
    );
  }
}

const addon = loadAddon();

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
