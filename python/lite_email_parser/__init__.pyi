from typing import Any, TypedDict

class IFile(TypedDict, total=False):
    name: str
    type: str
    size: int
    buffer: bytes
    src: str
    original_src: str

ParseEmailResult = TypedDict(
    "ParseEmailResult",
    {
        "subject": str,
        "from": str,
        "to": str,
        "last_message": str,
        "attachments": list[IFile],
        "inline_attachments": list[IFile],
    },
)

def clean_html(html: str) -> str: ...
def remove_signature(html: str) -> str: ...
def remove_replies(html: str) -> str: ...
def remove_dividers(html: str) -> str: ...
def parse_email(email: bytes | str) -> ParseEmailResult: ...
def replace_src(html: str, files: list[dict[str, Any]]) -> str: ...
