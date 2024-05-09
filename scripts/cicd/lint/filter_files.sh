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
# Filter out all deleted files from a list of modified files between current and
# target branches.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

#######################################
# Filter out all deleted files from a list of modified files between current and
# target branches.
# Arguments:
#   $1 - target Git branch
# Outputs:
#   Writes filtered file paths to stdout.
#######################################
filter()
{
    local -r target_branch="${1}"

    git fetch origin "${target_branch}"

    # Get a list of files that has been changed in this merge request.
    modified_files="$(git diff-tree --name-only -r "HEAD..origin/${target_branch}" \
        | grep -v '3rd-party' | sort)"

    # Exclude all deleted files.
    while IFS="" read -r file || [[ -n "${file}" ]]; do
        if [ -r "${file}" ]; then
            echo "${file}"
        fi
    done <<< "${modified_files}"
}

#######################################
# Main entry point for this script.
# Arguments:
#   $1 - target Git branch
#######################################
main()
{
    if [[ -n "${1}" ]]; then
        filter "${1}"
    elif [[ -n "${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}" ]]; then
        filter "${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
    else
        echo -e "\033[0;31mFailed to detect Git target branch!\033[0m" >&2
        exit 1
    fi
}

main "$@"
