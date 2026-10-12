#!/usr/bin/env bash
#
# Creates the certificates of the smoke test in tls/: a throwaway CA, a server
# certificate for localhost and a client certificate, each with its key in a
# single .pem file.

set -euo pipefail
# Git Bash on Windows would rewrite the /CN=... subjects as paths.
export MSYS_NO_PATHCONV=1

mkdir tls && cd tls
openssl req -x509 -newkey rsa:2048 -nodes -days 2 -subj "/CN=libmongoc-windows test CA" \
  -keyout ca.key -out ca.pem \
  -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign"
issue() { # name, extended key usage, [subjectAltName]
  openssl req -newkey rsa:2048 -nodes -subj "/CN=$1" -keyout "$1.key" -out "$1.csr"
  {
    echo "basicConstraints=CA:FALSE"
    echo "keyUsage=critical,digitalSignature,keyEncipherment"
    echo "extendedKeyUsage=$2"
    if [ -n "${3:-}" ]; then echo "subjectAltName=$3"; fi
  } > "$1.ext"
  openssl x509 -req -in "$1.csr" -CA ca.pem -CAkey ca.key -CAcreateserial -days 2 -sha256 -extfile "$1.ext" -out "$1.crt"
  # PKCS#1 key: mongod on Windows reads PEM through SChannel.
  openssl pkey -in "$1.key" -traditional -out "$1.rsa.key"
  cat "$1.crt" "$1.rsa.key" > "$1.pem"
}
issue server serverAuth "DNS:localhost,IP:127.0.0.1"
issue client clientAuth
