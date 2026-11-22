import argparse
import json
import sys
import tomllib
from pathlib import Path

import inject


def parse_step(text):
    return tomllib.loads(f"s = {{ {text} }}")["s"]


def main(argv=None):
    parser = argparse.ArgumentParser(prog="touch-inject")
    parser.add_argument("--pid", type=int, required=True)
    parser.add_argument("--settings", type=Path)
    sub = parser.add_subparsers(dest="command", required=True)
    info = sub.add_parser("info")
    run = sub.add_parser("run")
    run.add_argument("scenario", type=Path, nargs="?")
    run.add_argument("--step", action="append", default=[])
    run.add_argument("--out", type=Path, required=True)
    run.add_argument("--log", type=Path)
    shot = sub.add_parser("capture")
    shot.add_argument("file", type=Path)
    sub.add_parser("restore")
    args = parser.parse_args(argv)

    settings = inject.load_settings(args.settings)
    inject.make_dpi_aware()
    if args.command == "info":
        target = inject.Target.of_pid(args.pid)
        print(json.dumps({"digitizer": inject.digitizer_info(), "target": target.info(), "foreground": target.is_foreground()}))
        return 0
    if args.command == "capture":
        print(json.dumps(inject.Target.of_pid(args.pid).capture(args.file)))
        return 0
    if args.command == "restore":
        print(json.dumps({"restored": inject.Target.of_pid(args.pid).restore()}))
        return 0
    steps = []
    if args.scenario:
        with open(args.scenario, "rb") as f:
            steps.extend(tomllib.load(f)["step"])
    steps.extend(parse_step(s) for s in args.step)
    status, events = inject.run_scenario(args.pid, steps, args.out, settings, echo=lambda r: print(json.dumps(r, ensure_ascii=False)))
    if args.log:
        inject.write_log(args.log, events)
    for e in events:
        if "cancelled" in e or "cancel_failed" in e:
            print(json.dumps(e))
    print(status)
    return 0 if status == "ok" else 3


if __name__ == "__main__":
    sys.exit(main())
