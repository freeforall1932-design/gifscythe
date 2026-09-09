#!/usr/bin/env bash
# Compatibility wrapper for the GitHub Actions workflow.
#
# The canonical engine-build script is build_engine.sh (renamed 2026-09-09 for
# Gifscythe branding). Main's `.github/workflows/build.yml` still calls this
# old name, and the GitHub App that maintains this repo cannot edit workflow
# files (it lacks the `workflows` permission), so this shim keeps CI green
# until a maintainer updates the workflow. Remove this file once that happens.
exec "$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)/build_engine.sh" "$@"
