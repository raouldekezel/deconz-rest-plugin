# Fork Workflow

This repository is a personal fork of [dresden-elektronik/deconz-rest-plugin](https://github.com/dresden-elektronik/deconz-rest-plugin), the open-source REST API plugin of deCONZ. It carries local changes that are not (or not yet) available upstream, while staying easy to move to each new upstream release.

The first and main local change is a read-only **Zigbee network topology endpoint** (`GET /api/<apikey>/topology`: neighbor links with their LQI), taken from upstream PR [#8670](https://github.com/dresden-elektronik/deconz-rest-plugin/pull/8670). It lets tools outside deCONZ (dashboards, monitoring) draw the mesh without the GUI. Once upstream ships an equivalent endpoint in a release, the local patch is dropped and this fork can be retired.

## Branch model

| Branch | Role | Rules |
|--------|------|-------|
| `master` | Pristine mirror of upstream `master` | Never commit here directly. Updated exclusively by syncing from upstream. |
| `deploy` | **The upstream release tag this fork tracks** + local pull requests | The branch that is built and deployed. All local work lands here through pull requests. |

`deploy` is **not** based on upstream `master` but on a **release tag** (currently `v2.33.2`). The plugin is loaded by the proprietary deCONZ core of that exact release; building it from `master` would mix unreleased changes with an older core.

`deploy` is the default branch for pull-request ergonomics: new PRs target it by default instead of GitHub proposing the upstream repository.

## Development process

**Language policy:** all repository communication — issues, pull requests, commit messages, and documentation — is written in English.

### Issues

- **Every change starts as an issue**, with a typed identifier in the title, numbered per family and local to this repository: `BUG-NN`, `FEAT-NN`, `HARD-NN`, `CHORE-NN`, `SPIKE-NN`.
- **Titles and problem statements take the user's perspective**: the title describes the problem as the user experiences it, never the mechanism or the fix. Chores and spikes describe their goal instead.
- **The issue body is the normative source of truth.** Settled design and decisions are folded into the body in place, with a dated edit trailer. Comments carry only dated session reports and reviews.
- **An issue is closed only by the maintainer, and only after validation on a live Zigbee network.** A merge never closes an issue.

### Branches, pull requests, merges

- Work branches are named `patches/<id>-<slug>` and fork off `deploy`.
- One pull request per issue, targeting `deploy` **in this fork** (GitHub tends to preselect the upstream repository: double-check the base).
- Reference issues with `refs #NN` — never a closing keyword, even in prose: with `deploy` as the default branch, a merged closing keyword would auto-close the issue.
- Merge only on the maintainer's explicit approval. Work PRs are **squash-merged**, the only merge method enabled here. A change taken from upstream keeps its original author (`git cherry-pick` preserves it).

## Build and distribution

There is no package registry: the plugin is **built from source** and mounted into a deCONZ container.

- The build runs in a disposable container matching the target deCONZ image (Debian 12, Qt 5.15 for 2.33.2). The deCONZ library headers come from [dresden-elektronik/deconz-lib](https://github.com/dresden-elektronik/deconz-lib), fetched by CMake. Nothing is installed on the host.
- The result, `libde_rest_plugin.so`, is mounted read-only over `/usr/share/deCONZ/plugins/libde_rest_plugin.so`. Removing the mount and restarting deCONZ restores the stock plugin.
- Each deployed, validated state is published as a **GitHub pre-release** tagged `v<upstream-version>-raoul.N` (base = the upstream release `deploy` tracks, `N` monotonic per base), with the built `.so` attached and its SHA-256 in the notes.

## Moving to a new upstream release

1. Update `master` from upstream (**Sync fork** button, or `gh repo sync raouldekezel/deconz-rest-plugin -b master`).
2. Merge the new release tag into `deploy` **as a local merge pushed directly to `deploy`**, never through a pull request (a squash would collapse upstream history and re-conflict at every move):

   ```
   git fetch origin --tags
   git checkout deploy
   git merge v<new-version>
   git push origin deploy
   ```

3. Resolve conflicts in favor of the local changes — unless the new release ships the same feature, in which case drop the local patch.
4. Rebuild against the matching deCONZ release, redeploy, validate, cut `v<new-version>-raoul.1`.

## Contributing back upstream

Upstream contributions are made from a branch created **from `master`**, with only the relevant commits cherry-picked. For the topology endpoint, the upstream vehicle is PR #8670 itself: test feedback is posted there.

## Continuous integration

GitHub disables Actions on new forks; the upstream workflows (DDF bundles, button maps, release drafter, stale bot) are not needed here and stay disabled. Upstream workflow files are kept identical so that syncing never conflicts on them.

## Note on this file

`FORK_WORKFLOW.md` and the build tooling exist only on `deploy` (and branches derived from it), keeping `master` byte-identical to upstream.
