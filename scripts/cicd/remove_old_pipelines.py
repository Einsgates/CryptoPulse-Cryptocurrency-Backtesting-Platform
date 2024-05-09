#!/usr/bin/env python3

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

import logging
import os
import sys
from argparse import ArgumentParser
from datetime import datetime
from http import HTTPStatus
from typing import Any

import dateparser
import requests

#
# Main Logic
#


# Based on https://docs.gitlab.com/ee/api/pipelines.html.
# Based on https://stackoverflow.com/questions/53355578.
# Rate limits https://docs.gitlab.com/ee/user/gitlab_com/index.html#gitlabcom-specific-rate-limits.
def _remove_pipelines(
    base_url: str, token: str, per_page: int, remove_before: datetime
) -> bool:
    if not remove_before:
        logging.error(
            f'Invalid value ({remove_before}) for "remove_before" argument has been passed!'
        )
        raise ValueError('Invalid value for "remove_before"!')

    pipelines_list_params = {
        "per_page": per_page,
        "scope": "finished",
        "sort": "asc",
        "updated_before": remove_before.strftime("%Y-%m-%dT%H:%M:%SZ"),
    }

    with requests.Session() as s:
        s.headers.update({"PRIVATE-TOKEN": token})
        while True:
            response = s.get(base_url, params=pipelines_list_params)
            if not response.ok:
                logging.error(
                    f'Querying for pipelines failed because of "{response.reason}" '
                    f"reason for {response.url}!"
                )
                if response.status_code == HTTPStatus.TOO_MANY_REQUESTS:
                    logging.error(
                        "Rate Limits have been reached, wait and try again later!"
                    )
                raise RuntimeError("Failed to query pipelines!")

            pipelines = response.json()
            if not pipelines:
                logging.info("No more pipelines found, exiting.")
                return

            for pipeline in pipelines:
                pipeline_id = pipeline.get("id")
                response = s.delete(f"{base_url}/{pipeline_id}")
                if response.ok:
                    logging.info(
                        f"Pipeline with {pipeline_id} ID successfully deleted."
                    )
                    continue

                logging.warn(
                    f"Deleting pipeline with {pipeline_id} ID failed because of "
                    f'{response.reason}" reason for {response.url}!'
                )
                if response.status_code == HTTPStatus.TOO_MANY_REQUESTS:
                    logging.error(
                        "Rate Limits have been reached, wait and try again later!"
                    )
                    raise RuntimeError("Rate Limits have been reached!")


#
# Logging
#


class LogFormatter(logging.Formatter):
    COLOR_CODES = {
        logging.CRITICAL: "\033[1;35m",  # bright/bold magenta
        logging.ERROR: "\033[1;31m",  # bright/bold red
        logging.WARNING: "\033[1;33m",  # bright/bold yellow
        logging.INFO: "\033[0;37m",  # white / light gray
        logging.DEBUG: "\033[1;30m",  # bright/bold black / dark gray
    }

    RESET_CODE = "\033[0m"

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)

    def format(self, record, *args, **kwargs):  # noqa: A003
        if record.levelno in self.COLOR_CODES:
            record.color_on = self.COLOR_CODES[record.levelno]
            record.color_off = self.RESET_CODE
        else:
            record.color_on = ""
            record.color_off = ""
        return super().format(record, *args, **kwargs)


def _setup_logging(console_log_level) -> None:
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)

    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setLevel(console_log_level.upper())

    console_formatter = LogFormatter(
        fmt="%(color_on)s[%(asctime)s] [%(levelname)-8s] %(message)s%(color_off)s"
    )
    console_handler.setFormatter(console_formatter)

    logger.addHandler(console_handler)


#
# Command Line Arguments
#


def _environ_or_required(key: str, default: str = None) -> dict[str, Any]:
    # Based on https://stackoverflow.com/questions/10551117.
    if os.getenv(key, default):
        return {"default": os.getenv(key, default)}

    return {"required": True}


def _get_cli_arg_parser() -> ArgumentParser:
    parser = ArgumentParser(description="Remove old GitLab CI pilelines")

    api_group = parser.add_argument_group("Required API options")
    api_group.add_argument(
        "-a",
        "--api-root-url",
        type=str,
        help="The GitLab API v4 root URL",
        **_environ_or_required("CI_API_V4_URL", "https://gitlab.com/api/v4/"),
    )
    api_group.add_argument(
        "-i",
        "--project-id",
        type=int,
        help="The ID or URL-encoded path of the project",
        **_environ_or_required("CI_PROJECT_ID"),
    )
    api_group.add_argument(
        "-t",
        "--token",
        type=str,
        help="A token to authenticate with certain API endpoints",
        **_environ_or_required("BPROTO_CI_PRIVATE_API_TOKEN"),
    )

    request_group = parser.add_argument_group("Optional parameters for API calls")
    request_group.add_argument(
        "--per-page",
        type=int,
        help="A number of records to return in one request (max 100)",
        default=100,
    )
    request_group.add_argument(
        "--remove-before",
        type=str,
        help="Remove pipelines added before the specified time period of date.",
        default="1 week",
    )

    parser.add_argument(
        "--log_level",
        help="Set this script log level.",
        choices=["debug", "info", "warning", "error", "critical"],
        default="info",
    )

    return parser


if __name__ == "__main__":
    parser = _get_cli_arg_parser()
    args = parser.parse_args()

    _setup_logging(args.log_level)

    base_url = f"{args.api_root_url}/projects/{args.project_id}/pipelines"
    remove_before = dateparser.parse(args.remove_before)

    try:
        _remove_pipelines(base_url, args.token, args.per_page, remove_before)
    except (RuntimeError, ValueError):
        exit(1)
