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
# Extract C++ files and check their formatting.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

declare -r PATCH_FILE="clang_format.patch"

declare -r DEFAULT_CLANGFORMAT_CFG=".clang-format"
if [[ -s "${DEFAULT_CLANGFORMAT_CFG}" ]]; then
    # clang-format will detect this file by itself.
    declare -r CLANGFORMAT_CFG=""
elif [[ -n "${BPROTO_CLANGFORMAT_CFG}" ]]; then
    declare -r CLANGFORMAT_CFG="-style=file:${BPROTO_CLANGFORMAT_CFG}"
else
    echo -e "\033[0;31mFailed to detect configuration file for clang-format!\033[0m" >&2
    exit 1
fi

#######################################
# Extract C++ files and check their formatting.
# Globals:
#   CLANGFORMAT_CFG
#   BPROTO_CI_LINTER_TRACE
#   PATCH_FILE
# Arguments:
#   $1 - target Git branch
#   $2 - a list of files to be checked
#   $3 - a last Git commit SHA in the current branch
# Outputs:
#   Writes a diff with correct formatting to stdout and to a patch file.
#######################################
check()
{
    local -r last_commit_sha="${3}"
    local -r target_branch="${1}"
    git fetch origin "${target_branch}"

    # Extract C++ files if any.
    local -r changed_files="$(echo "${2}" | grep --color=never -E ".*\.[chi]pp$")"

    if [[ -n "${changed_files}" ]]; then
        if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
            echo -e "\033[0;33mDetected changes in ${changed_files}\033[0m"
        fi

        # Perform check

        while IFS= read -r file; do
            # Comment all C++ attributes that are placed on a single line.
            # Example:
            #   [[nodiscard]] -> // [[nodiscard]]
            #
            # This doesn't break formatting, unlike what has been suggested
            # here: https://stackoverflow.com/questions/45740466.
            sed -i -E 's|(\[\[.*\]\])$|\/\/ \1|' "${file}"

            # Patch all C++ classes that are placed on a single line.
            # Example:
            #   class MySuperClass -> class MySuperClass // clang-format-hack
            #
            # Luckily this force clang-format to handle inheritance list as we
            # want. See more here: https://stackoverflow.com/questions/61082996.
            sed -i -E 's|(class .*$)|\1 \/\/ clang-format-hack|' "${file}"
            sed -i -E 's|(struct .*$)|\1 \/\/ clang-format-hack|' "${file}"

            # Add a comment to all template openers ('<') that ends the line.
            # Example:
            #   using ContainerTypes = boost::mpl::vector< // clang-format-hack
            #      boost::mpl::pair<...>,
            #      boost::mpl::pair<...>
            #   >;
            #
            # Of cause we should be careful with this, because clang-format
            # won't be able to reformat code where it should.
            sed -i -E 's|([ ]{0,}<)$|\1 \/\/ clang-format-hack|' "${file}"
        done <<< "${changed_files}"

        # shellcheck disable=SC2086
        git diff -U0 "origin/${target_branch}..${last_commit_sha}" \
            | clang-format-diff.py \
                ${CLANGFORMAT_CFG} \
                -i \
                -p1 \
                -regex '^project\/.*\.[chi]pp$' \
                -v

        while IFS= read -r file; do
            # Uncomment all previously commented C++ attributes.
            sed -i -E 's|\/\/ (\[\[.*\]\])$|\1|' "${file}"
            # Remove all clang-format-hack comments. Take into account that
            # clang-format could align them as trailing comments.
            sed -i -E 's|[ ]{0,}\/\/ clang-format-hack$||' "${file}" "${file}"
        done <<< "${changed_files}"

        # Fail if there are some changes

        git diff > "${PATCH_FILE}"
        bat --color=always -pp "${PATCH_FILE}"
        git diff-index --quiet HEAD --
    fi
}

#######################################
# Main entry point for this script.
# Globals:
#   CI
#   CI_COMMIT_SHA
#   CI_MERGE_REQUEST_TARGET_BRANCH_NAME
#   BPROTO_CI_MERGE_REQUEST_CHANGED_FILES
# Arguments:
#   $1 - a list of files to be checked
#######################################
main()
{
    if [[ "${CI}" == "true" ]]; then
        local -r target_branch="${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
        local -r last_commit_sha="${CI_COMMIT_SHA}"
    else
        echo -e "\033[0;31mThis script can be used only in GitLab CI environment!\033[0m" >&2
        exit 1
    fi

    if [[ -v BPROTO_CI_MERGE_REQUEST_CHANGED_FILES ]]; then
        # Variable can exist but is empty.
        if [[ -n "${BPROTO_CI_MERGE_REQUEST_CHANGED_FILES}" ]]; then
            local -r changed_files="${BPROTO_CI_MERGE_REQUEST_CHANGED_FILES}"
        else
            echo -e "\033[0;33mNo useful changes have been detected in files!\033[0m"
            exit 0
        fi
    else
        echo -e "\033[0;31mFailed to detect modified files!\033[0m" >&2
        exit 1
    fi

    check "${target_branch}" "${changed_files}" "${last_commit_sha}"
}

main
