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
# Extract all commits from HEAD of current branch to its parent and check them
# for typos.
#
# ATTENTION:
#
# This script is mainly supposed to be used only by CI/CD linter jobs.

set -e

declare -r CODESPELL_LOG_FILE="codespell.txt"
declare -r COMMIT_MSG_FILE="commit.txt"

declare -r LOG_FILE="codespell.log"

declare -r DEFAULT_SETTINGS_FILE=".codespellrc"
if [[ -s "${DEFAULT_SETTINGS_FILE}" ]]; then
    declare -r SETTINGS_FILE="${DEFAULT_SETTINGS_FILE}"
elif [[ -n "${BPROTO_CODESPELL_CFG}" ]]; then
    declare -r SETTINGS_FILE="${BPROTO_CODESPELL_CFG}"
fi

# FMI:
#   - https://www.jessesquires.com/blog/2017/08/08/customizing-git-log/
#   - https://stackoverflow.com/questions/33806068
#   - https://stackoverflow.com/questions/76120184
declare -r GIT_PRETTY_LOG_FORMAT='
commit %C(bold magenta)%H%Creset
Author: %C(bold blue)%an%Creset %C(blue)<%ae>%Creset
Date: %C(bold cyan)%cd%Creset

%w(120,4,4)%-B%Creset
'

#######################################
# Extract a trailer part of a commit message with a specified key.
# Arguments:
#   $1 - a trailer key
#   $2 - a commit hash
# Outputs:
#   A content of a specific trailer with new lines and indentation removed.
#######################################
get_trailer()
{
    local -r key="${1}"
    local -r commit="${2}"

    # NOTE(b110011): Remove new lines and make one long string.
    #   If indentation is wrong, we will still get an error when grepping.
    git log --format="%(trailers:key=${key})" -n1 "${commit}" | sed "s/    \n/,/g"
}

#######################################
# Sort line values in ascending order.
# Arguments:
#   $1 - input line
#   $2 - values delimiter (default: ', ')
# Outputs:
#   A line with sorted values.
#######################################
sort_inplace()
{
    local -r line="${1}"
    local -r delim="${2:-, }"

    echo "${line}" | sed "s|${delim}|\n|g" | sort | sed ':a;N;$!ba;s|\n|'"${delim}"'|g'
}

#######################################
# Extract all commits from HEAD of current branch to its parent and check them
# for typos.
# Globals:
#   CODESPELL_LOG_FILE
#   COMMIT_MSG_FILE
#   LOG_FILE
#   SETTINGS_FILE
# Arguments:
#   $1 - target Git branch
# Outputs:
#   Writes all found typos to stdout and to a log file.
#######################################
check()
{
    local -r target_branch="${1}"
    local -i failed=0

    git fetch origin "${target_branch}"

    # Redirect everything to log file.
    # https://stackoverflow.com/questions/11229385
    exec 3>&1 1> "${LOG_FILE}"

    for commit in $(git rev-list "origin/${target_branch}..HEAD"); do
        git log -n 1 "${commit}" > "${COMMIT_MSG_FILE}"

        local -i is_commit_message_logged=0
        log_commit_message()
        {
            failed=1  # If we need to call this method, we have hit a jackpot.

            if [[ ${is_commit_message_logged} -eq 0 ]]; then
                is_commit_message_logged=1

                git log --color=always --pretty=format:"${GIT_PRETTY_LOG_FORMAT}" -n1 "${commit}"
            fi
        }

        codespell \
            --config "${SETTINGS_FILE}" \
            --enable-colors \
            "${COMMIT_MSG_FILE}" \
            > "${CODESPELL_LOG_FILE}" || true

        if [[ -s "${CODESPELL_LOG_FILE}" ]]; then
            log_commit_message
            cat "${CODESPELL_LOG_FILE}"
            echo ""
        fi

        # FMI: https://unix.stackexchange.com/questions/42898
        # NOTE(b110011): Doing as shellcheck suggests breaking the script.
        # shellcheck disable=SC2155
        local long_lines="$(git log --pretty=format:"%B" -n1 "${commit}" | grep '^.\{80\}' --color=always)"
        if [[ -n "${long_lines}" ]]; then
            log_commit_message
            echo -e "\033[0;31mBelow lines contain more than 80 symbols:\033[0m"
            echo "    - ${long_lines//$'\n'/$'\n'    - }"  # Prepend '    - ' to each line.
        fi

        check_trailer()
        {
            local -r key="${1}"
            local -r regex="${2}"
            local -r message="${3}"
            local -r allowed_format="${4}"

            local -r line="$(get_trailer "${key}" "${commit}")"
            if [[ -n "${line}" ]]; then
                local -r matched_line="$(grep -Eo "${regex}" <<< "${line}")"
                if [ "${matched_line}" != "${line}" ]; then
                    log_commit_message
                    echo -e "\033[0;31mInvalid format of ${message}! \033[0m"
                    echo -e "    \033[0;33mAllowed format: ${allowed_format}\033[0m"

                    return  # Stop here because we might not be able to correctly perform next check.
                fi

                local -r trailer_values=${line#"${key}: "}  # Remove key from line.
                local -r sorted_trailer_values="$(sort_inplace "${trailer_values}")"
                if [[ "${sorted_trailer_values}" != "${trailer_values}" ]]; then
                    log_commit_message
                    echo -e "\033[0;31m${message^} are not properly sorted! \033[0m"
                    echo -e "    \033[0;33mCorrent order: ${sorted_trailer_values}\033[0m"
                fi
            fi
        }

        check_trailer \
            'GitHub' \
            'GitHub: (#[0-9]+(, ){0,}){0,}' \
            'GitHub issue references' \
            'GitHub: #1, #2'

        check_trailer \
            'GitLab' \
            'GitLab: (#[0-9]+(, ){0,}){0,}' \
            'GitLab issue references' \
            'GitLab: #1, #2'

        check_trailer \
            'Jira' \
            'Jira: ([a-zA-Z]+-[0-9]+(, ){0,}){0,}' \
            'JIRA ticket IDs' \
            'Jira: MO-1, MO-10'

        check_trailer \
            'Tags' \
            'Tags: ([a-zA-Z0-9]+(, ){0,}){0,}' \
            'commit message tags' \
            'Tags: TagA1, TagB2'
    done

    # Restore output to terminal.
    # https://stackoverflow.com/questions/25474854
    exec 1>&3 3>&-

    # Show our log in terminal.
    # https://unix.stackexchange.com/questions/38634
    less -SEX -R codespell.log

    # Removing colors from output
    # https://stackoverflow.com/a/18000433
    sed -i -r "s/\x1B\[([0-9]{1,3}(;[0-9]{1,2})?)?[mGK]//g" "${LOG_FILE}"

    # Fail if there are some changes
    # shellcheck disable=SC2046
    exit ${failed}
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
    elif [[ -n "${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}" ]]; then
        check "${CI_MERGE_REQUEST_TARGET_BRANCH_NAME}"
    else
        echo -e "\033[0;31mFailed to detect Git target branch!\033[0m" >&2
        exit 1
    fi
}

main "$@"
