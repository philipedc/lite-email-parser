import os
from lite_email_parser import parse_email

# Resolve path to the shared root samples folder
current_dir = os.path.dirname(os.path.abspath(__file__))
email_path = os.path.join(current_dir, '../../samples/sample1.eml')

with open(email_path, 'r', encoding='utf-8') as f:
    email_content = f.read()

result = parse_email(email_content)
print(result)
