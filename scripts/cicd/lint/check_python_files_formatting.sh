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
# Extract Python files and check their formatting.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

declare -r PATCH_FILE="black.patch"

#######################################
# Extract Python files and check their formatting.
# Globals:
#   BPROTO_CI_LINTER_TRACE
#   PATCH_FILE
# Arguments:
#   $1 - a list of files to be checked
# Outputs:
#   Writes a diff with correct formatting to stdout and to a patch file.
#######################################
check()
{
    # Extract Python files if any.
    local -r changed_files="$(echo "${1}" | grep --color=never -E ".*\.py$")"

    if [[ -n "${changed_files}" ]]; then
        if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
            echo -e "\033[0;33mDetected changes in ${changed_files}\033[0m"
        fi

        # Perform check

        # shellcheck disable=SC2086
        black --check --diff --quiet ${changed_files//$'\n'/ } > "${PATCH_FILE}" \
            || true

        # Fail if there are some changes

        bat --color=always -pp "${PATCH_FILE}"

        # Fail if there are some changes
        # shellcheck disable=SC2046
        exit $(test ! -s "${PATCH_FILE}")
    fi
}

#######################################
# Main entry point for this script.
# Arguments:
#   $1 - a list of files to be checked
#######################################
main()
{
    if [[ -n "${1}" ]]; then
        check "${1}"
    elif [[ -v BPROTO_CI_MERGE_REQUEST_CHANGED_FILES ]]; then
        # Variable can exist but is empty.
        if [[ -n "${BPROTO_CI_MERGE_REQUEST_CHANGED_FILES}" ]]; then
            check "${BPROTO_CI_MERGE_REQUEST_CHANGED_FILES}"
        else
            echo -e "\033[0;33mNo useful changes have been detected in files!\033[0m"
        fi
    else
        echo -e "\033[0;31mFailed to detect modified files!\033[0m" >&2
        exit 1
    fi
}

main "$@"
