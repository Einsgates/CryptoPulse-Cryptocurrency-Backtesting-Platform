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
#
# DESCRIPTION:
#
# Extract C++ header files and check for correct format of include guards.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

API_DIRS='include|(?<!misc/|tests/)mocks'
SPECIAL_DIRS="include|mocks"

LOG_FILENAME="guardonce.log"

# Include guard pattern for checkguard from guardonce package.
PATTERN="path | remove - | remove _ | prepend __BPROTO_ | append __ | upper"

#######################################
# Check C++ header file for correct include guard.
# Arguments:
#   $1 - log file
#   $2 - C++ header file to check
# Outputs:
#   Writes an error message to log file if C++ header file has invalid include
#   guard.
#######################################
check_file()
{
    local -r log_file="${1}"
    local -r file="${2}"

    local -r result="$(checkguard -o guard -p "${PATTERN}" "${file}")"
    if [[ -n "${result}" ]]; then
        local -r guard="$(checkguard -o guard -p "${PATTERN}" "${file}" -n)"

        echo -e "\033[0;33m${result}\033[0m -> \033[0;31m${guard}\033[0m"
        echo "${result} -> ${guard}" >> "${log_file}"
    fi
}

#######################################
# Check C++ API header file for correct include guard.
# Globals:
#   OLDPWD
#   SPECIAL_DIRS
# Arguments:
#   $1 - root directory path for Git project
#   $2 - log file
#   $3 - C++ header file to check
# Outputs:
#   Writes an error message to log file if C++ header file has invalid include
#   guard.
#######################################
check_api_file()
{
    local -r project_root_dir="${1}"
    local -r log_file="${2}"
    local -r file="${3}"

    local -r dir_type="$(echo "${file}" | grep -Eo "${SPECIAL_DIRS}")"
    # https://wiki.bash-hackers.org/syntax/pe
    local -r project="${file%%/"${dir_type}"*}" # Get path before dir_type

    # In CI mode we get sorted list of modified files, so we can optimise this a
    # little bit.
    local -r destination="${project_root_dir}/${project}/${dir_type}"
    if [[ "${destination}" != "${OLDPWD}" ]]; then
        cd "${destination}"
    fi

    local -r project_file="${file##*"${dir_type}"/}" # Get path after dir_type
    check_file "${log_file}" "${project_file}"
}

#######################################
# Filter out all deleted files from diff between current and target branches.
# Globals:
#   BPROTO_CI_LINTER_TRACE
# Arguments:
#   None
# Outputs:
#   Writes all files to stdout that will be checked.
#######################################
print_files()
{
    if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
        echo -en "\n\033[0;33mDetected changes in ${1}\033[0m\n"
        echo -en "${2}\n"
    fi
}

#######################################
# Extract C++ files and check their include guards.
# Globals:
#   API_DIRS
#   LOG_FILENAME
# Arguments:
#   $1 - root directory path for Git project
#   $2 - a list of all changed files
# Outputs:
#   Writes filtered file paths to CI_MERGE_REQUEST_CHANGED_FILES global
#   envirinment variable.
#######################################
check()
{
    local -r project_root_dir="${1}"
    local -r log_file="${project_root_dir}/${LOG_FILENAME}"

    # Extract C++ header files if any.
    local -r changed_files="$(echo "${2}" | grep -E ".*\.[hi]pp$")"

    if [[ -n "${changed_files}" ]]; then
        print_files "" "${changed_files}"

        # Perform check for API files

        local -r changed_api_files="$(echo "${changed_files}" | grep -P "${API_DIRS}")"
        if [[ -n "${changed_api_files}" ]]; then
            print_files "API files:" "${changed_api_files}"

            # As a list of modified files comes with '\n', loop over it this way.
            while IFS= read -r api_file; do
                check_api_file "${project_root_dir}" "${log_file}" "${api_file}"
            done <<< "${changed_api_files}"

            cd "${project_root_dir}"
        fi

        # Perform check for sources

        local -r changed_src_files="$(echo "${changed_files}" | grep -Pv "${API_DIRS}")"
        if [[ -n "${changed_src_files}" ]]; then
            print_files "source files" "${changed_src_files}"

            cd "${project_root_dir}/project"

            # As a list of modified files comes with '\n', loop over it this way.
            while IFS= read -r src_file; do
                if echo "${src_file}" | grep -q "misc"; then
                    # This is a workaround a special 'misc' folder. Here we try
                    # to detect this folder and correctly handle it.
                    #
                    # Please take into account that we exclude MISC from generated
                    # include guards.
                    local project_file="${src_file##*project/misc/}"

                    cd misc
                    check_file "${log_file}" "${project_file}"
                    cd ..
                else
                    # Remove 'project' because we avoid it in our include guard.
                    local project_file="${src_file##*project/}"
                    check_file "${log_file}" "${project_file}"
                fi
            done <<< "${changed_src_files}"

            cd "${project_root_dir}"
        fi
    fi

    # Fail if there are some changes
    # shellcheck disable=SC2046
    exit $(test ! -f "${log_file}")
}

#######################################
# Main entry point for this script.
# Arguments:
#   $1 - root directory path for Git project
#   $2 - a list of all changed files
#######################################
main()
{
    if [[ -n "${1}" ]]; then
        local -r project_root_dir="${1}"
    elif [[ -n "${CI_PROJECT_DIR}" ]]; then
        local -r project_root_dir="${CI_PROJECT_DIR}"
    else
        echo -e "\033[0;31mFailed to detect project root directory!\033[0m" >&2
        exit 1
    fi

    if [[ -n "${2}" ]]; then
        local -r changed_files="${2}"
    elif [[ -v BPROTO_CI_MERGE_REQUEST_CHANGED_FILES ]]; then
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

    check "${project_root_dir}" "${changed_files}"
}

main "$@"
