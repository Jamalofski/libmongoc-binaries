# Builds the smoke test through the CMake files of a libmongoc-binaries Windows package, the way the
# README describes: cmake -DCMAKE_PREFIX_PATH=<extracted folder>

cmake_minimum_required(VERSION 3.15)
project(smoke C)

# Both branches have this package and this target. On 1.x they lead to the mongoc-1.0 package and to
# mongo::mongoc_shared, the names that upstream documents for that branch.
find_package(mongoc REQUIRED)

add_executable(smoke smoke.c)
target_link_libraries(smoke PRIVATE mongoc::mongoc)
