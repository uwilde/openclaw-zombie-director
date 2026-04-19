#!/usr/bin/env bash
set -euo pipefail

ENV_FILE="${1:-$(dirname "$0")/arma-reforger-server.env}"
if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$ENV_FILE"

: "${STEAMCMD_BIN:?STEAMCMD_BIN is required}"
: "${AR_SERVER_APPID:?AR_SERVER_APPID is required}"
: "${AR_SERVER_INSTALL_DIR:?AR_SERVER_INSTALL_DIR is required}"
: "${AR_SERVER_PROFILE_DIR:?AR_SERVER_PROFILE_DIR is required}"

mkdir -p "$AR_SERVER_INSTALL_DIR" "$AR_SERVER_PROFILE_DIR"

"$STEAMCMD_BIN" \
  +force_install_dir "$AR_SERVER_INSTALL_DIR" \
  +login anonymous \
  +app_update "$AR_SERVER_APPID" validate \
  +quit

echo "Arma Reforger dedicated server installed/updated at: $AR_SERVER_INSTALL_DIR"
