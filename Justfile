# Note: For Qt on Windows - PATH=/c/Qt/5.15.2/msvc2019_64/bin:$PATH

# Just variables (can be overidden in CLI)

BUILD                := "build"
CMAKE_FLAGS          := ""
CMAKE_INSTALL_PREFIX := BUILD / "install"
CMAKE_BUILD_TYPE     := "Release"
VERBOSE              := "Y"

GENERATOR            := if os_family() == "windows"             { "Visual Studio 16 2019"} else { "" }
PLATFORM             := if GENERATOR == "Visual Studio 16 2019" { "x64"}                   else { "" }

INSTALL  := if os_family() == "windows" { "Install" } else if CMAKE_BUILD_TYPE == "Release" { "install/strip" } else { "install"}
PARALLEL := if os_family() == "windows" { "" } else { "--parallel"}

EXTERNAL := "build_external"

default:
    @just --list --unsorted

# Completely rebuild from scratch
all: clobber external cmake build test install

# Remove cmake and build outputs
clobber:
    cmake -E rm -rf {{EXTERNAL}}
    cmake -E rm -rf {{BUILD}}
    cmake -E rm -rf {{CMAKE_INSTALL_PREFIX}}

external:
    cmake -S thirdparty/external -B {{EXTERNAL}} \
        {{ if GENERATOR == "" { "" } else { "-G \"" + GENERATOR + "\""} }} \
        {{ if PLATFORM  == "" { "" } else { "-A " + PLATFORM} }} \
        -DCMAKE_BUILD_TYPE={{CMAKE_BUILD_TYPE}} \
        {{ if VERBOSE   != "" { "-DCMAKE_VERBOSE_MAKEFILE=" + VERBOSE } else { "" } }} \
        {{CMAKE_FLAGS}}
    cmake --build {{EXTERNAL}} --config {{CMAKE_BUILD_TYPE}} {{PARALLEL}}

# cmake configuration step
cmake:
    cmake -S . -B {{BUILD}} \
        {{ if GENERATOR == "" { "" } else { "-G \"" + GENERATOR + "\""} }} \
        {{ if PLATFORM  == "" { "" } else { "-A " + PLATFORM} }} \
        -DCMAKE_INSTALL_PREFIX={{CMAKE_INSTALL_PREFIX}} \
        -DCMAKE_BUILD_TYPE={{CMAKE_BUILD_TYPE}} \
        {{ if VERBOSE   != "" { "-DCMAKE_VERBOSE_MAKEFILE=" + VERBOSE } else { "" } }} \
        {{CMAKE_FLAGS}}

# cmake build step
build:
    cmake --build {{BUILD}} --config {{CMAKE_BUILD_TYPE}} {{PARALLEL}}

# installation step
install:
    cmake --build {{BUILD}} --config {{CMAKE_BUILD_TYPE}} {{PARALLEL}} --target {{INSTALL}}

# Remove build outputs
clean:
    cmake --build {{BUILD}} --config {{CMAKE_BUILD_TYPE}} {{PARALLEL}} --target clean

# Unit tests
test:
    ctest \
        {{ if VERBOSE != "" { "--extra-verbose" } else { "--verbose" } }} \
        --output-on-failure -C {{CMAKE_BUILD_TYPE}} --test-dir {{BUILD}}

# Miscellanea

# Toolset version information
version:
    git --version
    cmake --version

# git clean
git-clean:
    git clean -xdf .

