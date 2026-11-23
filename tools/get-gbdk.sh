#!/usr/bin/env bash
# Downloads the pinned GBDK-2020 release into vendor/gbdk (not committed).
# Usage: tools/get-gbdk.sh
set -euo pipefail

# 4.6.0 (SDCC 4.6.2) drops an `add hl, hl` when indexing a pointer by y * 2 (see README), so stay on 4.5.0.
VERSION="4.5.0"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/vendor/gbdk"

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)  ASSET="gbdk-linux64.tar.gz";      SHA="d7857a5f6d135ee4c249043ca26aad9f2ec8ab5d4106d97720d404114f42605c" ;;
  Linux-aarch64) ASSET="gbdk-linux-arm64.tar.gz";  SHA="31eb2235f0fdb60163d0b1e9574a022098d6069cd56606a1daca4478a46e0439" ;;
  Darwin-arm64)  ASSET="gbdk-macos-arm64.tar.gz";  SHA="289ee60e46c5a2785a21e35533f84a5131ed4a063b21b0dbdedc9a10af15bf78" ;;
  Darwin-x86_64) ASSET="gbdk-macos.tar.gz";        SHA="1aa549d12032d8f6509d11923bb28b1a453098f42597feb378e9a42541f8fd89" ;;
  *) echo "No pinned GBDK build for $(uname -s)-$(uname -m). Get one from https://github.com/gbdk-2020/gbdk-2020/releases" >&2; exit 1 ;;
esac

if [ -x "$DEST/bin/lcc" ] && [ "$(cat "$DEST/.version" 2>/dev/null)" = "$VERSION" ]; then
  echo "GBDK $VERSION already in vendor/gbdk"; exit 0
fi

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
URL="https://github.com/gbdk-2020/gbdk-2020/releases/download/$VERSION/$ASSET"
echo "Downloading $URL"
curl -fL --retry 3 -o "$TMP/$ASSET" "$URL"
echo "$SHA  $TMP/$ASSET" | sha256sum -c -

rm -rf "$DEST"
mkdir -p "$TMP/x" "$ROOT/vendor"
tar -xzf "$TMP/$ASSET" -C "$TMP/x"
mv "$TMP/x/gbdk" "$DEST"
echo "$VERSION" > "$DEST/.version"
echo "GBDK $VERSION installed in vendor/gbdk"
