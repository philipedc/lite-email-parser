# Lite email parser

Simple library to remove signature and replies and extract attachments and inline attachments from an email.


At [Deer Designer](https://deerdesigner.com) we were having trouble to receive emails and filter out unnecessary content. After a careful analysis, I noticed machine learning didn't deal well with HTML mixed with attachments and embedded files. Our use case depends on enriched HTML and there weren't any good libs out there that solve this problem. So I decided to create this library.

If you stumble upon an email that is not parsed correctly, please create an issue and attach the raw .eml file.

Example of a parsed email: [sample](https://github.com/philipedc/lite-email-parser/blob/main/sample/sample.ts)

## Import

`const { parseEmail } = require('lite-email-parser');`

`import { parseEmail } from 'lite-email-parser';`


## Usage

### No file upload

```
// Example: Simple usage without uploading attachments
const { parseEmail } = require('lite-email-parser');
const fs = require('fs');

// Load your email (string or Buffer accepted here)
const email = fs.readFileSync('email.eml');

const result = await parseEmail(email);

// lastMessage: Pure HTML content with replies/signatures stripped
console.log(result.lastMessage);

// attachments: Array of IFile objects
console.log(result.attachments);

// inlineAttachments: Array of IFile objects embedded in the email
console.log(result.inlineAttachments);
```

### File upload

```
import { parseEmail, replaceSrc } from 'lite-email-parser';
import * as fs from 'fs';

// Load your email (string or Buffer accepted here)
const email = fs.readFileSync('email.eml');

const result = await parseEmail(email);

// lastMessage: Pure HTML content with replies/signatures stripped
console.log(result.lastMessage);

// attachments: Array of IFile objects
console.log(result.attachments);

// inlineAttachments: Array of IFile objects embedded in the email
console.log(result.inlineAttachments);
```