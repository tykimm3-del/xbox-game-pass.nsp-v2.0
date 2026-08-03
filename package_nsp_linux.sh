#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"

APP_NAME="Xbox"
APP_VERSION="2.0"
TITLE_ID="0100A5B0C0DE0000"
FINAL_DIR="final_nsp"
FINAL_NAME="${APP_NAME}-${APP_VERSION}-${TITLE_ID}.nsp"

PACKER=""
for candidate in ./hacbrewpack ./hacBrewPack; do
  if [[ -x "$candidate" ]]; then
    PACKER="$candidate"
    break
  fi
done

if [[ -z "$PACKER" ]]; then
  echo "ERROR: executable hacbrewpack was not found in this folder." >&2
  exit 1
fi

if [[ ! -f keys.dat ]]; then
  echo "ERROR: keys.dat was not found. Keep your own keyset local." >&2
  exit 1
fi

required=(
  exefs/main
  exefs/main.npdm
  control/control.nacp
  control/icon_AmericanEnglish.dat
  control/icon_Korean.dat
)
for file in "${required[@]}"; do
  if [[ ! -f "$file" ]]; then
    echo "ERROR: missing $file" >&2
    exit 1
  fi
done

if [[ ! -d romfs ]]; then
  echo "ERROR: missing romfs directory" >&2
  exit 1
fi

rm -rf hacbrewpack_temp hacbrewpack_nca hacbrewpack_nsp "$FINAL_DIR"
mkdir -p "$FINAL_DIR"

"$PACKER" -k keys.dat \
  --exefsdir exefs \
  --romfsdir romfs \
  --controldir control \
  --nologo \
  --tempdir hacbrewpack_temp \
  --ncadir hacbrewpack_nca \
  --nspdir hacbrewpack_nsp

mapfile -t packages < <(find hacbrewpack_nsp -maxdepth 1 -type f -name '*.nsp' -print)
if [[ ${#packages[@]} -eq 0 ]]; then
  echo "ERROR: hacBrewPack completed but no NSP file was found." >&2
  exit 1
fi

cp -f "${packages[0]}" "$FINAL_DIR/$FINAL_NAME"
echo "DONE: $FINAL_DIR/$FINAL_NAME"
echo "Do not upload keys.dat or temporary key-related files."
