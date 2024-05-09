# Copyright 2024 Zinchenko Serhii <zinchenko.serhii@pm.me>.
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

# Fix variables

$BPROTO_CI_CMAKE_ARGS = ${env:BPROTO_CI_CMAKE_ARGS}
if (${BPROTO_CI_CMAKE_ARGS} -eq $null) {
    $BPROTO_CI_CMAKE_ARGS = ""
}

$BPROTO_CI_CMAKE_EXTRA_ARGS = ${env:BPROTO_CI_CMAKE_EXTRA_ARGS}
if (${BPROTO_CI_CMAKE_EXTRA_ARGS} -eq $null) {
    $BPROTO_CI_CMAKE_EXTRA_ARGS = ""
}

# Download all Conan packages
if ((Test-Path "${env:CI_PROJECT_DIR}\\conanfile.py") -or (Test-Path "${env:CI_PROJECT_DIR}\\conanfile.txt")) {
    conan install . -if build -of build --build=missing `
        -pr:b default `
        -pr:h default `
        -s build_type="${env:BPROTO_CI_CMAKE_BUILD_TYPE}"

    if (${LastExitCode} -ne 0) {
        exit ${LastExitCode}
    }

    # Conan will generate this for us.
    $BPROTO_CI_CMAKE_TOOLCHAIN_FILE = "-DCMAKE_TOOLCHAIN_FILE='conan_toolchain.cmake'"
} else {
    New-Item -ItemType Directory -Force build | Out-Null
}

# Display current setup
Write-Host "BPROTO_CI_CMAKE_BUILD_TYPE:`n`t${env:BPROTO_CI_CMAKE_BUILD_TYPE}"
Write-Host "BPROTO_CI_CMAKE_ARGS:`n`t${BPROTO_CI_CMAKE_ARGS}"
Write-Host "BPROTO_CI_CMAKE_EXTRA_ARGS:`n`t${BPROTO_CI_CMAKE_EXTRA_ARGS}"
Write-Host "BPROTO_CI_CMAKE_TOOLCHAIN_FILE:`n`t${BPROTO_CI_CMAKE_TOOLCHAIN_FILE}"

# Configure CMake project
cmake -S. -Bbuild `
    ${BPROTO_CI_CMAKE_ARGS}.Split(' ') `
    ${BPROTO_CI_CMAKE_EXTRA_ARGS}.Split(' ') `
    ${BPROTO_CI_CMAKE_TOOLCHAIN_FILE} `
        *>&1 | % ToString | Tee-Object build/ConfigurationLog.txt

if (${LastExitCode} -ne 0) {
    exit ${LastExitCode}
}

# Build CMake project
cmake `
    --build build `
    --config "${env:BPROTO_CI_CMAKE_BUILD_TYPE}" `
    --parallel "$env:NUMBER_OF_PROCESSORS" `
        *>&1 | % ToString | Tee-Object build/CompilationLog.txt

if (${LastExitCode} -ne 0) {
    exit ${LastExitCode}
}
