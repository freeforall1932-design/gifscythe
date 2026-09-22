#!/usr/bin/env bash
# Fail-closed portable stager; explicit headless and Windows targets supported.
set -euo pipefail
package_kind=portable
source "$(dirname "${BASH_SOURCE[0]}")/package_common.sh" "$@"
