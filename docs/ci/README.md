# CI workflow proposal

The GitHub App used for this branch cannot update `.github/workflows/*`
(missing `workflows` permission). The rewritten workflow therefore lives here:

- **`build.yml.proposed`** — full Linux + Windows recipe (aqtinstall, win32cfg
  engine build, smoke tests, artifacts, windeployqt).

## Apply manually

```bash
cp docs/ci/build.yml.proposed .github/workflows/build.yml
git add .github/workflows/build.yml
git commit -m "ci: apply proposed Gifscythe build workflow"
git push
```

Until applied, Actions still run the older `build.yml` from main (Windows job
known-broken: wrong Qt package name, no win32cfg, bare `./build.sh`).
