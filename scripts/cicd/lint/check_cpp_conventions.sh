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
# Extract C++ files and check that they follow our conventions.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

# clang-tidy generates errors on missing PCH, as they are generated only during
# project compilation.
declare -a CMAKE_ARGS=(
    "-DCMAKE_BUILD_TYPE=Debug"
    "-DCMAKE_DISABLE_PRECOMPILE_HEADERS=ON"
    "-DCMOPTS_ENABLE_CLANG_TIDY=ON"
    "-DCMOPTS_ENABLE_LTO=OFF"
    # Either contains CMAKE_TOOLCHAIN_FILE option or nothing.
    "${BPROTO_CI_CMAKE_TOOLCHAIN_FILE}"
)
declare -r CMAKE_BUILD_DIR="build"

# See https://stackoverflow.com/questions/3601515 for more magic.
declare -r IS_CI_RUN="${CI:=false}"

declare -r LOG_FILE="clang_tidy.log"

NPROC=$(nproc || sysctl -n hw.physicalcpu) 2> /dev/null
declare -r NPROC

declare -r DEFAULT_CLANGTIDY_CFG=".clang-tidy"
if [[ -s "${DEFAULT_CLANGTIDY_CFG}" ]]; then
    # clang-tidy will detect this file by itself.
    declare -r CLANGTIDY_CFG=""
elif [[ -n "${BPROTO_CLANGTIDY_CFG}" ]]; then
    declare -r CLANGTIDY_CFG="${BPROTO_CLANGTIDY_CFG}"

    # TODO(b110011): This is a workaround for currunt clang-tidy scripts.
    # Even so 'run-clang-tidy' supports '-config-file', 'clang-tidy-diff'
    # still doesn't know about this. Therefore it's better to use same
    # approach for both tools.
    #
    # Once both scripts will support '-config-file', we can move to our
    # standard approach and pass configuration file directly to those scripts.
    cp "${CLANGTIDY_CFG}" ".clang-tidy"
else
    echo -e "\033[0;31mFailed to detect configuration file for clang-format!\033[0m" >&2
    exit 1
fi

# NOTE(b110011): gmock in gtest/1.11.0 generated some errors in "merge request"
#   mode, so we want to disable them and clean our pipeline.
declare -r DISABLED_GMOCK_CHECKS=(
    "misc-non-private-member-variables-in-classes"
)

#######################################
# Output a colorful log message to terminal.
# Arguments:
#   $1 - a message to be logged
#######################################
log()
{
    echo -e "\033[0;33m'${1}'\033[0m"
}

#######################################
# Make all necessary preparations to successfully run clang-tidy based tools.
# Globals:
#   CMAKE_ARGS
#   CMAKE_BUILD_DIR
#######################################
prepare()
{
    # clang-tidy generates errors on some GCC flags, so we have to use clang compiler
    # explicitly here.
    local -r cc="clang"
    local -r cxx="clang++"

    # Generate compile_commands.json file for clang-tidy.
    CC="${cc}" CXX="${cxx}" cmake -S. -B"${CMAKE_BUILD_DIR}" "${CMAKE_ARGS[@]}"

    # Because JQ can't modify file in-place...

    local -r old_compile_commands="${CMAKE_BUILD_DIR}/old_compile_commands.json"
    local -r compile_commands="${CMAKE_BUILD_DIR}/compile_commands.json"

    mv "${compile_commands}" "${old_compile_commands}"

    # Remove files that is generated only during project compilation.
    local -r query='del(.[] | select(.file | contains("read_only_git_info_impl")))'
    jq -r "${query}" "${old_compile_commands}" > "${compile_commands}"

    # Clean-up.

    rm "${old_compile_commands}"
}

#######################################
# Get a field from a CSV table separated by '|'.
# Arguments:
#   $1 - a line from CSV table
#   $2 - position of the field in the CSV table (mix 1, max 3)
#######################################
get_field()
{
    cut -d'|' -f "${2}" -s <<< "${1}"
}

#######################################
# Check all C++ file in the project for follow our conventions.
#
# This is a heavy and long-running job (>30min) so we cache it result and rerun
# only on new changes.
#
# Globals:
#   CI_API_V4_URL
#   CI_COMMIT_REF_NAME
#   CI_COMMIT_SHORT_SHA
#   CI_JOB_TOKEN
#   CI_PROJECT_ID
#   CMAKE_BUILD_DIR
#   IS_LOCAL_RUN
#   NPROC
#   LOG_FILE
# Outputs:
#   Writes all found error to stdout and a file.
#######################################
check_all()
{
    local -r job_token="Job-Token: ${CI_JOB_TOKEN}"

    local -r pkg_name="nightly_clang_tidy"
    local -r pkg_file="result.txt"

    local -r status_fail="fail"
    local -r status_success="success"

    local -r api_url="${CI_API_V4_URL}/projects/${CI_PROJECT_ID}/packages/generic/${pkg_name}/${CI_COMMIT_REF_NAME}/${pkg_file}"

    # Download cached result from a previous job invocation and check if we need
    # to run job again.
    if [[ "${IS_CI_RUN}" == "true" ]]; then
        log "Downloading and checking previous cached result."

        local -r result="$(wget --header="${job_token}" -O - --quiet "${api_url}" || true)"

        local -r prev_commit_sha1="$(get_field "${result}" 1)"
        local -r commit_sha1="$(get_field "${result}" 2)"
        local -r status="$(get_field "${result}" 3)"

        if [[ "${commit_sha1}" == "${CI_COMMIT_SHORT_SHA}" ]]; then
            log "No new changes have been found since last time!"

            # There might be no SHA-1 of previous Git commit, if there has been
            # no file in package registry.
            if [[ -z "${prev_commit_sha1}" ]]; then
                log "Exiting with a previous CI job status."
            else
                log "Listing all commits that might contain issues and exiting with a previous job status."
                git log --pretty=short "${prev_commit_sha1}..HEAD"
            fi

            # shellcheck disable=SC2046
            exit $(test "${status}" == "${status_success}")
        fi
    fi

    # Perform check

    run-clang-tidy \
        -j "${NPROC}" \
        -p "${CMAKE_BUILD_DIR}" \
        -quiet \
        -use-color \
        | tee "${LOG_FILE}" || true

    # Deploy

    if [[ "${IS_CI_RUN}" == "true" ]]; then
        local -r new_prev_commit_sha1="${commit_sha1}"
        local -r new_commit_sha1="${CI_COMMIT_SHORT_SHA}"
        local new_status="${status_success}"

        if [[ $(grep -c "error:" "${LOG_FILE}") -ne 0 ]]; then
            local new_status="${status_fail}"

            # There might be no SHA-1 of previous Git commit, if there has been
            # no file in package registry.
            if [[ -n "${new_prev_commit_sha1}" ]]; then
                log "Listing all new commits that might contain issues and exiting..."
                git log --pretty=short "${new_prev_commit_sha1}..HEAD"
            fi
        fi

        echo "${new_prev_commit_sha1}|${new_commit_sha1}|${new_status}" > "${pkg_file}"

        curl --header "${job_token}" --upload-file "${pkg_file}" "${api_url}"
    fi
}

#######################################
# Extract C++ files from a diff and check that they follow our conventions.
# Globals:
#   CMAKE_BUILD_DIR
#   DISABLED_GMOCK_CHECKS
#   NPROC
#   LOG_FILE
# Arguments:
#   $1 - target Git branch
#   $2 - a list of changed files
# Outputs:
#   Writes all found error to stdout and a file.
#######################################
check_diff()
{
    # Extract C++ mock files if any.
    local -r changed_mock_files="$(echo "${2}" | grep --color=never -E ".*_mock.*")"
    if [[ -n "${changed_mock_files}" ]]; then
        if [[ "${BPROTO_CI_LINTER_TRACE}" == "true" ]]; then
            echo -e "\033[0;33mDetected changes in these mocks ${changed_files}\033[0m"
        fi

        # Patch C++ mocks files to disable certain checks.

        local disabled_checks
        disabled_checks="$(IFS=, ; echo "${DISABLED_GMOCK_CHECKS[*]}")"
        disabled_checks="\/\/ NOLINTNEXTLINE\(${disabled_checks}\)"

        while IFS= read -r file; do
            # This will Insert a line before match found.
            sed -i "/MOCK_METHOD/i ${disabled_checks}" "${file}"
        done <<< "${changed_files}"
    fi

    local -r target_branch="${1}"
    git fetch origin "${target_branch}"

    git diff -U0 "origin/${target_branch}..HEAD" \
        | clang-tidy-diff-14.py \
            -clang-tidy-binary clang-tidy-14 \
            -j "${NPROC}" \
            -p1 \
            -path "${CMAKE_BUILD_DIR}" \
            -quiet \
            -use-color \
        | tee "${LOG_FILE}"
}

#######################################
# Main entry point for this script.
# Globals:
#   CI_MERGE_REQUEST_TARGET_BRANCH_NAME
# Arguments:
#   $1 - a list of files to be checked
#######################################
main()
{
    prepare

    if [[ -n "${1}" ]]; then
        log "Running 'clang-tidy-diff' on local changes..."
        check_diff "${1}"
    elif [[ -n "${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}" ]]; then
        log "Running 'clang-tidy-diff' on merge request..."

        local -r target_branch="${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
        local -r changed_files="${BPROTO_CI_MERGE_REQUEST_CHANGED_FILES}"

        check_diff "${target_branch}" "${changed_files}"
    else
        log "Running 'run-clang-tidy' on the hole project..."
        check_all
    fi

    # Removing colors from output
    # https://stackoverflow.com/a/18000433
    sed -i -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" "${LOG_FILE}"

    # Fail if there are some changes
    # shellcheck disable=SC2046
    exit $(grep -c "error:" "${LOG_FILE}")
}

main "$@"
