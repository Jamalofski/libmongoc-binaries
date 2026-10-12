#!/usr/bin/env bash
#
# Installs Ubuntu packages as root, on the runner or in a container:
#   apt-install.sh <package>...
#
# apt-get update once hung for 16 minutes on a runner, without a word, most
# likely on a mirror that did not answer. Each attempt gets a time limit here,
# there are three of them, and apt-get says what it downloads.

set -euo pipefail
export DEBIAN_FRONTEND=noninteractive

echo "::group::apt-get install $*"
attempt=1
until timeout 120 apt-get update -q &&
      timeout 180 apt-get install -y -q --no-install-recommends "$@"; do
  if [ "$attempt" -ge 3 ]; then
    echo "::endgroup::"
    echo "::error::apt-get failed or timed out 3 times"
    exit 1
  fi
  echo "::warning::apt-get failed or timed out, attempt $attempt of 3"
  attempt=$((attempt + 1))
  sleep 20
done
echo "::endgroup::"
