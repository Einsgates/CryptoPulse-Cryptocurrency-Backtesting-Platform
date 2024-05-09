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

set -e

#
# CLI
#

usage()
{
    echo "Usage: $0 [-t <string>] -p <string> -r <string>

  -t            target branch
  -p            prefix for macro and include guards
  -r            a repo short path (CI_PROJECT_PATH)
" 1>&2
    exit 1
}

while getopts ":t:p:r:" o; do
    case "${o}" in
        t)
            declare -r TARGET_BRANCH="${OPTARG}"
            ;;
        p)
            declare -r PREFIX="${OPTARG}"
            ;;
        r)
            declare -r REPO_PATH="${OPTARG}"
            ;;
        *)
            usage
            ;;
    esac
done

shift $((OPTIND - 1))

if [ -z "${PREFIX}" ] || [ -z "${REPO_PATH}" ]; then
    usage
fi

if [ -z "${TARGET_BRANCH}" ]; then
    declare TARGET_BRANCH="develop"
fi

#
# Utils
#

commit()
{
    echo -e "\033[0;32m${1} \033[0m"
    git commit -q --message "${1}" --trailer "Tags: ${2}"
}

commit_if_changed()
{
    local -r file="${1}"
    local -r commit_msg="${2}"
    local -r commit_tags="${3}"

    if git diff --name-only | grep -q -e "${file}"; then
        git add "${file}"
        commit "${commit_msg}" "${commit_tags}"
    else
        echo -e "\033[0;33mHave not detected changes in '${file}' file, skipping.\033[0m"
    fi
}

#
# Checks
#

# Check 1

if [[ $(git rev-parse --show-toplevel 2> /dev/null) != "$PWD" ]]; then
    echo -e "\033[0;31m'$0' can be used only in the root of git repository! \033[0m"
    exit 1
fi

# Check 2

if git show-ref -q --heads "${TARGET_BRANCH}"; then
    echo -e "\033[0;31m'${TARGET_BRANCH}' branch exists locally! \033[0m"
    exit 1
fi

#
# Mandatory actions
#

# Action 1

git checkout -b "${TARGET_BRANCH}"

# Action 2

declare -r EXTERNAL_CI=".gitlab/ci/external"

find "${EXTERNAL_CI}" -type f -exec sed -i -e "s|project: 'bproto/cicd'|project: '${REPO_PATH}'|g" {} \;
find "${EXTERNAL_CI}" -type f -exec sed -i -e "s|ref: main|ref: '${TARGET_BRANCH}'|g" {} \;

git add "${EXTERNAL_CI}"
commit "Replace project path from 'bproto/cicd' to '${REPO_PATH}' in CI external files" "CI, Cpp"

# Action 3

sed -i "s/BPROTO/${PREFIX}/g" .clang-tidy
git add .clang-tidy
commit "Replace BPROTO with ${PREFIX} in clang-tidy config file" "Cpp, ClangTidy, Linters"

# Action 4

declare -r IG_LINTER="scripts/cicd/lint/check_cpp_include_guards.sh"

sed -i "s/ __BPROTO_ / __${PREFIX}_ /g" "${IG_LINTER}"
git add "${IG_LINTER}"
commit "Change include guards prefix from BPROTO to ${PREFIX}" "Cpp, Linters, Shell"

# Action 5

commit_if_changed 'pyproject.toml' 'Add project specific python packages' 'Python'

# Action 6

commit_if_changed 'conan/profiles' 'Update conan profiles' 'Cpp, Conan'

# Action 7

commit_if_changed 'conan/conanfile.txt' 'Add project specific conan packages' 'Cpp, Conan'

#
# Optional actions
#

# Action 1

commit_if_changed '.clang-format' 'Update clang-format config' 'Cpp, ClangFormat, Linters'

# Action 2

commit_if_changed '.clang-tidy' 'Update clang-tidy config' 'Cpp, ClangTidy, Linters'
