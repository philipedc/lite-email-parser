import { parseEmail } from '../dist/index.js';
import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath } from 'url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));

// Resolve path to the shared root samples folder
const emailPath = path.resolve(__dirname, '../../samples/gmail.eml');
const email = fs.readFileSync(emailPath, 'utf-8');
const buffer = Buffer.from(email, 'utf-8');

const parsedEmail = await parseEmail(buffer);
console.log(parsedEmail);
