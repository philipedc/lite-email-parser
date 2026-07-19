"""Build script for the lite-email-parser Python extension.

The core parsing logic lives in ../core (pure C++, no Python or Node
dependencies) and depends on the vendored Gumbo HTML5 parser in
../deps/gumbo-parser. Both are shared with the Node.js binding
(../node/binding.gyp) so any behavior fix applies to every language binding.

Since a PyPI sdist must be buildable on its own (without the rest of the
monorepo present), we vendor copies of those two directories into
python/_vendor/ at build time. When building from within the monorepo
checkout (local dev, CI, cibuildwheel) the vendored copy is refreshed from
the source of truth. When building from an already-vendored sdist (e.g. pip
installing from source on a platform without a prebuilt wheel), the existing
python/_vendor/ copy is used as-is.
"""
import shutil
from pathlib import Path

from pybind11.setup_helpers import Pybind11Extension, build_ext
from setuptools import setup

HERE = Path(__file__).parent.resolve()
REPO_ROOT = HERE.parent
VENDOR_DIR = HERE / "_vendor"

CORE_SOURCE = REPO_ROOT / "core"
GUMBO_SOURCE = REPO_ROOT / "deps" / "gumbo-parser"


def _vendor_sources() -> tuple[Path, Path]:
    vendor_core = VENDOR_DIR / "core"
    vendor_gumbo = VENDOR_DIR / "gumbo-parser"

    if CORE_SOURCE.exists() and GUMBO_SOURCE.exists():
        # Building inside the monorepo: refresh the vendored copy so the
        # sdist we produce stays in sync and is self-contained.
        for src, dst in ((CORE_SOURCE, vendor_core), (GUMBO_SOURCE, vendor_gumbo)):
            if dst.exists():
                shutil.rmtree(dst)
            shutil.copytree(src, dst)
    elif not (vendor_core.exists() and vendor_gumbo.exists()):
        raise RuntimeError(
            "lite-email-parser: could not find core/ and deps/gumbo-parser/ "
            "sources. Build either from within the monorepo checkout, or "
            "from a self-contained sdist that already vendors python/_vendor/."
        )

    return vendor_core, vendor_gumbo


vendor_core, vendor_gumbo = _vendor_sources()


def _relative(path: Path) -> str:
    """setuptools requires sdist/Extension sources as relative,
    forward-slash paths from the setup.py directory."""
    return path.relative_to(HERE).as_posix()


gumbo_include_dirs = [
    _relative(vendor_gumbo / "src"),
    _relative(vendor_gumbo / "compat"),
]

# Gumbo's sources are plain C (C99), compiled as a separate static library.
# They must NOT be compiled with `-std=c++17` (see below), so they can't be
# passed directly as `Pybind11Extension` sources alongside the C++ files.
gumbo_library = (
    "gumbo",
    {
        "sources": sorted(
            _relative(p) for p in (vendor_gumbo / "src").glob("*.c")
        ),
        "include_dirs": gumbo_include_dirs,
        # Match core/Makefile's C flags. Crucially, this does NOT include
        # `-std=c++17`: Pybind11Extension's `cxx_std` applies that flag to
        # every source file in an extension, which GCC only warns about for
        # .c files but Apple Clang treats as a hard, build-breaking error.
        "cflags": ["-std=c99"],
    },
)

ext_modules = [
    Pybind11Extension(
        "lite_email_parser._core",
        sources=sorted(
            _relative(p)
            for p in [
                HERE / "src" / "python_bindings.cpp",
                vendor_core / "src" / "html_parser.cpp",
                vendor_core / "src" / "html_extractor.cpp",
            ]
        ),
        include_dirs=[
            _relative(vendor_core / "include"),
            *gumbo_include_dirs,
        ],
        libraries=["gumbo"],
        cxx_std=17,
    ),
]

setup(
    libraries=[gumbo_library],
    ext_modules=ext_modules,
    cmdclass={"build_ext": build_ext},
)
