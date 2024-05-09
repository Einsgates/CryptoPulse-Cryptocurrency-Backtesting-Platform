#!/usr/bin/env bash

# Copyright 2023 Zinchenko Serhii <zinchenko.serhii@pm.me>.
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
#
# DESCRIPTION:
#
# Extract C++ files and check for inclusion of iostream file.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

#######################################
# Find all 'iosteam' includes in a file.
# Arguments:
#   $1 - a file path
# Outputs:
#   Writes all found typos to stdout and to a log file.
# Note:
#   We don't handle ignored includes here.
#######################################
find_iostream()
{
    grep -Hn -e '#include <iostream>' -e '#include "iostream"' --color=always "${1}"
}

#######################################
# Extract C++ files and check them for iostream file inclusion.
# Globals:
#   BPROTO_CI_LINTER_TRACE
# Arguments:
#   $1 - a list of files to be checked
# Outputs:
#   Writes all found typos to stdout and to a log file.
#######################################
check()
{
    local -i found=0

    # Extract C++ header files if any.
    local -r changed_files="$(echo "${2}" | grep -E ".*\.[chi]pp$")"

    if [[ -n "${changed_files}" ]]; then
        if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
            echo -e "\033[0;33mDetected changes in ${changed_files}\033[0m"
        fi

        while IFS= read -r file; do
            if find_iostream "${file}" | grep -v "// ignore"; then
                found=1
            fi
        done <<< "${changed_files}"
    fi

    # Fail if there are some changes
    exit ${found}
}

#######################################
# Main entry point for this script.
# Globals:
#   BPROTO_CI_MERGE_REQUEST_CHANGED_FILES
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
