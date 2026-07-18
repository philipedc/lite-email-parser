import { parseEmail } from '..'; // Import from the local npm package entry
import * as fs from 'fs';
import * as path from 'path';

// Resolve path to the shared root samples folder
const emailPath = path.resolve(__dirname, '../../samples/sample1.eml');
const email = fs.readFileSync(emailPath, 'utf-8');
const buffer = Buffer.from(email, 'utf-8');

const parsedEmail = await parseEmail(buffer);
console.log(parsedEmail);
