# Lite email parser

Simple library to remove signature and replies and extract attachments and inline attachments from an email.

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