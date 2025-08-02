#!/usr/bin/env python3

# This is an example program that uses ura IPC to manage scratchpad.
# Copyright (c) 2025 levinion

import subprocess
import argparse
import sys


def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest="command")
    subparsers.add_parser("list")
    activate_parser = subparsers.add_parser("activate")
    activate_parser.add_argument("index", type=int)
    args = parser.parse_args(sys.argv[1:])
    match args.command:
        case "list":
            r = subprocess.check_output(
                "uracil -c 'local scratchpad = ura.ws.get(-1) assert(scratchpad) for _, toplevel in ipairs(scratchpad.windows) do print(toplevel.index, toplevel.app_id, toplevel.title) end'",
                shell=True,
                text=True,
            ).strip()
            print(r)
        case "activate":
            subprocess.call(
                f"uracil -c 'ura.win.activate(-1,{args.index})'",
                shell=True,
            )


if __name__ == "__main__":
    main()
