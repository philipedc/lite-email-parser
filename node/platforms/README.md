# Platform-specific packages

Each directory contains a package that ships a single `parser.node` binary for one platform/arch combination.

## Publishing

Build the binary on each target platform (or in CI), then publish:

```bash
# On Linux x64
cd linux-x64
cp ../../build/Release/parser.node .
npm publish --access public

# On macOS arm64
cd darwin-arm64
cp ../../build/Release/parser.node .
npm publish --access public

# etc.
```

## Adding a new platform

1. Create a new directory named `{os}-{arch}` (e.g. `linux-arm64`)
2. Copy an existing `package.json`, update `name`, `os`, and `cpu`
3. Build the binary on that platform and place `parser.node` in the directory
4. `npm publish --access public`
5. Add the package to `optionalDependencies` in the main `package.json`
