"""Publishes the settings description inside each packaged plugin to fusionfix.io, authenticated by the run's
GitHub Actions OIDC token (permissions: id-token: write).

    python tools/releases/publish-settings.py --channel release|dev [--tags gta3,gtavc|all]

Reads the zips under data/Archives, the release tag of each package from tools/releases/releases.json. A PC plugin's
description is its ASISETTINGS resource (translations: ASISETTINGS_TEXT); an emulator plugin's are the
<id>.asisettings.json / <id>.asisettings.<lang>.json files next to it in the zip.
"""
import argparse, hashlib, json, os, subprocess, sys, urllib.error, urllib.request, zipfile

root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))


def tag_case(name):
    # rc.exe upper-cases resource names: EN -> en, PT-BR -> pt-BR
    parts = name.split("-")
    return "-".join([parts[0].lower()] + [p.upper() if len(p) == 2 else p.title() if len(p) == 4 else p for p in parts[1:]])


def from_pe(data):
    import pefile
    pe = pefile.PE(data=data)

    def resources(type_name):
        out = {}
        directory = getattr(pe, "DIRECTORY_ENTRY_RESOURCE", None)
        for t in directory.entries if directory else []:
            if t.name is None or str(t.name) != type_name:
                continue
            for n in t.directory.entries:
                for lang in n.directory.entries:
                    s = lang.data.struct
                    out[str(n.name) if n.name is not None else str(n.id)] = pe.get_data(s.OffsetToData, s.Size).decode("utf-8-sig")
        return out

    descriptions = resources("ASISETTINGS")
    if not descriptions:
        return None
    info = {}
    for fi in getattr(pe, "FileInfo", []):
        for entry in fi:
            for st in getattr(entry, "StringTable", []):
                info.update({k.decode(): v.decode() for k, v in st.entries.items()})
    return {
        "description": json.loads(next(iter(descriptions.values()))),
        "translations": {tag_case(k): json.loads(v) for k, v in resources("ASISETTINGS_TEXT").items()},
        "version": info.get("FileVersion", "").strip(),
        "buildTimeUtc": info.get("BuildTimeUtc", "").strip(),
    }


def commit_time():
    # Emulator plugins have no version resource: the build's commit time stands for the build time
    out = subprocess.run(["git", "-C", root, "log", "-1", "--format=%cd", "--date=format-local:%Y-%m-%dT%H:%M:%SZ"],
                         capture_output=True, text=True, check=True, env={**os.environ, "TZ": "UTC"})
    return out.stdout.strip()


def plugins_in(zip_path):
    with zipfile.ZipFile(zip_path) as z:
        names = z.namelist()
        for name in names:
            base = name.rsplit("/", 1)[-1]
            if base.lower().endswith(".asi"):
                found = from_pe(z.read(name))
                if found:
                    yield base[:-4], found
            elif base.endswith(".asisettings.json"):
                fix = base[: -len(".asisettings.json")]
                folder = name[: len(name) - len(base)]
                translations = {}
                for other in names:
                    prefix = folder + fix + ".asisettings."
                    if other.startswith(prefix) and other.endswith(".json") and other != name:
                        translations[tag_case(other[len(prefix):-5])] = json.loads(z.read(other).decode("utf-8-sig"))
                yield fix, {"description": json.loads(z.read(name).decode("utf-8-sig")), "translations": translations,
                            "version": "", "buildTimeUtc": commit_time()}


def oidc_token():
    request = urllib.request.Request(os.environ["ACTIONS_ID_TOKEN_REQUEST_URL"] + "&audience=fusionfix.io",
                                     headers={"Authorization": "bearer " + os.environ["ACTIONS_ID_TOKEN_REQUEST_TOKEN"]})
    return json.load(urllib.request.urlopen(request))["value"]


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--channel", choices=["release", "dev"], required=True)
    parser.add_argument("--tags", default="all")
    args = parser.parse_args()
    catalog = json.load(open(os.path.join(root, "tools/releases/releases.json"), encoding="utf-8"))
    wanted = None if args.tags in ("", "all") else set(args.tags.split(","))
    token = oidc_token()
    failed = 0
    for release in catalog["releases"]:
        if wanted is not None and release["tag"] not in wanted:
            continue
        for package in release["packages"]:
            zip_path = os.path.join(root, "data", "Archives", package + ".zip")
            if not os.path.isfile(zip_path):
                continue
            data = open(zip_path, "rb").read()
            for fix, found in plugins_in(zip_path):
                body = {
                    "fix": fix, "channel": args.channel, "version": found["version"] or release["tag"],
                    "buildTimeUtc": found["buildTimeUtc"], "sha": os.environ["GITHUB_SHA"],
                    "asset": {"name": os.path.basename(zip_path), "sha256": hashlib.sha256(data).hexdigest(), "size": len(data)},
                    "settings": {"format": 1, "description": found["description"], "translations": found["translations"]},
                }
                if args.channel == "release":
                    body["tag"] = release["tag"]
                if not body["buildTimeUtc"]:
                    print(f"{fix}: no BuildTimeUtc in the plugin's version resource", file=sys.stderr)
                    failed += 1
                    continue
                request = urllib.request.Request("https://fusionfix.io/api/publish/settings", data=json.dumps(body).encode(), method="POST",
                                                 headers={"Authorization": "Bearer " + token, "Content-Type": "application/json", "User-Agent": "FusionFix-CI"})
                try:
                    print(f"{fix}: {urllib.request.urlopen(request).read().decode()}")
                except urllib.error.HTTPError as e:
                    print(f"{fix}: {e.code} {e.read().decode()}", file=sys.stderr)
                    failed += 1
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
