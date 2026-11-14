#!/usr/bin/env python3
"""Minimal FIT inspector: strict-parse a .fit file and dump key fields.

Uses fitdecode the way GPXSee/other readers effectively do -- iterate the
messages and let any malformed structure raise -- so a clean run here is good
evidence the file will open elsewhere.
"""
import sys
import fitdecode


def main(path: str) -> int:
    laps = []
    sess = None
    records = 0
    try:
        with fitdecode.FitReader(path) as fit:
            for msg in fit:
                if not isinstance(msg, fitdecode.FitDataMessage):
                    continue
                name = msg.name
                if name == 'session':
                    sess = msg
                elif name == 'lap':
                    laps.append(msg)
                elif name == 'record':
                    records += 1
    except fitdecode.FitParseError as ex:
        print(f"[inspect] PARSE ERROR: {ex}")
        return 1

    print(f"[inspect] OK: {records} records, {len(laps)} laps")
    if sess is not None:
        for field in ('total_elapsed_time', 'total_timer_time', 'start_time'):
            v = sess.get_value(field)
            print(f"[inspect] SESSION {field} = {v}")
    else:
        print("[inspect] no SESSION message")
    for i, lap in enumerate(laps):
        et = lap.get_value('total_elapsed_time')
        d = lap.get_value('total_distance')
        print(f"[inspect] LAP{i}: elapsed={et} distance={d}")
    return 0


if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("usage: inspect_fit.py <file.fit>")
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
