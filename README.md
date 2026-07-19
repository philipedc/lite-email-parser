# Lite email parser

Simple library to remove signature and replies and extract attachments and inline attachments from an email.


At [Deer Designer](https://deerdesigner.com) we were having trouble to receive emails and filter out unnecessary content. After a careful analysis, I noticed machine learning didn't deal well with HTML mixed with attachments and embedded files. Our use case depends on enriched HTML and there weren't any good libs out there that solve this problem. So I decided to create this library.

If you stumble upon an email that is not parsed correctly, please create an issue and attach the raw .eml file.

Example of a parsed email: [sample](https://github.com/philipedc/lite-email-parser/blob/main/sample/sample.ts)

## Installation

```
npm install lite-email-parser
```

```
pip install lite-email-parser
```

## JavaScript / TypeScript usage

### Import

`const { parseEmail } = require('lite-email-parser');`

`import { parseEmail } from 'lite-email-parser';`


### Usage

#### No file upload

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

#### File upload

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

## Python usage

### Import

`from lite_email_parser import parse_email`

### Usage

```python
from lite_email_parser import parse_email

# Load your email (bytes or str accepted here)
with open('email.eml', 'rb') as f:
    email = f.read()

result = parse_email(email)

# last_message: Pure HTML content with replies/signatures stripped
print(result['last_message'])

# attachments: list of dicts (name, type, size, buffer)
print(result['attachments'])

# inline_attachments: list of dicts embedded in the email
# (also includes original_src/src once replace_src has been used)
print(result['inline_attachments'])
```

To swap inline attachment sources after uploading them (e.g. to your own
storage), use `replace_src`:

```python
from lite_email_parser import replace_src

html = replace_src(result['last_message'], [
    {'original_src': attachment['original_src'], 'src': uploaded_url}
    for attachment, uploaded_url in uploaded_inline_attachments
])
```

`clean_html`, `remove_signature`, `remove_replies` and `remove_dividers` are
also available, mirroring the Node.js API 1:1 (in snake_case). See
[python/samples/sample.py](python/samples/sample.py) for a runnable example.