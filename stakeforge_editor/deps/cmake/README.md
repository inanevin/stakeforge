The official Windows x64 portable CMake distribution is vendored here as a ZIP.
`distribution.json` records its version, source URL, archive prefix and SHA-256.
Keep the archive in the repository: SDK packaging is offline and must work after
deleting `_build`.

`utils/packaging/package_sdk.py` verifies the archive and synchronizes its contents,
including the upstream licenses, into `stakeforge_sdk/tools/cmake/` beside the editor.
No CMake installer or system PATH changes are needed by the distributed editor.
