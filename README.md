# libmongoc-windows

Prebuilt binaries of the [MongoDB C Driver](https://github.com/mongodb/mongo-c-driver) (`libmongoc` and `libbson`), 1.x and 2.x: Windows DLLs for x64 and x86, built with MSVC and OpenSSL 3.5 LTS, and Linux shared libraries for x64.

MongoDB publishes the C driver as source code only. This repository builds each upstream release on GitHub Actions from the signed tarball, tests it against a real `mongod` over TLS, and publishes the result as release assets.

This is an unofficial build, not affiliated with or endorsed by MongoDB, Inc.

## Which branch

| Releases | Windows | Linux | For |
| --- | --- | --- | --- |
| `v2.*` (marked Latest) | `mongoc2.dll`, `bson2.dll` | `libmongoc2.so.2`, `libbson2.so.2` | New code: 2.x is the current upstream branch |
| `v1.*` | `libmongoc-1.0.dll`, `libbson-1.0.dll` | `libmongoc-1.0.so.0`, `libbson-1.0.so.0` | Delphi FireDAC, and applications written for the 1.x API |

The 1.x DLLs keep the `lib`-prefixed names that FireDAC looks for. Upstream dropped that prefix in 1.16, so a default 1.x build does not work with FireDAC. FireDAC cannot use 2.x, whose DLL names and API differ.

## Download

Get the archive for your branch and platform from the [releases](https://github.com/jamalofski/libmongoc-windows/releases):

| File | For |
| --- | --- |
| `libmongoc-windows-<version>-x64.zip` | 64-bit Windows applications |
| `libmongoc-windows-<version>-x86.zip` | 32-bit Windows applications, including the 32-bit Delphi IDE |
| `libmongoc-linux-<version>-x64.tar.gz` | 64-bit Linux applications, on Ubuntu 24.04 and later |

## Windows

Each zip contains:

```
bin/        the two driver DLLs (see above)
            libssl-3-x64.dll, libcrypto-3-x64.dll   (x86: libssl-3.dll, libcrypto-3.dll)
            vcruntime140.dll, the Microsoft Visual C++ runtime
include/    libbson and libmongoc headers
lib/        import libraries (1.x: libmongoc-1.0.lib, libbson-1.0.lib; 2.x: mongoc2.dll.lib, bson2.dll.lib),
            CMake and pkg-config files
licenses/   mongo-c-driver and its bundled components, OpenSSL
```

Copy the DLLs from `bin/` next to your executable. The driver DLL imports the OpenSSL DLLs when it loads, so they are needed even if you never use TLS, and they must be OpenSSL 3.5 or later. If your application already ships OpenSSL 3.5 or later under the same file names, you can keep yours.

`vcruntime140.dll` is the only thing the DLLs need that Windows 10 and 11 do not include. It normally comes with the [Microsoft Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist), and the copy in the package makes the DLLs load on machines that do not have it. Every Visual Studio since 2015 uses this file name, and a newer runtime runs what an older compiler built: leave the file out if you install the redistributable (14.44 or later) with your application, and do not replace a newer `vcruntime140.dll` with it.

For C and C++ projects, add the two directories under `include/` to the include path and link the two import libraries from `lib/`, or add the extracted folder to `CMAKE_PREFIX_PATH`.

## Linux

Distributions package the driver, but one branch at a time: Ubuntu 24.04 has 1.26, and Ubuntu 26.04 has 2.2 and no 1.x any more. The archive brings the current release of either branch to both:

```
lib/        libmongoc-1.0.so.0, libbson-1.0.so.0   (2.x: libmongoc2.so.2, libbson2.so.2) and their unversioned links,
            CMake and pkg-config files
include/    libbson and libmongoc headers
licenses/   mongo-c-driver and its bundled components
```

Unlike the Windows packages, it contains no OpenSSL: the libraries use the OpenSSL 3, Cyrus SASL and zlib of the distribution, which keeps them on its security updates. Most installations already have them:

```sh
sudo apt install libssl3t64 libsasl2-2 ca-certificates
```

The libraries are built on Ubuntu 24.04 and tested on Ubuntu 24.04 and 26.04. They are not tested on other distributions, where they need the glibc of Ubuntu 24.04 (2.39) or a later one, `libssl.so.3` and `libsasl2.so.2`.

Extract the archive anywhere. libmongoc looks for libbson in its own directory, so an application that loads libmongoc by its full path needs nothing else. To let every application find the libraries by name, register the directory with the dynamic linker:

```sh
sudo tar -xzf libmongoc-linux-<version>-x64.tar.gz -C /opt
echo /opt/libmongoc-linux-<version>-x64/lib | sudo tee /etc/ld.so.conf.d/libmongoc.conf
sudo ldconfig
```

The 1.x and 2.x libraries have different names, so one branch can be installed next to the other, including the one the distribution packages.

For C and C++ projects, add the extracted folder to `CMAKE_PREFIX_PATH`.

## Verify a download

Each release comes with a `SHA256SUMS` file and a [build provenance attestation](https://docs.github.com/en/actions/security-for-github-actions/using-artifact-attestations/using-artifact-attestations-to-establish-provenance-for-builds) for every archive, DLL and shared library, which proves the file was produced by this repository's workflow:

```sh
sha256sum -c SHA256SUMS
gh attestation verify libmongoc-windows-<version>-x64.zip --repo jamalofski/libmongoc-windows
gh attestation verify mongoc2.dll --repo jamalofski/libmongoc-windows
gh attestation verify libmongoc-1.0.so.0 --repo jamalofski/libmongoc-windows
```

## How it is built

[`build.yml`](.github/workflows/build.yml) builds the Windows DLLs on a GitHub-hosted `windows-2025` runner. The runner has Visual Studio 2026, but the DLLs are compiled with the MSVC 14.44 toolset from Visual Studio 2022 17.14, which it installs side by side:

1. It downloads the mongo-c-driver release and checks its signature against the [MongoDB C Driver release key](keys/mongo-c-driver.asc) (`6DB5 5D82 23FF 44E4 9DCB 9813 44E7 6C05 65AB C463`).
2. It builds OpenSSL from the latest 3.5 LTS release, after checking the tarball signature against the [OpenSSL signing certificate](keys/openssl.asc) (`B146 647E 45A7 B339 47AB 226B 2A2C 87D1 6169 2D40`), with OpenSSL's default Windows directories.
3. It builds libbson and libmongoc with `ENABLE_SSL=OPENSSL`, `ENABLE_SASL=SSPI`, `ENABLE_SRV=ON`, `ENABLE_ZLIB=BUNDLED`, `ENABLE_MONGODB_AWS_AUTH=ON`, `ENABLE_SNAPPY=OFF`, `ENABLE_ZSTD=OFF` and `ENABLE_CLIENT_SIDE_ENCRYPTION=OFF`. On 1.x it adds `BSON_OUTPUT_BASENAME=libbson` and `MONGOC_OUTPUT_BASENAME=libmongoc`.
4. It adds `vcruntime140.dll` from the runner's Visual Studio, then checks the architecture and version of every DLL, and that each dependency is either in the package or part of Windows.
5. It compiles a [smoke test](test/smoke.c) and runs it with only the package and Windows on the DLL search path, against a local `mongod` that requires TLS and a client certificate.

The same workflow builds the Linux libraries on an `ubuntu-24.04` runner, from the same verified tarball and with the same options, except `ENABLE_SASL=CYRUS` and `ENABLE_ZLIB=SYSTEM`. They link to the OpenSSL of the system, and libmongoc gets an `$ORIGIN` run path. The workflow checks that they depend on nothing but glibc, OpenSSL 3, Cyrus SASL and zlib, then runs the smoke test in fresh `ubuntu:24.04` and `ubuntu:26.04` containers that only have these, against a `mongod` container that requires TLS and a client certificate.

[`check.yml`](.github/workflows/check.yml) looks for new upstream 1.x and 2.x releases every week and builds and publishes them the same way.

## Versions

Release tags follow upstream: the release tagged `v<version>` is built from mongo-c-driver `<version>`. Each release lists the OpenSSL, compiler and runtime versions it was built with.

Only new mongo-c-driver releases trigger a build, and each Windows build takes the latest OpenSSL 3.5 release available at that time. A new OpenSSL release on its own does not produce a new package.

## License

The build scripts in this repository are under the [MIT license](LICENSE). The binaries are distributed under their own licenses, included in each archive: Apache License 2.0 for the MongoDB C Driver and for the OpenSSL of the Windows packages, plus the notices of the code bundled in the driver. `vcruntime140.dll` is a Microsoft file, redistributed unmodified under the Visual Studio license terms.

MongoDB is a trademark of MongoDB, Inc.
