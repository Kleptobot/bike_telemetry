"""Tune Adafruit TinyUSB for USB-Mass-Storage over SD (nRF52840).

The nRF52840 port runs the whole TinyUSB stack (including the MSC read10/
write10 callbacks) on a dedicated FreeRTOS task created with a 200-word stack.
The default is far too small to hold a synchronous SdFat/SPI call chain, which
overflows and hard-crashes the MCU once the host starts reading the SD volume.
The MSC endpoint buffer is also only 512 bytes, so a Windows READ10 (which
issues up to 8 sectors per command) is throttled to one sector at a time -- the
reason the volume takes seconds to appear.

This script edits the two files in the vendored platform package and is
idempotent: running it again (e.g. after `platformio` re-installs the package)
simply re-applies the same values.

Usage:
    python tools/patch_tinyusb_stack.py [package_dir]

Default package_dir is the adafruit nRF52 Arduino core package under
~/.platformio/packages.
"""

import os
import re
import sys

# Replacement pairs: (regex, replacement string).
PATCHES = [
    # 1. Raise the USB task stack so synchronous SD/SPI cannot overflow it.
    #    200 words -> 1536 words (6 KB). Only the USB task lives on it.
    (r"USBD_STACK_SZ\s+\(200\)", "USBD_STACK_SZ (1536)"),
    # 2. Widen the MSC endpoint buffer so READ10 can move 8 sectors per call.
    #    512 -> 4096; the class driver sizes its static buffer from this and
    #    caps each read10/write10 callback to it, so bigger = fewer SPI calls.
    (r"CFG_TUD_MSC_EP_BUFSIZE\s+512", "CFG_TUD_MSC_EP_BUFSIZE 4096"),
]

FILES = [
    "Adafruit_TinyUSB_nrf.cpp",
    "tusb_config_nrf.h",
]


def default_nrf_dir():
    home = os.path.expanduser("~")
    return os.path.join(
        home,
        ".platformio",
        "packages",
        "framework-arduinoadafruitnrf52",
        "libraries",
        "Adafruit_TinyUSB_Arduino",
        "src",
        "arduino",
        "ports",
        "nrf",
    )


def main():
    nrf_dir = sys.argv[1] if len(sys.argv) > 1 else default_nrf_dir()
    if not os.path.isdir(nrf_dir):
        sys.stderr.write(
            f"TinyUSB nRF port not found at '{nrf_dir}'. "
            "Pass package_dir explicitly.\n"
        )
        return 1

    for fname in FILES:
        path = os.path.join(nrf_dir, fname)
        if not os.path.isfile(path):
            sys.stderr.write(f"Missing '{path}'\n")
            return 1

        with open(path, "r", encoding="utf-8") as fh:
            text = fh.read()

        if fname == "Adafruit_TinyUSB_nrf.cpp":
            pat, rep = PATCHES[0]
        else:
            pat, rep = PATCHES[1]

        if re.search(pat, text):
            text = re.sub(pat, rep, text)
            with open(path, "w", encoding="utf-8") as fh:
                fh.write(text)
            print(f"patched {fname}: {pat!r} -> {rep!r}")
        else:
            print(f"skipped {fname}: pattern {pat!r} already applied")

    print("done")
    return 0


if __name__ == "__main__":
    sys.exit(main())