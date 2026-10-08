# Release packaging

`packages.json` is the packaging contract. Source paths are relative to the
repository and destinations are relative to the ZIP root. `packages.schema.json`
provides editor completion. The PowerShell packager rejects unknown fields,
duplicate destinations, escaping paths, missing/empty inputs, ambiguous archive
members, incorrect architectures, mismatched symbols, and tool failures.

Each package lists build producers, asset files, and dependency extraction rules.
FarCry requires both architectures; MaxPayne also requires its MSVCP60 wrapper.
Selected releases automatically build every producer their packages need. A new
solution project without a manifest entry fails rather than being omitted.
Inactive data directories without build projects are not released.

## Local commands

Packaging and orchestration use PowerShell 5.1 or newer. No Node.js, npm, Python,
or additional PowerShell modules are needed locally. Native builds still require
Visual Studio/MSBuild. The repository's 7za.exe and EmbedPDB.exe are used on Windows.
PowerShell 7 can run the packaging code on other systems with a 7z installation;
native building, PDB embedding, and Authenticode signing still require Windows tools.

Run `premake5.bat` and `before_build.bat` first. The latter prepares texture assets.

From a Visual Studio Developer shell, `build_release.bat` builds all Release
solutions and packages them as they finish, with signing off. It forwards extra
arguments such as `-Packages GTAVCDE.FusionFix` or `-NoDownload` to the PowerShell
coordinator and preserves its exit code.

```powershell
powershell -NoProfile -File tools/packaging/Release.ps1 -Mode after-build
powershell -NoProfile -File tools/packaging/Release.ps1 -Mode as-ready
powershell -NoProfile -File tools/packaging/Release.ps1 -Packages GTAVCDE.FusionFix -Signing off
powershell -NoProfile -File tools/packaging/Release.ps1 -Existing -Packages FarCry.WidescreenFix -Signing off
```

`-NoDownload` uses previously downloaded dependency archives. `-Existing` packages
outputs already built successfully and does not assert their commit/configuration.
`-Existing -Available` selects only packages with all producer outputs present;
AppVeyor uses it for its partial build. Required asset files still fail if missing.
GitHub builds use explicit build plans and completion records.

`before_packaging.bat` and `data/release.bat` are compatibility launchers for
`Download.ps1` and `Release.ps1 -Existing`. Their build/texture prerequisites apply.

## Scheduling and publication

`after-build` waits for every selected build to succeed before packaging.
`as-ready` packages once all producers of an individual package finish successfully,
including up-to-date builds. MSBuild imports `completion.targets` only for these
coordinated builds and atomically publishes records in a unique invocation folder.
Output existence alone never signals readiness. Two PowerShell runspaces package
concurrently, without spawning a PowerShell process for each package.

Master push nightlies use `as-ready`, require signing, and build Win64 first for
earlier artifacts. Other push/PR builds use `after-build` with signing off. Manual
public releases use `after-build` and require signing; public publishing and tag
updates remain gated on success.

GitHub's artifact API requires its JavaScript client. `upload.cjs` is a small,
GitHub-only adapter: it launches PowerShell and uploads atomically published ZIP
records, with four concurrent uploads. Its pinned npm dependency is installed only
in GitHub Actions. Local packaging does not load or invoke JavaScript.

Complete artifacts uploaded early remain available if a later build fails. The
run stays failed and public publishing does not proceed. Upload errors fail the
coordinator; started packaging and uploads settle before exit.

## Integrity and symbols

Inputs are copied into private staging folders. PDB embedding and signing affect
only staged binaries. Signing profiles are `required`, `optional` (when credentials
exist), and `off`. Both signing and Authenticode verification must succeed when
signing is enabled. Third-party binaries retain their upstream signatures.

`embedPdb: "if-present"` embeds when the binary has a CodeView PDB reference;
`"required"` also requires that reference. PDB GUID and age must match. Embedded
bytes are decompressed and compared before and after signing. A stale Debug PDB
beside a Release binary without a CodeView reference is ignored. Build symbol
settings are unchanged.

ZIPs are written privately, tested, extracted, and checked against every staged
file's SHA-256 before their final names appear under `data/Archives`. Uploads
consume only these completed archives. Selected old archives are removed before
work starts, so failures cannot substitute an earlier ZIP.

Dependency repositories, tags, asset patterns, nested archives, members, and
renames are defined in JSON. Downloads are checked for size, available upstream
digest, and archive integrity. Extraction must match exactly one member. No Actions
cache is used. Resolution hashes and package summaries are recorded under
`build/packaging/<id>`. The three externally supplied frontend texture directories
remain optional, following the corresponding plugin package's readiness.

## Manual UnrealScript compilation

Routine builds package the checked-in Splinter Cell `.u` files. After changing
UnrealScript sources, set `SCCOMPILE_PASSWORD` and manually run:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File tools/compile-splintercell-scripts.ps1
```

The helper uses a fresh build directory, checks compilation and all three outputs,
and updates EchelonHUD.u, EchelonIngredient.u, and UWindow.u. Review and commit
these generated files alongside the source changes. Routine CI receives no
sccompile password and does not invoke this helper.

## Verification

```powershell
powershell -NoProfile -File tools/packaging/Test.ps1
```

The tests cover manifest migration, real ZIP integrity, unchanged build outputs,
missing inputs, path/collision checks, symbols, signing failures, and the manual
compiler guard. Native and console completion flows are also exercised locally.
