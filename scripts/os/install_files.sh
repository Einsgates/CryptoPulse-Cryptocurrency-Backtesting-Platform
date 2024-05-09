#!/usr/bin/env bash

# Copyright 2022-2023 Zinchenko Serhii <zinchenko.serhii@pm.me>.
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
# This script is mainly supposed to be used only by dockers image to provide a
# pre-build image with all pre-installed and pre-configured system packages and
# project dependencies.

set -e

#------------------------------------------------------------------------------#
# Install CI/CD scripts                                                        #
#------------------------------------------------------------------------------#
# See https://askubuntu.com/questions/308045 for more info.

chmod +x /tmp/*.*
cp /tmp/*.* /usr/local/bin/

# Remove this script from copied files, as we don't need it in a resulting image.
rm "/usr/local/bin/$(basename "${0}")"

#------------------------------------------------------------------------------#
# Install configuration files                                                  #
#------------------------------------------------------------------------------#
# See https://stackoverflow.com/questions/1024114 for more info.

declare -r DEST="/usr/local/share"
declare -r ENV_FILE="/etc/profile.d/linters.sh"

#######################################
# A utility function to provide a generic way to install linters config files.
# Globals:
#   DEST
#   ENV_FILE
# Arguments:
#   $1 - a linter name.
#   $2 - a linter's config file.
#   $3 - a custom name for installed config file.
#   $4 - an environment variable which contains a path to config file and is used
#        by a linter.
# Outputs:
#   A new directory with config file in $DEST and a new entries in $ENV_FILE file.
#######################################
install_file()
{
    echo -e "\033[0;33mInstall ${2} config file for ${1} linter\033[0m"

    local app="${1}"

    # Copy to a separate app directory.

    local -r app_dir="${DEST}/${app}"
    local -r src_config="/tmp/${2}"

    if [[ -n "${3}" ]]; then
        local -r dest_config="${app_dir}/${3}"
    else
        local -r src_config_ext="$(grep -Po '\..*\K\..*' <<< "${src_config}" || true)"
        local -r dest_config="${app_dir}/config${src_config_ext}"
    fi

    mkdir -p "${app_dir}"
    cp "${src_config}" "${dest_config}"

    # Add global environment variables.

    app="${app/-/}"
    app="${app/_/}"
    app="${app^^}" # uppercase

    echo "export BPROTO_${app}_CFG=\"${dest_config}\"" >> "${ENV_FILE}"

    if [[ -n "${4}" ]]; then
        echo "export ${4}=\"${dest_config}\"" >> "${ENV_FILE}"
    fi

    echo "" >> "${ENV_FILE}"
}

#######################################
# Main entry point for this script.
# Arguments:
#   $1 - a flag which indicates what set of files to install
#######################################
main()
{
    case "${1}" in
        "only_clang_format")
            install_file "clang-format" ".clang-format" "" ""
        ;;

        "only_hadolint")
            install_file "hadolint" ".hadolint.yaml" "" ""
        ;;

        *)
            #            Linter name,     Input CFG,         Output CFG,    Custom env var name
            install_file "clang-format"   ".clang-format"    ""             ""
            install_file "clang-tidy"     ".clang-tidy"      ""             ""
            install_file "cmakelang"      ".cmake-format.py" ""             ""
            install_file "codespell"      ".codespellrc"     ""             ""
            install_file "flake8"         ".flake8"          "flake8.ini"   ""
            install_file "hadolint"       ".hadolint.yaml"   ""             ""
            install_file "licenseheaders" ".license.tmpl"    "license.tmpl" ""
            install_file "yamllint"       ".yamllint"        ""             "YAMLLINT_CONFIG_FILE"
        ;;
    esac
}

main "$@"
