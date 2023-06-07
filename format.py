# Copyright (C) 2023 Nicolai Brand (https://lytix.dev)
# 
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import os
import argparse


def format():
    for root, _, files in os.walk("."):
        for file in files:
            if file.endswith((".c", ".h")):
                os.system("clang-format -i -style=file " + root + "/" + file)


def checkstyle():
    fail = False
    for root, _, files in os.walk("."):
        for file in files:
            if file.endswith((".c", ".h")):
                rc = os.system("clang-format --dry-run --Werror -style=file " + root + "/" + file)
                if rc != 0:
                    fail = True

    if fail:
        exit(1)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("-checkstyle",
                        help="returns success (0) if every file already formatted, else fail (1)",
                        action="store_true")
    args = parser.parse_args()

    if args.checkstyle:
        checkstyle()
    else:
        format()


if __name__ == "__main__":
    main()
