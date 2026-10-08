# Release picker

Double-click `release_gui.bat` at the repository root. It uses Windows PowerShell 5.1 and WPF. Install [GitHub CLI](https://cli.github.com/) and sign in with `gh auth login`, or use the picker's **Sign in** button. No Node installation is required locally.

Choose a repository and source branch/tag, filter by platform or search for a game/plugin ID, and check releases. **Select shown** selects the current filtered list; selections persist across filters. **Select all releases** includes every supported release. The review panel lists the packages that will be updated. Some releases bundle several plugins or architectures and are selected together.

**Publish** submits one `all.yml` workflow. CI builds and packages only the selected releases, then updates their GitHub release assets after all selected builds and package checks succeed. Signing credentials stay in GitHub Actions. **Open workflow run** follows the submitted run. If submission times out, check Actions before retrying; the picker never retries a dispatch automatically.

Commit and push the new picker/catalog/workflow files before using Publish. The workflow must be registered on the repository's default branch. A dispatch uses the selected remote branch/tag; local uncommitted code is not uploaded. Both the picker and CI check the catalog hash, including a check against the selected remote ref, so a selection cannot silently use a different release mapping. Git's LF/CRLF conversion does not affect this check.

`releases.json` maps release tags to package IDs in `tools/packaging/packages.json`. Update the catalog, `all.yml` upload entry, release description in `.github/docs`, and optionally the single-release `tag.yml` menu together when adding a release. Midnight Club: LA Remix uses `mclarpsp`; its first selected successful workflow creates the release. GTA III/VC/SA and Manhunt frontend archives require manual Magic.TXD conversion. Those archives and Condemned Missing Steam Files archives stay on their existing releases and are not rebuilt or removed.

Run the offline checks with:

```bat
powershell -NoProfile -STA -ExecutionPolicy Bypass -File tools/releases/Test.ps1
```

These checks exercise selection, workflow/catalog consistency, filtered solution generation and the GUI without dispatching or publishing a release.
