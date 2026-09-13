#!/usr/bin/env bash
# Fail-closed system-dependent package; never bundles Qt runtime libraries.
set -euo pipefail
package_kind=system
source "$(dirname "${BASH_SOURCE[0]}")/package_common.sh" "$@"
