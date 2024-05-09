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
# Check files for correct license header.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

declare -r PATCH_FILE="license_headers.patch"

declare -r DEFAULT_TEMPLATE_FILE=".license.tmpl"
if [[ -s "${DEFAULT_TEMPLATE_FILE}" ]]; then
    declare -r TEMPLATE_FILE="${DEFAULT_TEMPLATE_FILE}"
elif [[ -n "${BPROTO_LICENSEHEADERS_CFG}" ]]; then
    declare -r TEMPLATE_FILE="-t ${BPROTO_LICENSEHEADERS_CFG}"
fi

#######################################
# Check files for correct license header.
# Globals:
#   BPROTO_CI_LINTER_TRACE
#   PATCH_FILE
#   TEMPLATE_FILE
# Arguments:
#   $1 - a list of files to be checked
# Outputs:
#   Writes a diff with correct license header to stdout and a patch file.
#######################################
check()
{
    # Extract CMake files if any.
    local -r exclude="3rd-party|.*\.drawio$|.*\.jpg$|.*\.md$|.*\.rst$"
    local -r changed_files="$(echo "${1}" | grep --color=never -E "${exclude}")"

    if [[ -n "${changed_files}" ]]; then
        if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
            echo -e "\033[0;33mDetected changes in ${changed_files}\033[0m"
        fi

        # Perform check

        local -r years="2020-$(date +%Y)"

        # shellcheck disable=SC2086
        licenseheaders \
            --additional-extensions script=.cmake,.txt \
            ${TEMPLATE_FILE} \
            -y "${years}" \
            -f ${changed_files//$'\n'/ }

        # Fail if there are some changes

        git diff > "${PATCH_FILE}"
        bat --color=always -pp "${PATCH_FILE}"
        git diff-index --quiet HEAD --
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
