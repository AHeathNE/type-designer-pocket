#!/usr/bin/env bash
# Downloads the pinned GBDK-2020 release into vendor/gbdk (not committed).
# Usage: tools/get-gbdk.sh
set -euo pipefail

VERSION="4.6.0"
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DEST="$ROOT/vendor/gbdk"

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64)  ASSET="gbdk-linux64.tar.gz";      SHA="c033dac026b99c080f07c2a659eb25976f65ca114cb15d1570bbb23c99c27e35" ;;
  Linux-aarch64) ASSET="gbdk-linux-arm64.tar.gz";  SHA="003bdb93b4a46702ec04cb9758be5032431deab024dff5637171183a7cea6f5b" ;;
  Darwin-arm64)  ASSET="gbdk-macos-arm64.tar.gz";  SHA="36bf2b005d5a87c7c343e0ddafb42f53578bf22e52a78f17c4aa74b80e71bc8a" ;;
  Darwin-x86_64) ASSET="gbdk-macos.tar.gz";        SHA="490748e5f100ae2421aef38196ac93c76c2af86efc81edf817bc4d0af725ad44" ;;
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
