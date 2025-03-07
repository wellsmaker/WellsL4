#!/usr/bin/env python3
#
# Copyright (c) 2017 Intel Corporation
#
# SPDX-License-Identifier: Apache-2.0

"""
Script to generate system call invocation macros

This script parses the system call metadata JSON file emitted by
parse_syscalls.py to create several files:

- A file containing weak aliases of any potentially unimplemented system calls,
  as well as the system call dispatch table, which maps system call type IDs
  to their handler functions.

- A header file defing the system call type IDs, as well as function
  prototypes for all system call handler functions.

- A directory containing header files. Each header corresponds to a header
  that was identified as containing system call declarations. These
  generated headers contain the inline invocation functions for each system
  call in that header.
"""

import sys
import re
import argparse
import os
import json

types64 = ["s64_t", "u64_t"]

# The kernel linkage is complicated.  These functions from
# userspace_handlers.c are present in the kernel .a library after
# userspace.c, which contains the weak fallbacks defined here.  So the
# linker finds the weak one first and stops searching, and thus won't
# see the real implementation which should override.  Yet changing the
# order runs afoul of a comment in CMakeLists.txt that the order is
# critical.  These are core syscalls that won't ever be unconfigured,
# just disable the fallback mechanism as a simple workaround.
noweak = ["handle_space_control","handle_processor_control","handle_system_clock",
        "handle_unmap_page","handle_exchange_ipc","handle_kernel_interface",
        "handle_exchange_registers","handle_thread_control","handle_switch_thread",
        "handle_schedule_control","handle_uprintk_string_out","handle_device_binding",
        "handle_dobject_alloc","handle_dobject_free","handle_kobject_access_grant",
        "handle_kobject_access_revoke","handle_retype_untyped"]

table_template = """/* auto-generated by gen_syscalls.py, don't edit */

/* Weak handler functions that get replaced by the real ones unless a system
 * call is not implemented due to kernel configuration.
 */
%s

const syscall_handler_t k_syscall_table[K_SYSCALL_NUM] = {
\t%s
};
"""

list_template = """
/* auto-generated by gen_syscalls.py, don't edit */
#ifndef WELLSL4_SYSCALL_LIST_H
#define WELLSL4_SYSCALL_LIST_H

%s

#ifndef _ASMLANGUAGE

#include <sys/stdint.h>

#endif /* _ASMLANGUAGE */

#endif /* WELLSL4_SYSCALL_LIST_H */
"""

syscall_template = """
/* auto-generated by gen_syscalls.py, don't edit */
%s

#ifndef _ASMLANGUAGE

#include <syscall_list.h>
#include <api/syscall.h>

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#endif

#ifdef __GNUC__
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

#ifdef __cplusplus
extern "C" {
#endif

%s

#ifdef __cplusplus
}
#endif

#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif

#endif
#endif /* include guard */
"""

handler_template = """
extern uintptr_t hdlr_%s(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
                uintptr_t arg4, uintptr_t arg5, uintptr_t arg6, void *ssf);
"""

weak_template = """
__weak ALIAS_OF(handle_reserved_handler)
uintptr_t %s(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3,
         uintptr_t arg4, uintptr_t arg5, uintptr_t arg6, void *ssf);
"""

#

typename_regex = re.compile(r'(.*?)([A-Za-z0-9_]+)$')


class SyscallParseException(Exception):
    pass


def typename_split(item):
    if "[" in item:
        raise SyscallParseException(
            "Please pass arrays to syscalls as pointers, unable to process '%s'" %
            item)

    if "(" in item:
        raise SyscallParseException(
            "Please use typedefs for function pointers")

    mo = typename_regex.match(item)
    if not mo:
        raise SyscallParseException("Malformed system call invocation")

    m = mo.groups()
    return (m[0].strip(), m[1])

def need_split(argtype):
    return (not args.long_registers) and (argtype in types64)

# Note: "lo" and "hi" are named in little endian conventions,
# but it doesn't matter as long as they are consistently
# generated.
def union_decl(type):
    return "union { struct { uintptr_t lo, hi; } split; %s val; }" % type

def wrapper_defs(func_name, func_type, args):
    ret64 = need_split(func_type)
    mrsh_args = [] # List of rvalue expressions for the marshalled invocation
    split_args = []
    nsplit = 0
    for argtype, argname in args:
        if need_split(argtype):
            split_args.append((argtype, argname))
            mrsh_args.append("parm%d.split.lo" % nsplit)
            mrsh_args.append("parm%d.split.hi" % nsplit)
            nsplit += 1
        else:
            mrsh_args.append("*(uintptr_t *)&" + argname)

    if ret64:
        mrsh_args.append("(uintptr_t)&ret64")

    decl_arglist = ", ".join([" ".join(argrec) for argrec in args])

    wrap = "extern %s syscall_%s(%s);\n" % (func_type, func_name, decl_arglist)
    wrap += "static inline %s %s(%s)\n" % (func_type, func_name, decl_arglist)
    wrap += "{\n"
    wrap += "#ifdef CONFIG_USERSPACE\n"
    wrap += ("\t" + "u64_t ret64;\n") if ret64 else ""
    wrap += "\t" + "if (syscall_trap()) {\n"

    for parmnum, rec in enumerate(split_args):
        (argtype, argname) = rec
        wrap += "\t\t%s parm%d;\n" % (union_decl(argtype), parmnum)
        wrap += "\t\t" + "parm%d.val = %s;\n" % (parmnum, argname)

    if len(mrsh_args) > 6:
        wrap += "\t\t" + "uintptr_t more[] = {\n"
        wrap += "\t\t\t" + (",\n\t\t\t".join(mrsh_args[5:])) + "\n"
        wrap += "\t\t" + "};\n"
        mrsh_args[5:] = ["(uintptr_t) &more"]

    syscall_id = "K_SYSCALL_" + func_name.upper()
    invoke = ("arch_syscall_invoke%d(%s)"
              % (len(mrsh_args),
                 ", ".join(mrsh_args + [syscall_id])))

    if ret64:
        wrap += "\t\t" + "(void)%s;\n" % invoke
        wrap += "\t\t" + "return (%s)ret64;\n" % func_type
    elif func_type == "void":
        wrap += "\t\t" + "%s;\n" % invoke
        wrap += "\t\t" + "return;\n"
    else:
        wrap += "\t\t" + "return (%s) %s;\n" % (func_type, invoke)

    wrap += "\t" + "}\n"
    wrap += "#endif\n"

    # Otherwise fall through to direct invocation of the impl func.
    # Note the compiler barrier: that is required to prevent code from
    # the impl call from being hoisted above the check for user
    # context.
    impl_arglist = ", ".join([argrec[1] for argrec in args])
    impl_call = "syscall_%s(%s)" % (func_name, impl_arglist)
    wrap += "\t" + "compiler_barrier();\n"
    wrap += "\t" + "%s%s;\n" % ("return " if func_type != "void" else "",
                               impl_call)

    wrap += "}\n"

    return wrap

# Returns an expression for the specified (zero-indexed!) marshalled
# parameter to a syscall, with handling for a final "more" parameter.
def mrsh_rval(mrsh_num, total):
    if mrsh_num < 5 or total <= 6:
        return "arg%d" % mrsh_num
    else:
        return "(((uintptr_t *)more)[%d])" % (mrsh_num - 5)

def marshall_defs(func_name, func_type, args):
    mrsh_name = "handle_" + func_name

    nmrsh = 0        # number of marshalled uintptr_t parameter
    vrfy_parms = []  # list of (arg_num, mrsh_or_parm_num, bool_is_split)
    split_parms = [] # list of a (arg_num, mrsh_num) for each split
    for i, (argtype, _) in enumerate(args):
        if need_split(argtype):
            vrfy_parms.append((i, len(split_parms), True))
            split_parms.append((i, nmrsh))
            nmrsh += 2
        else:
            vrfy_parms.append((i, nmrsh, False))
            nmrsh += 1

    # Final argument for a 64 bit return value?
    if need_split(func_type):
        nmrsh += 1

    decl_arglist = ", ".join([" ".join(argrec) for argrec in args])
    mrsh = "extern %s syscall_%s(%s);\n" % (func_type, func_name, decl_arglist)

    mrsh += "uintptr_t %s(uintptr_t arg0, uintptr_t arg1, uintptr_t arg2,\n" % mrsh_name
    if nmrsh <= 6:
        mrsh += "\t\t" + "uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, void *ssf)\n"
    else:
        mrsh += "\t\t" + "uintptr_t arg3, uintptr_t arg4, void *more, void *ssf)\n"
    mrsh += "{\n"
    mrsh += "\t" + "_current_cpu->syscall_frame_point = ssf;\n"

    for unused_arg in range(nmrsh, 6):
        mrsh += "\t(void) arg%d;\t/* unused */\n" % unused_arg

    if nmrsh > 6:
        mrsh += ("\tSYSCALL_OOPS(SYSCALL_MEMORY_READ(more, "
                 + str(nmrsh - 6) + " * sizeof(uintptr_t)));\n")

    for i, split_rec in enumerate(split_parms):
        arg_num, mrsh_num = split_rec
        arg_type = args[arg_num][0]
        mrsh += "\t%s parm%d;\n" % (union_decl(arg_type), i)
        mrsh += "\t" + "parm%d.split.lo = %s;\n" % (i, mrsh_rval(mrsh_num,
                                                                 nmrsh))
        mrsh += "\t" + "parm%d.split.hi = %s;\n" % (i, mrsh_rval(mrsh_num + 1,
                                                                 nmrsh))
    # Finally, invoke the verify function
    out_args = []
    for i, argn, is_split in vrfy_parms:
        if is_split:
            out_args.append("parm%d.val" % argn)
        else:
            out_args.append("*(%s*)&%s" % (args[i][0], mrsh_rval(argn, nmrsh)))

    vrfy_call = "syscall_%s(%s)\n" % (func_name, ", ".join(out_args))

    if func_type == "void":
        mrsh += "\t" + "%s;\n" % vrfy_call
        mrsh += "\t" + "return 0;\n"
    else:
        mrsh += "\t" + "%s ret = %s;\n" % (func_type, vrfy_call)
        if need_split(func_type):
            ptr = "((u64_t *)%s)" % mrsh_rval(nmrsh - 1, nmrsh)
            mrsh += "\t" + "SYSCALL_OOPS(SYSCALL_MEMORY_WRITE(%s, 8));\n" % ptr
            mrsh += "\t" + "*%s = ret;\n" % ptr
            mrsh += "\t" + "return 0;\n"
        else:
            mrsh += "\t" + "return (uintptr_t) ret;\n"

    mrsh += "}\n"

    return mrsh, mrsh_name

def analyze_fn(match_group):
    func, args = match_group

    try:
        if args == "void":
            args = []
        else:
            args = [typename_split(a.strip()) for a in args.split(",")]

        func_type, func_name = typename_split(func)
    except SyscallParseException:
        sys.stderr.write("In declaration of %s\n" % func)
        raise

    sys_id = "K_SYSCALL_" + func_name.upper()

    marshaller = None
    marshaller, handler = marshall_defs(func_name, func_type, args)
    invocation = wrapper_defs(func_name, func_type, args)

    # Entry in k_syscall_table
    table_entry = "[%s] = %s" % (sys_id, handler)

    return (handler, invocation, marshaller, sys_id, table_entry)

def parse_args():
    global args
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)

    parser.add_argument("-i", "--json-file", required=True,
                        help="Read syscall information from json file")
    parser.add_argument("-d", "--syscall-dispatch", required=True,
                        help="output C system call dispatch table file")
    parser.add_argument("-l", "--syscall-list", required=True,
                        help="output C system call list header")
    parser.add_argument("-o", "--base-output", required=True,
                        help="Base output directory for syscall macro headers")
    parser.add_argument("-s", "--split-type", action="append",
                        help="A long type that must be split/marshalled on 32-bit systems")
    parser.add_argument("-x", "--long-registers", action="store_true",
                        help="Indicates we are on system with 64-bit registers")
    args = parser.parse_args()


def main():
    parse_args()

    if args.split_type is not None:
        for t in args.split_type:
            types64.append(t)

    with open(args.json_file, 'r') as fd:
        syscalls = json.load(fd)

    invocations = {}
    mrsh_defs = {}
    mrsh_includes = {}
    ids = []
    table_entries = []
    handlers = []

    for match_group, fn in syscalls:
        handler, inv, mrsh, sys_id, entry = analyze_fn(match_group)

        if fn not in invocations:
            invocations[fn] = []

        invocations[fn].append(inv)
        ids.append(sys_id)
        table_entries.append(entry)
        handlers.append(handler)

        if mrsh:
            syscall = typename_split(match_group[0])[1]
            mrsh_defs[syscall] = mrsh
            mrsh_includes[syscall] = "#include <syscalls/%s>" % fn

    with open(args.syscall_dispatch, "w") as fp:
        table_entries.append("[K_SYSCALL_INVAILD] = handle_invaild_handler")

        weak_defines = "".join([weak_template % name
                                for name in handlers
                                if not name in noweak])

        # The "noweak" ones just get a regular declaration
        weak_defines += "\n".join(["extern uintptr_t %s(uintptr_t arg1, uintptr_t arg2, uintptr_t arg3, uintptr_t arg4, uintptr_t arg5, uintptr_t arg6, void *ssf);"
                                   % s for s in noweak])

        fp.write(table_template % (weak_defines,
                                   ",\n\t".join(table_entries)))

    # Listing header emitted to stdout
    ids.sort()
    ids.extend(["K_SYSCALL_INVAILD", "K_SYSCALL_NUM"])

    ids_as_defines = ""
    for i, item in enumerate(ids):
        ids_as_defines += "#define {} {}\n".format(item, i)

    with open(args.syscall_list, "w") as fp:
        fp.write(list_template % ids_as_defines)

    os.makedirs(args.base_output, exist_ok=True)
    for fn, invo_list in invocations.items():
        out_fn = os.path.join(args.base_output, fn)

        ig = re.sub("[^a-zA-Z0-9]", "_", "INCLUDE_SYSCALLS_" + fn).upper()
        include_guard = "#ifndef %s\n#define %s\n" % (ig, ig)
        header = syscall_template % (include_guard, "\n\n".join(invo_list))

        with open(out_fn, "w") as fp:
            fp.write(header)

    # Likewise emit _mrsh.c files for syscall inclusion
    for fn in mrsh_defs:
        mrsh_fn = os.path.join(args.base_output, fn + "_mrsh.c")

        with open(mrsh_fn, "w") as fp:
            fp.write("/* auto-generated by gen_syscalls.py, don't edit */\n")
            fp.write("#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)\n")
            fp.write("#pragma GCC diagnostic push\n")
            fp.write("#endif\n")
            fp.write("#ifdef __GNUC__\n")
            fp.write("#pragma GCC diagnostic ignored \"-Wstrict-aliasing\"\n")
            fp.write("#endif\n")
            fp.write(mrsh_includes[fn] + "\n")
            fp.write("\n")
            fp.write(mrsh_defs[fn] + "\n")
            fp.write("#if __GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6)\n")
            fp.write("#pragma GCC diagnostic pop\n")
            fp.write("#endif\n")

if __name__ == "__main__":
    main()
