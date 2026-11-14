#!/usr/bin/env python3
"""Print the authoritative FIT profile field numbers for the messages the
firmware writes (from fitdecode's bundled Profile data).

Used to check FitWriter/FITLogger field numbers against the real spec instead of
memory -- a wrong field number is silent: the file still parses, but the value
lands in the wrong column (or nowhere) in Garmin Connect / GPXSee.

Usage:
    python sim/tools/fit_profile.py [mesg_num ...]   # default: 18 19 20 34
"""
import sys

try:
    from fitdecode import profile
except ImportError:
    sys.exit("fitdecode is not installed: python -m pip install fitdecode")

DEFAULT = [0, 18, 19, 20, 23, 34]


def main(argv):
    nums = [int(a) for a in argv[1:]] or DEFAULT
    for num in nums:
        try:
            mesg = profile.MESSAGE_TYPES[num]
        except KeyError:
            print(f"global {num}: <not in profile>")
            continue
        print(f"global {num} = {mesg.name}")
        for fnum, fdef in sorted(mesg.fields.items()):
            scale = getattr(fdef, "scale", None)
            units = getattr(fdef, "units", None)
            scale = "" if not scale else f" scale={scale}"
            units = "" if not units else f" units={units}"
            print(f"    {fnum:>4}  {fdef.name}{units}{scale}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
