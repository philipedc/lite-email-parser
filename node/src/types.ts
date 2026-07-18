export interface IFile {
  name: string;
  src?: string;
  type: string;
  size: number;
  buffer: Buffer;
  originalSrc?: string;
}

export interface ParseEmailResult {
  lastMessage: string;
  attachments: IFile[];
  inlineAttachments: IFile[];
}
