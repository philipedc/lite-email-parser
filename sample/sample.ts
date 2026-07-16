import { parseEmail, replaceSrc } from 'lite-email-parser';
import * as fs from 'fs';

const email = fs.readFileSync('email.eml', 'utf-8');
const buffer = Buffer.from(email, 'utf-8');

const { lastMessage, attachments, inlineAttachments } = await parseEmail(buffer);

console.log('--- Initial Parsed Output ---');
console.log('attachments count:', attachments.length);
console.log('inlineAttachments count:', inlineAttachments.length);

// Consumer-side upload simulation
console.log('\n--- Simulating Consumer Upload ---');
const uploadedAttachments = [];
const uploadedInline = [];

for (const file of attachments) {
  const url = await uploadToS3('my-bucket', file.name, file.buffer);
  uploadedAttachments.push({ ...file, src: url });
}

for (const file of inlineAttachments) {
  const url = await uploadToS3('my-bucket', file.name, file.buffer);
  uploadedInline.push({ ...file, src: url });
}

// Replace sources in the clean HTML using the utility function
const finalHtml = replaceSrc(lastMessage, uploadedInline);

console.log('\n--- Final Replaced HTML ---');
console.log(finalHtml);

async function uploadToS3(
  bucket: string,
  filename: string,
  content: Buffer | string,
): Promise<string> {
  console.log(`uploading ${filename} to bucket ${bucket}...`);
  await new Promise((resolve) => setTimeout(resolve, 100));
  console.log(`uploaded ${filename}`);
  return `https://s3.example.com/${bucket}/attachments/${filename}`;
}
