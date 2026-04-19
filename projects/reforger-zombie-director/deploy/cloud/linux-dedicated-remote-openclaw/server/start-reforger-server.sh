#!/usr/bin/env bash
set -euo pipefail

ENV_FILE="${1:-$(dirname "$0")/arma-reforger-server.env}"
if [[ ! -f "$ENV_FILE" ]]; then
  echo "Environment file not found: $ENV_FILE" >&2
  exit 1
fi

# shellcheck disable=SC1090
source "$ENV_FILE"

: "${AR_SERVER_INSTALL_DIR:?AR_SERVER_INSTALL_DIR is required}"
: "${AR_SERVER_PROFILE_DIR:?AR_SERVER_PROFILE_DIR is required}"
: "${AR_SERVER_CONFIG_PATH:?AR_SERVER_CONFIG_PATH is required}"

AR_SERVER_BIN="${AR_SERVER_BIN:-ArmaReforgerServer}"
AR_SERVER_ARGS="${AR_SERVER_ARGS:-}"

mkdir -p "$AR_SERVER_PROFILE_DIR"

cd "$AR_SERVER_INSTALL_DIR"
exec "./$AR_SERVER_BIN" -config "$AR_SERVER_CONFIG_PATH" -profile "$AR_SERVER_PROFILE_DIR" $AR_SERVER_ARGS
