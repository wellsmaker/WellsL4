#!/usr/bin/env python3
#
# Copyright (c) 2018 Henrik Brix Andersen <henrik@brixandersen.dk>
#
# SPDX-License-Identifier: Apache-2.0

import argparse
import os
import sys

from PIL import ImageFont
from PIL import Image
from PIL import ImageDraw

PRINTABLE_MIN = 32
PRINTABLE_MAX = 126

def generate_element(image, charcode):
    """Generate CFB font element for a given character code from an image"""
    blackwhite = image.convert("1", dither=Image.NONE)
    pixels = blackwhite.load()

    width, height = image.size
    if args.dump:
        blackwhite.save("{}_{}.png".format(args.name, charcode))

    if PRINTABLE_MIN <= charcode <= PRINTABLE_MAX:
        char = " ({:c})".format(charcode)
    else:
        char = ""

    args.output.write("""\t/* {:d}{} */\n\t{{\n""".format(charcode, char))

    for col in range(0, width):
        args.output.write("\t\t")
        for octet in range(0, int(height / 8)):
            value = ""
            for bit in range(0, 8):
                row = octet * 8 + bit
                if pixels[col, row]:
                    value = "0" + value
                else:
                    value = "1" + value
            args.output.write("0x{:02x},".format(int(value, 2)))
        args.output.write("\n")
    args.output.write("\t},\n")

def extract_font_glyphs():
    """Extract font glyphs from a TrueType/OpenType font file"""
    font = ImageFont.truetype(args.input, args.size)

    # Figure out the bounding box for the desired glyphs
    fw_max = 0
    fh_max = 0
    for i in range(args.first, args.last + 1):
        fw, fh = font.getsize(chr(i))
        if fw > fw_max:
            fw_max = fw
        if fh > fh_max:
            fh_max = fh

    # Round the vtiled length up to pack into bytes.
    width = fw_max
    height = 8 * int((fh_max + args.y_offset + 7) / 8)

    # Diagnose inconsistencies with arguments
    if width != args.width:
        raise Exception('text width {} mismatch with -x {}'.format(width, args.width))
    if height != args.height:
        raise Exception('text height {} mismatch with -y {}'.format(height, args.height))

    for i in range(args.first, args.last + 1):
        image = Image.new('1', (width, height), 'white')
        draw = ImageDraw.Draw(image)

        fw, fh = draw.textsize(chr(i), font=font)

        xpos = 0
        if args.center_x:
            xpos = (width - fw) / 2 + 1
        ypos = args.y_offset

        draw.text((xpos, ypos), chr(i), font=font)
        generate_element(image, i)

def extract_image_glyphs():
    """Extract font glyphs from an image file"""
    image = Image.open(args.input)

    x_offset = 0
    for i in range(args.first, args.last + 1):
        glyph = image.crop((x_offset, 0, x_offset + args.width, args.height))
        generate_element(glyph, i)
        x_offset += args.width

def generate_header():
    """Generate CFB font header file"""

    wellsl4_base = os.environ.get('WELLSL4_BASE', "")

    clean_cmd = []
    for arg in sys.argv:
        if arg.startswith("--bindir"):
            # Drop. Assumes --bindir= was passed with '=' sign.
            continue
        if args.bindir and arg.startswith(args.bindir):
            # +1 to also strip '/' or '\' separator
            striplen = min(len(args.bindir)+1, len(arg))
            clean_cmd.append(arg[striplen:])
            continue

        clean_cmd.append(arg.replace(wellsl4_base, '"${WELLSL4_BASE}"'))

    args.output.write("""/*
 * This file was automatically generated using the following command:
 * {cmd}
 *
 */

#include <wellsl4.h>
#include <display/cfb.h>

static const u8_t cfb_font_{name:s}_{width:d}{height:d}[{elem:d}][{b:.0f}] = {{\n"""
                      .format(cmd=" ".join(clean_cmd),
                              name=args.name,
                              width=args.width,
                              height=args.height,
                              elem=args.last - args.first + 1,
                              b=args.width / 8 * args.height))

    if args.type == "font":
        extract_font_glyphs()
    elif args.type == "image":
        extract_image_glyphs()
    elif args.input.name.lower().endswith((".otf", ".otc", ".ttf", ".ttc")):
        extract_font_glyphs()
    else:
        extract_image_glyphs()

    args.output.write("""
}};

FONT_ENTRY_DEFINE({name}_{width}{height},
		  {width},
		  {height},
		  CFB_FONT_MONO_VPACKED,
		  cfb_font_{name}_{width}{height},
		  {first},
		  {last}
);
""" .format(name=args.name, width=args.width, height=args.height,
            first=args.first, last=args.last))

def parse_args():
    """Parse arguments"""
    global args
    parser = argparse.ArgumentParser(
        description="Character Frame Buffer (CFB) font header file generator",
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument(
        "-d", "--dump", action="store_true",
        help="dump generated CFB font elements as images for preview")

    group = parser.add_argument_group("input arguments")
    group.add_argument(
        "-i", "--input", required=True, type=argparse.FileType('rb'), metavar="FILE",
        help="TrueType/OpenType file or image input file")
    group.add_argument(
        "-t", "--type", default="auto", choices=["auto", "font", "image"],
        help="Input file type (default: %(default)s)")

    group = parser.add_argument_group("font arguments")
    group.add_argument(
        "-s", "--size", type=int, default=10, metavar="POINTS",
        help="TrueType/OpenType font size in points (default: %(default)s)")

    group = parser.add_argument_group("output arguments")
    group.add_argument(
        "-o", "--output", type=argparse.FileType('w'), default="-", metavar="FILE",
        help="CFB font header file (default: stdout)")
    group.add_argument(
        "--bindir", type=str,
        help="CMAKE_BINARY_DIR for pure logging purposes. No trailing slash.")
    group.add_argument(
        "-x", "--width", required=True, type=int,
        help="width of the CFB font elements in pixels")
    group.add_argument(
        "-y", "--height", required=True, type=int, choices=range(8, 128, 8),
        help="height of the CFB font elements in pixels")
    group.add_argument(
        "-n", "--name", default="custom",
        help="name of the CFB font entry (default: %(default)s)")
    group.add_argument(
        "--first", type=int, default=PRINTABLE_MIN, metavar="CHARCODE",
        help="character code mapped to the first CFB font element (default: %(default)s)")
    group.add_argument(
        "--last", type=int, default=PRINTABLE_MAX, metavar="CHARCODE",
        help="character code mapped to the last CFB font element (default: %(default)s)")
    group.add_argument(
        "--center-x", action='store_true',
        help="center character glyphs horizontally")
    group.add_argument(
        "--y-offset", type=int, default=0,
        help="vertical offset for character glyphs (default: %(default)s)")

    args = parser.parse_args()

def main():
    """Parse arguments and generate CFB font header file"""
    parse_args()
    generate_header()

if __name__ == "__main__":
    main()
