#!/usr/bin/env bash
# Decide whether the nightly workflow must build, and write "should_build=true|false" to $GITHUB_OUTPUT.
#  - manual runs: always;
#  - no previous nightly (missing release or assets);
#  - the AliceVision nightly changed: the plugins only load with the AliceVision libraries they
#    are built against, and the AliceVision "nightly" release only keeps its latest build;
#  - daily develop build: no nightly was built yet today (UTC), it is past 04:00 UTC (after the
#    AliceVision nightly at 02:00) and develop changed since the last nightly. A nightly built
#    earlier the same day, for instance for a new AliceVision nightly, counts as the daily build:
#    merges made after it wait for the next day.
#
# Environment: GH_TOKEN, EVENT_NAME, GITHUB_REPOSITORY, GITHUB_SHA, GITHUB_OUTPUT.
set -euo pipefail

if [ "$EVENT_NAME" != "schedule" ]; then
  echo "Manual run: building."
  echo "should_build=true" >> "$GITHUB_OUTPUT"
  exit 0
fi

# Last nightly, in a single call: first line, upload time of its assets (the publishedAt of the
# release never changes); then its release notes, listing its commit and the AliceVision archives
# it is built against. Only a missing release means "no previous nightly": any other error (API
# outage, rate limit) fails the check rather than triggering a rebuild.
GH_ERR=$(mktemp)
if ! LAST_NIGHTLY=$(gh release view nightly --repo "$GITHUB_REPOSITORY" --json assets,body \
                    --jq '([.assets[].createdAt] | min // ""), .body' 2>"$GH_ERR"); then
  if ! grep -q "release not found" "$GH_ERR"; then
    echo "::error title=Nightly check::Cannot read the last nightly: $(cat "$GH_ERR")"
    exit 1
  fi
  LAST_NIGHTLY=""
fi
LAST_BUILD=$(head -n1 <<< "$LAST_NIGHTLY")
NOTES=$(tail -n +2 <<< "$LAST_NIGHTLY")
LAST_SHA=$(grep -oE '/commit/[0-9a-f]{40}' <<< "$NOTES" | head -n1 | cut -d/ -f3 || true)
# A published nightly always lists its commit: fail rather than rebuild every hour when it does not
# (notes format changed, or a publication interrupted before the notes update: run the workflow manually)
if [ -n "$LAST_BUILD" ] && [ -z "$LAST_SHA" ]; then
  echo "::error title=Nightly check::The notes of the last nightly do not contain its commit (/commit/<sha>)."
  exit 1
fi

# Only the archives the plugins are built against. Both must be present: the AliceVision nightly
# deletes its assets before uploading the new ones, so a check during its publication sees 0 or 1.
AV_ASSETS=$(gh release view nightly --repo alicevision/AliceVision --json assets \
            --jq '.assets[].name | select(endswith("-linux.tar.gz") or endswith("-windows.zip"))')
if [ "$(grep -c . <<< "$AV_ASSETS" || true)" -ne 2 ]; then
  echo "AliceVision nightly incomplete (being published?): '${AV_ASSETS//$'\n'/ }'. Skipping, retrying next hour."
  echo "should_build=false" >> "$GITHUB_OUTPUT"
  exit 0
fi

REASON=""
if [ -z "$LAST_BUILD" ]; then
  REASON="no previous nightly"
fi
for ASSET in $AV_ASSETS; do
  if ! grep -qF "\`${ASSET}\`" <<< "$NOTES"; then
    REASON="${REASON:+$REASON, }AliceVision ${ASSET}"
  fi
done

# ISO 8601 UTC timestamps in the same format: lexicographic order is chronological order
NOW=$(date -u +%Y-%m-%dT%H:%M:%SZ)
TODAY=$(date -u +%Y-%m-%dT00:00:00Z)
CUTOFF=$(date -u +%Y-%m-%dT04:00:00Z)
if [ -n "$LAST_BUILD" ] && [[ "$LAST_BUILD" < "$TODAY" && "$CUTOFF" < "$NOW" && "$LAST_SHA" != "$GITHUB_SHA" ]]; then
  REASON="${REASON:+$REASON, }daily build of develop ${LAST_SHA:0:8} -> ${GITHUB_SHA:0:8}"
fi

echo "Last nightly: ${LAST_BUILD:-none} (${LAST_SHA:-unknown commit}), develop: ${GITHUB_SHA}"
if [ -n "$REASON" ]; then
  echo "Building: ${REASON}."
  echo "should_build=true" >> "$GITHUB_OUTPUT"
else
  echo "Nothing changed since the last nightly. Skipping."
  echo "should_build=false" >> "$GITHUB_OUTPUT"
fi
