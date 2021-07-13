#!/usr/bin/env -S bash ../.port_include.sh
port=v
version=weekly.2021.28
files="https://codeload.github.com/vlang/v/tar.gz/refs/tags/$version v-$version.tar.gz c5e98cd4ea011dde2f08e68144e98e85e82fe45eef92a17dedc06e9404da117e"

build() {
    (
    cd "$workdir"
    make CC=gcc all # local build
    ./v -prod -cc "$CC" -o v2 cmd/v
    )
}

install() {
    # v requires having rw access to the srcdir to rebuild on demand
    # so we just copy that into the default user's home for now.
    # proper system-wide dist builds will be added in vlang later
    mkdir -p "${SERENITY_INSTALL_ROOT}/home/anon/vlang"
    cp -rf "$workdir"/* "${SERENITY_INSTALL_ROOT}/home/anon/vlang"
    ln -fs "${SERENITY_INSTALL_ROOT}/home/anon/vlang/v2" "${SERENITY_INSTALL_ROOT}/usr/local/bin/v"
}
