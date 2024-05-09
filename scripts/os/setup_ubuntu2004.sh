#!/usr/bin/env bash

# Copyright 2020-2023 Zinchenko Serhii <zinchenko.serhii@pm.me>.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# DESCRIPTION:
#
# This script is mainly supposed to be used only by Ubuntu 20.04 docker image to
# provide a pre-build image with all pre-installed and pre-configured system
# packages and project dependencies.

set -e

#------------------------------------------------------------------------------#
# Packages                                                                     #
#------------------------------------------------------------------------------#

declare -a INSTALL_PKGS=(
    # compilation
    "g++-13"
    "gcc-13"
    "make"
    # essential
    "git"
    "python3-pip"
    # pybind11
    "libpython3-dev"
)

#
# Install
#

# set noninteractive installation
export DEBIAN_FRONTEND=noninteractive
export TZ=Europe/Kiev

# We need to enable official Ubuntu Toolchain PPA to get new version of GCC.
#   - https://stackoverflow.com/questions/67298443
#   - https://wiki.ubuntu.com/ToolChain#PPA_packages
apt-get update
apt-get install --no-install-recommends -y software-properties-common
add-apt-repository ppa:ubuntu-toolchain-r/test

# Install packages

apt-get install --no-install-recommends -y "${INSTALL_PKGS[@]}"

#
# Clean
#

rm -rf /var/lib/apt/lists/*
apt-get clean

#------------------------------------------------------------------------------#
# Packages Aliases                                                             #
#------------------------------------------------------------------------------#

# See https://stackoverflow.com/questions/7832892
# See https://stackoverflow.com/questions/67298443

# GCC

update-alternatives --install /usr/bin/cpp cpp /usr/bin/cpp-13 110

update-alternatives \
    --install /usr/bin/gcc gcc /usr/bin/gcc-13 110 \
    --slave /usr/bin/g++ g++ /usr/bin/g++-13 \
    --slave /usr/bin/gcov gcov /usr/bin/gcov-13 \
    --slave /usr/bin/gcc-ar gcc-ar /usr/bin/gcc-ar-13 \
    --slave /usr/bin/gcc-ranlib gcc-ranlib /usr/bin/gcc-ranlib-13

#------------------------------------------------------------------------------#
# Python                                                                       #
#------------------------------------------------------------------------------#

pip3 install --no-cache-dir . '.[conan]'
