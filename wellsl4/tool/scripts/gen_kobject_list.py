#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0
"""
Script to generate gperf tables of kernel object metadata

User mode threads making system calls reference kernel objects by memory
address, as the kernel/driver APIs in WellL4 are the same for both user
and supervisor contexts. It is necessary for the kernel to be able to
validate accesses to kernel objects to make the following assertions:

    - That the memory address points to a kernel object

    - The kernel object is of the expected type for the API being invoked

    - The kernel object is of the expected initialization state

    - The calling thread has sufficient permissions on the object

For more details see the "Kernel Objects" section in the documentation.

The wellsl4 build generates an intermediate ELF binary, wellsl4_prebuilt.elf,
which this script scans looking for kernel objects by examining the DWARF
debug information to look for instances of data structures that are considered
kernel objects. For device drivers, the API struct pointer populated at build
time is also examined to disambiguate between various device driver instances
since they are all 'struct device'.

This script can generate five different output files:

    - A gperf script to generate the hash table mapping kernel object memory
      addresses to kernel object metadata, used to track permissions,
      object type, initialization state, and any object-specific data.

    - A header file containing generated macros for validating driver instances
      inside the system call handlers for the driver subsystem APIs.

    - A code fragment included by kernel.h with one enum constant for
      each kernel object type and each driver instance.

    - The inner cases of a switch/case C statement, included by
      kernel/userspace.c, mapping the kernel object types and driver
      instances to their human-readable representation in the
      otype_to_str() function.

    - The inner cases of a switch/case C statement, included by
      kernel/userspace.c, mapping kernel object types to their sizes.
      This is used for allocating instances of them at runtime
      (CONFIG_DYNAMIC_OBJECTS) in the obj_size_get() function.
"""

import sys
import argparse
import math
import os
import struct
from elf_helper import ElfHelper, kobject_to_enum

from collections import OrderedDict

# Keys in this dictionary are structs which should be recognized as kernel
# objects. Values are a tuple:
#
#  - The first item is None, or the name of a Kconfig that
#    indicates the presence of this object's definition in case it is not
#    available in all configurations.
#
#  - The second item is a boolean indicating whether it is permissible for
#    the object to be located in user-accessible memory.

# Regular dictionaries are ordered only with Python 3.6 and
# above. Good summary and pointers to official documents at:
# https://stackoverflow.com/questions/39980323/are-dictionaries-ordered-in-python-3-6
kobjects = OrderedDict([
    ("thread_stack", (None, False)),
    ("device", (None, False))
])

subsystems = [
# driver api
]

header = """%compare-lengths
%define lookup-function-name kobject_lookup
%language=ANSI-C
%global-table
%struct-type
%{
#include <kernel_object.h>
#include <toolchain.h>
#include <sys/string.h>
%}
struct k_object;
typedef void (*ks_wordlist_cb_func_t)(struct k_object *ko, void *ctx);
"""

# Different versions of gperf have different prototypes for the lookup
# function, best to implement the wrapper here. The pointer value itself is
# turned into a string, we told gperf to expect binary strings that are not
# NULL-terminated.
footer = """%%
struct k_object *k_object_gperf_find(void *obj)
{
    return kobject_lookup((const char *)obj, sizeof(void *));
}

void k_object_gperf_wordlist_foreach(ks_wordlist_cb_func_t func, void *ctx)
{
    int i;

    for (i = MIN_HASH_VALUE; i <= MAX_HASH_VALUE; i++) {
        if (wordlist[i].name != NULL) {
            func(&wordlist[i], ctx);
        }
    }
}

struct k_object *ks_object_find(void *obj)
	ALIAS_OF(k_object_gperf_find);

void ks_object_wordlist_foreach(ks_wordlist_cb_func_t func, void *context)
	ALIAS_OF(k_object_gperf_wordlist_foreach);

"""

def write_gperf_table(fp, eh, objs, static_begin, static_end):
    fp.write(header)

    fp.write("%%\n")
    # Setup variables for mapping thread indexes
    syms = eh.get_symbols()

    for obj_addr, ko in objs.items():
        obj_type = ko.type_name
        # pre-initialized objects fall within this memory range, they are
        # either completely initialized at build time, or done automatically
        # at boot during some PRE_KERNEL_* phase
        #common ram
        initialized = static_begin <= obj_addr < static_end
        #subsystem
        is_driver = obj_type.startswith("K_OBJ_DRIVER_")

        if "CONFIG_64BIT" in syms:
            format_code = "Q"
        else:
            format_code = "I"

        if eh.little_endian:
            endian = "<"
        else:
            endian = ">"

        byte_str = struct.pack(endian + format_code, obj_addr)
        fp.write("\"")
        for byte in byte_str:
            val = "\\x%02x" % byte
            fp.write(val)

        flag = "0"
        if initialized:
            flag += " | obj_allocated_obj"
        if is_driver:
            flag += " | obj_subsystem_obj"

        right = "0"
        fp.write("\", %s, %s, %s, %s\n" % (right, obj_type, flag, str(ko.data)))

    fp.write(footer)

    # Generate the array of already mapped thread indexes
    fp.write('\n')

def write_kobj_types_output(fp):
    fp.write("/* Core kernel objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _ = obj_info
        if kobj == "device":
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write("%s,\n" % kobject_to_enum(kobj))

        if dep:
            fp.write("#endif\n")

    fp.write("/* Driver subsystems */\n")
    for subsystem in subsystems:
        subsystem = subsystem.replace("_driver_api", "").upper()
        fp.write("K_OBJ_DRIVER_%s,\n" % subsystem)


def write_kobj_otype_output(fp):
    fp.write("/* Core kernel objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _ = obj_info
        if kobj == "device":
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write('case %s: ret = "%s"; break;\n' %
                 (kobject_to_enum(kobj), kobj))
        if dep:
            fp.write("#endif\n")

    fp.write("/* Driver subsystems */\n")
    for subsystem in subsystems:
        subsystem = subsystem.replace("_driver_api", "")
        fp.write('case K_OBJ_DRIVER_%s: ret = "%s driver"; break;\n' % (
            subsystem.upper(),
            subsystem
        ))


def write_kobj_size_output(fp):
    fp.write("/* Non device/stack objects */\n")
    for kobj, obj_info in kobjects.items():
        dep, _ = obj_info
        # device handled by default case. Stacks are not currently handled,
        # if they eventually are it will be a special case.
        if kobj in {"device", "thread_stack"}:
            continue

        if dep:
            fp.write("#ifdef %s\n" % dep)

        fp.write('case %s: ret = sizeof(struct %s); break;\n' %
                 (kobject_to_enum(kobj), kobj))
        if dep:
            fp.write("#endif\n")

def parse_args():
    global args

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-k", "--kernel", required=False,
                        help="Input wellsl4 ELF binary")
    parser.add_argument(
        "-g", "--gperf-output", required=False,
        help="Output list of kernel object addresses for gperf use")
    parser.add_argument(
        "-V", "--validation-output", required=False,
        help="Output driver validation macros")
    parser.add_argument(
        "-K", "--kobj-types-output", required=False,
        help="Output k_object enum constants")
    parser.add_argument(
        "-S", "--kobj-otype-output", required=False,
        help="Output case statements for otype_to_str()")
    parser.add_argument(
        "-Z", "--kobj-size-output", required=False,
        help="Output case statements for obj_size_get()")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="Print extra debugging information")
    args = parser.parse_args()
    if "VERBOSE" in os.environ:
        args.verbose = 1


def main():
    parse_args()

    if args.gperf_output:
        assert args.kernel, "--kernel ELF required for --gperf-output"
        eh = ElfHelper(args.kernel, args.verbose, kobjects, subsystems)
        syms = eh.get_symbols()
        objs = eh.find_kobjects(syms)
        if not objs:
            sys.stderr.write("WARNING: zero kobject found in %s\n"
                             % args.kernel)

        with open(args.gperf_output, "w") as fp:
            write_gperf_table(fp, eh, objs,
                              syms["_static_kernel_objects_begin"],
                              syms["_static_kernel_objects_end"])

    if args.kobj_types_output:
        with open(args.kobj_types_output, "w") as fp:
            write_kobj_types_output(fp)

    if args.kobj_otype_output:
        with open(args.kobj_otype_output, "w") as fp:
            write_kobj_otype_output(fp)

    if args.kobj_size_output:
        with open(args.kobj_size_output, "w") as fp:
            write_kobj_size_output(fp)

if __name__ == "__main__":
    main()
