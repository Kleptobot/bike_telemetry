#!/usr/bin/env python3
"""Independent FIT validation using fitdecode (a spec-strict third-party parser).

Purpose: the firmware's own FitLogParser agrees with FitWriter by construction --
both were written to the same (possibly wrong) assumptions -- so it CANNOT tell
us whether a .fit file is spec-valid. This tool is the outside opinion, the same
kind of judgement Garmin Connect / GPXSee / fitfiletools make.

Setup (once):
    python -m pip install fitdecode

Usage:
    python sim/tools/fit_validate.py <file.fit> [more.fit ...]

Exit code 0 = every file parsed cleanly, 1 = at least one was rejected.
"""
import sys

try:
    import fitdecode
except ImportError:
    sys.exit("fitdecode is not installed: python -m pip install fitdecode")


def dump(path, verbose=True):
    """Parse one file. Returns (ok, message_count)."""
    try:
        with fitdecode.FitReader(path) as fit:
            n = 0
            for frame in fit:
                if frame.frame_type == fitdecode.FIT_FRAME_DATA:
                    n += 1
                    if verbose:
                        print(f"  {frame.name}({frame.global_mesg_num}):")
                        for f in frame.fields:
                            print(f"      {f.name} = {f.value!r} (raw={f.raw_value!r})")
                elif frame.frame_type == fitdecode.FIT_FRAME_HEADER:
                    if verbose:
                        # Attribute names differ across fitdecode versions, so
                        # report whatever the header object actually exposes
                        # rather than assuming a schema.
                        attrs = [(a, getattr(frame, a, None))
                                 for a in ("header_size", "proto_ver",
                                           "protocol_version", "profile_ver",
                                           "profile_version", "body_size",
                                           "data_size", "crc", "crc_matched")
                                 if hasattr(frame, a)]
                        print("  HEADER: " + ", ".join(f"{k}={v!r}" for k, v in attrs))
            return True, n
    except Exception as exc:  # fitdecode raises FitError subclasses
        return False, f"{type(exc).__name__}: {exc}"


def main(argv):
    if len(argv) < 2:
        sys.exit(__doc__)
    failures = 0
    for path in argv[1:]:
        print(f"=== {path} ===")
        ok, info = dump(path)
        if ok:
            print(f"  -> OK ({info} data messages)")
        else:
            failures += 1
            print(f"  -> REJECTED: {info}")
    print()
    print("ALL FILES VALID" if failures == 0 else f"{failures} FILE(S) REJECTED")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
