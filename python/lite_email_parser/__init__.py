"""lite_email_parser: parse an email to get its signature-stripped body,
attachments and inline images.

This is a thin Python wrapper around a native C++ extension (built from the
same core shared with the Node.js binding of this library). See the
project README for full usage documentation.
"""
from __future__ import annotations

from typing import Any, TypedDict

from . import _core

__all__ = [
    "IFile",
    "ParseEmailResult",
    "clean_html",
    "remove_signature",
    "remove_replies",
    "remove_dividers",
    "parse_email",
    "replace_src",
]


class IFile(TypedDict, total=False):
    """A file attachment or inline image extracted from an email."""

    name: str
    type: str
    size: int
    buffer: bytes
    src: str
    original_src: str


ParseEmailResult = TypedDict(
    "ParseEmailResult",
    {
        # "from" is a reserved keyword, hence the functional TypedDict form.
        "subject": str,
        "from": str,
        "to": str,
        "last_message": str,
        "attachments": list,
        "inline_attachments": list,
    },
)
"""Result of :func:`parse_email`.

Keys: ``subject``, ``from``, ``to``, ``last_message`` (HTML body with
signature/replies stripped), ``attachments`` and ``inline_attachments``
(each a ``list[IFile]``).
"""


def clean_html(html: str) -> str:
    """Strip signatures, quoted replies and dividers from an HTML body."""
    return _core.clean_html(html)


def remove_signature(html: str) -> str:
    """Remove a trailing email signature block from an HTML body."""
    return _core.remove_signature(html)


def remove_replies(html: str) -> str:
    """Remove quoted reply/forward chains from an HTML body."""
    return _core.remove_replies(html)


def remove_dividers(html: str) -> str:
    """Remove Outlook/Gmail-style horizontal dividers from an HTML body."""
    return _core.remove_dividers(html)


def parse_email(email: bytes | str) -> ParseEmailResult:
    """Parse a raw .eml message.

    Args:
        email: the raw email contents, as ``bytes`` (recommended, matches
            reading the file in binary mode) or ``str`` (encoded as UTF-8
            internally).

    Returns:
        A dict with ``subject``, ``from``, ``to``, ``last_message`` (HTML
        body with signature/replies stripped), ``attachments`` and
        ``inline_attachments`` (each a list of dicts with ``name``,
        ``type``, ``size``, ``buffer``, and for inline attachments
        optionally ``src``/``original_src``).
    """
    return _core.parse_email(email)


def replace_src(html: str, files: list[dict[str, Any]]) -> str:
    """Replace ``src`` attributes of inline images in ``html``.

    ``files`` should be a list of dicts, each with ``original_src`` (the
    ``data:`` URI originally found in the email) and ``src`` (the URL to
    swap it for, e.g. after uploading the attachment to storage).
    """
    return _core.replace_src(html, files)
