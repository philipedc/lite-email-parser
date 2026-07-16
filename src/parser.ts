import { simpleParser, Attachment } from 'mailparser';
import * as cheerio from 'cheerio';

import { IFile, ParseEmailResult } from './types.js';

export async function parseEmail(
  buffer: Buffer | string,
): Promise<ParseEmailResult> {
  const parsedEmail = await simpleParser(buffer);

  let content = parsedEmail.html;
  if (!content) {
    content = parsedEmail.text || '';
  }

  const cleanedContent = extractFirstMessage(content);

  const attachments: IFile[] = [];
  const inlineAttachments: IFile[] = [];

  for (const att of parsedEmail.attachments || []) {
    const filename = att.filename || new Date().toDateString();
    const contentType = att.contentType || 'application/octet-stream';

    const file: IFile = {
      name: filename,
      type: contentType,
      size: att.size,
      buffer: att.content,
    };

    if (att.related) {
      const base64Data = att.content.toString('base64');
      file.originalSrc = `data:${contentType};base64,${base64Data}`;
      inlineAttachments.push(file);
    } else {
      attachments.push(file);
    }
  }

  return {
    lastMessage: cleanedContent,
    attachments,
    inlineAttachments,
  };
}



// Replace src attributes with the uploaded file URLs
export function replaceSrc(html: string, files: IFile[]): string {
  const $ = cheerio.load(html);


  for (const file of files) {
    if (file.originalSrc && file.src) {
      $(`img[src="${file.originalSrc}"]`).attr('src', file.src);
    }
  }

  return $.html();
}

function extractFirstMessage(html: string): string {
  const $ = cheerio.load(html);

  // Remove text dividers (e.g. ----- Original Message -----) and everything after them
  let dividerNode: any = null;
  let isTextNode = false;

  function findDividerNode(parent: any): boolean {
    const contents = $(parent).contents();
    for (let i = 0; i < contents.length; i++) {
      const node = contents[i];
      if (node.type === 'text') {
        const text = $(node).text();
        const match = text.match(/-{5,}.*?-{5,}/);
        if (match) {
          const index = text.indexOf(match[0]);
          node.data = text.substring(0, index);
          dividerNode = node;
          isTextNode = true;
          return true;
        }
      } else if (node.type === 'tag') {
        if (node.name !== 'style' && node.name !== 'script') {
          const clone = $(node).clone();
          clone.find('style, script').remove();
          if (/-{5,}.*?-{5,}/.test(clone.text())) {
            const foundInChild = findDividerNode(node);
            if (!foundInChild) {
              dividerNode = node;
              isTextNode = false;
            }
            return true;
          }
        }
      }
    }
    return false;
  }

  findDividerNode($('body'));

  if (dividerNode) {
    let current = $(dividerNode);
    let isFirst = true;
    while (current.length > 0 && !current.is('body')) {
      current.nextAll().remove();
      const parent = current.parent();
      if (isFirst) {
        if (!isTextNode) {
          current.remove();
        }
        isFirst = false;
      }
      current = parent;
    }
  }

  // Gmail signature
  $('span.gmail_signature_prefix').remove();
  $('div.gmail_signature').remove();

  // Gmail quoted replies
  $('.gmail_chip').remove();
  $('.gmail_quote').nextAll().remove();
  $('.gmail_quote').remove();

  // Outlook signature / forwarded messages
  $('#divRplyFwdMsg').nextAll().remove();
  $('#divRplyFwdMsg').remove();
  // Outlook signature marker: <div id="Signature">
  $('div#Signature').remove();

  // Apple Mail signature
  // Apple Mail wraps signatures in <div dir="ltr" class="AppleMailSignature">
  $('div.AppleMailSignature').remove();

  // Apple Mail quoted replies
  // Apple Mail wraps quoted content in <blockquote type="cite">
  $('blockquote[type="cite"]').nextAll().remove();
  $('blockquote[type="cite"]').remove();

  // Fallback for other email providers
  // Matches any div whose id or class contains "signature" (case-insensitive)
  $('div')
    .filter((_, el) => {
      const id = ($(el).attr('id') || '').toLowerCase();
      const cls = ($(el).attr('class') || '').toLowerCase();
      return id.includes('signature') || cls.includes('signature');
    })
    .remove();

  // Fallback for quoted replies from other email providers
  // Matches any div whose id or class contains "quote" or "reply" (case-insensitive)
  $('div')
    .filter((_, el) => {
      const id = ($(el).attr('id') || '').toLowerCase();
      const cls = ($(el).attr('class') || '').toLowerCase();
      return (
        id.includes('quote') ||
        id.includes('reply') ||
        cls.includes('quote') ||
        cls.includes('reply')
      );
    })
    .each((_, el) => {
      $(el).nextAll().remove();
      $(el).remove();
    });

  return $('body').html() || '';
}
