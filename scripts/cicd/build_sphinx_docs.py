#!/usr/bin/env python3

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

import os
import subprocess
from argparse import ArgumentDefaultsHelpFormatter, ArgumentParser, ArgumentTypeError
from typing import Any, Dict, Text, Union


class _WidthFormatter(ArgumentDefaultsHelpFormatter):
    def __init__(self, prog: Text) -> None:
        super().__init__(prog, width=100)


def _environ_or_required(key: str, default: str = None) -> Dict[str, Any]:
    # Based on https://stackoverflow.com/questions/10551117.
    if value := os.getenv(key, default):
        return {"default": value}

    return {"required": True}


def _str2bool(v: Union[bool, str]) -> bool:
    if isinstance(v, bool):
        return v

    v = v.lower()
    if v in ("1", "on", "yes", "y", "true"):
        return True
    elif v in ("0", "off", "no", "n", "false"):
        return False

    raise ArgumentTypeError("Boolean value expected.")


def _get_cli_args() -> ArgumentParser:
    description = """
    Generate documentation from source files.
    sphinx-build generates documentation from the files in SOURCEDIR and places it in OUTPUTDIR.
    It looks for 'conf.py' in SOURCEDIR for the configuration settings.
    """
    parser = ArgumentParser(description=description, formatter_class=_WidthFormatter)

    parser.add_argument(
        "-s",
        "--source",
        type=str,
        help="Path to documentation source files",
        **_environ_or_required("BPROTO_CI_SPHINX_SOURCE_DIR", "docs"),
    )
    parser.add_argument(
        "-o",
        "--output",
        type=str,
        help="Path to output directory",
        default="build",
    )
    parser.add_argument(
        "-W",
        "--warnings-as-errors",
        type=_str2bool,
        help="Turn warnings into errors",
        **_environ_or_required("BPROTO_CI_SPHINX_WARNINGS_AS_ERRORS", "True"),
        const=True,
        nargs="?",
    )

    return parser.parse_args()


if __name__ == "__main__":
    args = _get_cli_args()
    cmd = " ".join(
        [
            "sphinx-build",
            "-b html",
            "--color",
            "-W" if args.warnings_as_errors else "",
            args.source,
            args.output,
        ]
    )

    try:
        print(f"Executing '{cmd}' command...\n")
        subprocess.run(cmd, check=True, shell=True)
    except Exception:
        print("")
        raise
