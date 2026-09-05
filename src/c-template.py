import argparse
import json
import re
import sys
import copy
import os.path as path
from sys import stderr, stdin
from types import SimpleNamespace
from typing import Any
from warnings import simplefilter

DEFAULT_TEMPLATE = [
    [
        {
            "typename": "int8_t",
            "includes": [
                "<stdint.h>"
            ],
            "short": "i8"
        }
    ],
    [
        {
            "typename": "int16_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "i16"
        }
    ],
    [
        {
            "typename": "int32_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "i32"
        }
    ],
    [
        {
            "typename": "int64_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "i64"
        }
    ],
    [
        {
            "typename": "uint8_t",
            "includes": [
                "<stdint.h>"
            ],
            "short": "u8"
        }
    ],
    [
        {
            "typename": "uint16_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "u16"
        }
    ],
    [
        {
            "typename": "uint32_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "u32"
        }
    ],
    [
        {
            "typename": "uint64_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "u64"
        }
    ],
    [
        {
            "typename": "size_t",
            "includes": [
                "<stddef.h>",
            ],
            "short": "uz"
        }
    ],
    [
        {
            "typename": "ptrdiff_t",
            "includes": [
                "<stddef.h>",
            ],
            "short": "iz"
        }
    ],
    [
        {
            "typename": "intptr_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "iptr"
        }
    ],
    [
        {
            "typename": "uintptr_t",
            "includes": [
                "<stdint.h>",
            ],
            "short": "uptr"
        }
    ],
    [
        {
            "typename": "float",
            "short": "f"
        }
    ],
    [
        {
            "typename": "double",
            "short": "d"
        }
    ],
    [
        {
            "typename": "long double",
            "short": "ld"
        }
    ],
    [
        {
            "typename": "int",
            "short": "i"
        }
    ],
    [
        {
            "typename": "unsigned int",
            "short": "u"
        }
    ],
    [
        {
            "typename": "long",
            "short": "l"
        }
    ],
    [
        {
            "typename": "unsigned long",
            "short": "ul"
        }
    ],
    [
        {
            "typename": "long long",
            "short": "ll"
        }
    ],
    [
        {
            "typename": "unsigned long long",
            "short": "ull"
        }
    ],
    [
        {
            "typename": "void *",
            "short": "ptr"
        }
    ],
    [
        {
            "typename": "bool",
            "includes": [
                "<stdbool.h>"
            ],
            "short": "b"
        }
    ],
    [
        {
            "typename": "char",
            "short": "c"
        }
    ],
    [
        {
            "typename": "wchar_t",
            "short": "wc"
        }
    ]
]

DIR = re.compile(r"//\s*!\s*Template\s+([A-Z]+)\s+(.*)")
TYPEDEF = re.compile(r"\s*typedef\s+(.*)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*;\s*", flags=re.MULTILINE)
DEFINE = re.compile(r"\s*#\s*define\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*(.*)\s*$\s*", flags=re.MULTILINE)

options: dict[str, bool] = {
    "remove-inline": False,
    "mangle-groups": False,
}

def option(opt: Any):
    if isinstance(opt, dict):
        for key, value in opt.items():
            options[key] = value
    elif isinstance(opt, str):
        options[opt] = True
    else:
        print(f"Ignoring unknown option {opt}", file=stderr)

def is_set(opt: str) -> bool:
    return opt in options and options[opt]

def read_directives(source, dir_callback):
    i = 0
    while i < len(source):
        m = DIR.match(source, i)
        if m is None:
            i += 1
            continue
        dir_callback(m)
        i = m.end()

def lineo(m: re.Match[str], src: str):
    col = 1
    line = 1
    for i in range(0, m.start()):
        if src[i] == '\n':
            line += 1
            col = 1
        else:
            col += 1
    return f"{col}:{line}"

def erase_all(source, regions):
    result = ""
    start = 0
    for i, j in regions:
        result += source[start:i]
        start = j

    result += source[start:]
    return result

def parse_types(args, source):

    types = {}
    erase_regions = []

    def parser(m: re.Match[str]):
        if m.group(1) == 'OPTION':
            option(json.loads(m.group(2)))
            return

        if m.group(1) != 'TYPE':
            return
 
        index = int(m.group(2))
        if index > len(args.types):
            print(f"Defaulting unspecified argument {index} to int ({lineo(m, source)})", file=stderr)
            targ = "int"
        else:
            targ = args.types[index - 1]

        typedef = TYPEDEF.search(source, m.end())
        if typedef is None:
            print(f"TYPE directive should annotate a typedef at {lineo(m, source)}", file=stderr)
            return

        erase_regions.append((m.start(), typedef.end()))

        tname = typedef.group(2)
        types[tname] = targ


    read_directives(source, parser)
    return erase_all(source, erase_regions), types

def seek_paren(source, i, oparen, cparen):
    level = 0
    while i < len(source):
        if source[i] == oparen:
            level += 1
        elif source[i] == cparen:
            level -= 1

        if level == 0:
            return i + 1
        i += 1
    return len(source)

COMMANDS = {
    "toupper": lambda params, values: " ".join(params[1:]).upper(),
    "tolower": lambda params, values: " ".join(params[1:]).lower(),
}

def run_cmd(cmd, values):
    params = cmd.split(' ')
    if params[0] in COMMANDS:
        return COMMANDS[params[0]](params, values)
    return ""

def expand_env(source, values):
    i = 0
    subst = ""
    while i < len(source):

        if source[i] != '$':
            subst += source[i]
            i += 1
            continue

        if i + 2 >= len(source):
            subst += source[i]
            i += 1
            continue

        if source[i + 1] == '{':
            end = seek_paren(source, i + 1, '{', '}')
            key = expand_env(source[i + 2 : end - 1], values)
            keys = key.split('.')
            value = values
            for part in keys:
                if part in value:
                    value = value[part]
                else:
                    break
            if isinstance(value, str):
                subst += value
            elif "typename" in value:
                subst += value["typename"]
            i = end
        elif source[i + 1] == '(':
            end = seek_paren(source, i + 1, '(', ')')
            cmd = expand_env(source[i + 2 : end - 1], values)
            subst += run_cmd(cmd, values)
            i = end

    return subst

def parse_mangles(args, types, source):

    mangles = {}
    erase_regions = []

    def parser(m: re.Match[str]):
        if m.group(1) != 'MANGLE':
            return

        pattern = json.loads(m.group(2))

        offset = m.end()
        while True:

            define = DEFINE.search(source, offset)
            if define is None:
                if offset != 0 and is_set("mangle-groups"):
                    break

                print(f"MANGLE directive should annotate a #define at {lineo(m, source)}", file=stderr)
                return

            subst_target = define.group(1)
            subst_base = define.group(2)

            subst_value = expand_env(pattern, {
                "1": subst_base,
                "<": subst_target,
                **types,
            })

            mangles[subst_target] = subst_value
 
            offset = define.end()
            if not is_set("mangle-groups"):
                break

        erase_regions.append((m.start(), offset))

    read_directives(source, parser)
    return erase_all(source, erase_regions), mangles

WHITESPACE = re.compile(r"\s+", flags=re.MULTILINE)
CSTRING = re.compile(r'"(?:[^"]|\\")*"')
CCHAR = re.compile(r"'(?:[^']|\\')*'")
CNAME = re.compile(r"[a-zA-Z_][a-zA-Z_0-9]*")
CPREPROCESSOR = re.compile(r"\s*#.*$", flags=re.MULTILINE)
CCOMMENT = re.compile(r"//.*$", flags=re.MULTILINE)
CKEYWORD = re.compile(r"(?:alignas|alignof|auto|bool|break|case|char|const|constexpr|continue|default|do|double|else|enum|extern|false|float|for|goto|if|inline|int|long|nullptr|register|restrict|return|short|signed|sizeof|static|static_assert|struct|switch|thread_local|true|typedef|typeof|typeof_unqual|union|unsigned|void|volatile|while|_Alignas|_Alignof|_Atomic|_BitInt|_Bool|_Complex|_Decimal128|_Decimal32|_Decimal64|_Generic|_Imaginary|_Noreturn|_Static_assert|_Thread_local)")
CBLOCKC = re.compile(r"/\*.*\*/", flags=re.MULTILINE)
COTHER = re.compile(r"[^\sa-zA-Z_]+")
def token(source: str) -> tuple[str, str, str]:

    if source == "":
        return "NONE", "", ""

    PATTERNS = [
        ("WS", WHITESPACE),
        ("CS",  CSTRING),
        ("CC", CCHAR),
        ("CK", CKEYWORD),
        ("CN", CNAME),
        ("CPP", CPREPROCESSOR),
        ("CCOM", CCOMMENT),
        ("CCOM", CBLOCKC),
        ("CO", COTHER),
    ]

    for kind, pattern in PATTERNS:
        m = re.match(pattern, source)
        if m is not None:
            return kind, m.group(0), source[m.end():]

    return "NONE", "", source

INCLUDE_CPP = re.compile(r'\s*#\s*include\s*((<[^>]+>)|("[^"]+"))\s*', flags=re.MULTILINE)

def substitute(source: str, mangles: dict[str, Any], out_includes: list[str] | None = None) -> str:
    result = ""

    while True:
        kind, tok, next = token(source)
        source = next

        if kind == "CPP" and out_includes is not None:
            m = INCLUDE_CPP.match(tok)
            if m is not None:
                out_includes.append(m.group(1))

        if tok == "":
            result += next
            break

        if kind == "CN" and tok in mangles:
            result += mangles[tok]
            continue

        result += tok

    return result

def str_types(types: dict[str, Any]) -> dict[str, str]:
    results = {}

    for key, val in types.items():
        if val is str:
            results[key] = val
        else:
            results[key] = val["typename"]

    return results

HEADER = re.compile(r"\s*//\s*!\s*Template\s+H\s+(.*)\s*$\s*", flags=re.MULTILINE)
SOURCE = re.compile(r"\s*//\s*!\s*Template\s+C\s+(.*)\s*$\s*", flags=re.MULTILINE)
GUARD = re.compile(r"\s*//\s*!\s*Template\s+GUARD\s+(.*)\s*$\s*", flags=re.MULTILINE)
INCLUDE = re.compile(r"\s*//\s*!\s*Template\s+INCLUDE\s+(.*)\s*$\s*", flags=re.MULTILINE)

def seek_header(args, data, types):

    results = SimpleNamespace()
    results.header_name = args.out + ".h"
    results.source_name = args.out + ".c"
    results.header = data
    results.source = None

    m = HEADER.search(data)
    if m is None:
        return results

    env = {
        '1': args.out,
        '<': args.file,
        **types,
    }

    results.header_name = expand_env(json.loads(m.group(1)), env).replace(" ", "_")

    src = SOURCE.search(data, m.end())
    if src is None:
        results.header = data[m.end():]
        return results

    results.source_name = expand_env(json.loads(src.group(1)), env).replace(" ", "_")

    results.header = data[m.end():src.start()]
    results.source = data[src.end():]

    return results

def indent_print(s: str, indent: str = "    "):
    for line in s.splitlines():
        print(indent + line)

def macro_name(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s).upper()

def get_type_includes(results: SimpleNamespace, types: list[dict], s: str, existing_includes: list[str] | None = None) -> list[str]:
    env = namespace_to_dict(results)

    for tx in types:
        env = {
            **env,
            **tx
        }

    include_list = []
    for t in types:
        if "includes" not in t:
            continue

        for inc in t["includes"]:
            inc = expand_env(inc, env)
            if existing_includes is None or inc not in existing_includes:
                include_list.append(f"#include {inc}")
                if existing_includes is not None:
                    existing_includes.append(inc)
    return include_list

def add_header_guard(args, results: SimpleNamespace, types: dict[str, str], s: str, includes: list[str]) -> str:

    m = GUARD.search(s)
    if m is not None:
        guard_macro = expand_env(json.loads(m.group(1)), {
            '<': args.file,
            **types,
            **namespace_to_dict(results),
        })
        s = s[:m.start()] + s[m.end():]
    else:
        guard_macro = macro_name(args.file)

    return f"""
#if !defined({guard_macro})

// Generated by:
//
// c-template {" ".join(f"{arg!r}" for arg in sys.argv[1:])}
//
//
// ===============================================
// Template implementation of {args.file}
// {"\n// ".join([ f"{key}: {value}" for key, value in types.items() ])}
// ===============================================
//
{f"""// ==== Includes added by template generator ====
{"\n".join(includes)}
// =============================================="""}
//
// ==== BEGIN GENERATED CODE ====
{s}
// ==== END GENERATED CODE ====

#define {guard_macro} 1
#endif // !defined({guard_macro})"""

def namespace_to_dict(ns: Any) -> dict:
    if isinstance(ns, dict):
        return {
            key: value if isinstance(value, str) else namespace_to_dict(value)
                for key, value in ns.items()
        }
    else:
        return {
            key: value if isinstance(value, str) else namespace_to_dict(value)
                for key, value in vars(ns).items()
        }

def add_includes(results: SimpleNamespace, types: dict[str, Any], name: str, s: str) -> str:
    while True:
        m = INCLUDE.search(s)
        if m is None:
            break

        include_path = expand_env(m.group(1), {
            '<': name,
            **types,
            **namespace_to_dict(results),
        })

        s = s[:m.start()] + f'#include {include_path}\n' + s[m.end():]
    return s

def remove_inline(source: str) -> str:

    result = ""
    removed = False

    while True:

        kind, tok, next = token(source)
        source = next

        if removed and kind == "WS":
            removed = False
            continue

        removed = False

        if kind == "CK" and tok == "inline":
            removed = True
            continue

        if tok == "":
            result += next
            break

        result += tok

    return result

def process(args):
    with open(args.file) as f:
        src = f.read()
    existing_includes = []
    data, types = parse_types(args, src)
    data, mangles = parse_mangles(args, types, data)
    data = substitute(data, str_types(types), out_includes=existing_includes)
    data = substitute(data, mangles)

    results = seek_header(args, data, types)

    results.header = add_includes(results, types, results.header_name, results.header)

    header_includes_to_add = get_type_includes(results, args.types, results.header, existing_includes=existing_includes)

    results.header = add_header_guard(args, results, types, results.header, header_includes_to_add)

    with open(f"{args.out}/{results.header_name}", "w") as f:
        print(f"Wrote {args.out}/{results.header_name}", file=stderr)
        f.write(results.header)

    if results.source is not None:
        results.source = add_includes(results, types, results.source_name, results.source)

        if is_set("remove-inline"):
            results.source = remove_inline(results.source)

        with open(f"{args.out}/{results.source_name}", "w") as f:
            print(f"Wrote {args.out}/{results.source_name}", file=stderr)
            f.write(results.source)

def main():
    parser = argparse.ArgumentParser(
        prog="c-template",
        description="""
        A Python-based template generator for C.
        """)
    parser.add_argument("-o",
                        "--output-directory",
                        dest="out",
                        default=".",
                        help="Specify the directory of the generated source file(s)")
    parser.add_argument("-i",
                        "--input-file",
                        dest="file",
                        default="/dev/stdin",
                        help="C source file to generate a template from")
    parser.add_argument("-J",
                        "--template-json",
                        dest="template_json_file",
                        help="JSON template file to parse type arguments from")
    parser.add_argument("--default-template",
                        action="store_true",
                        dest="default_template",
                        help="Use the default template arguments.")
    parser.add_argument("--print-default-template",
                        action="store_true",
                        dest="print_default_template",
                        help="Print the default template arguments (enabled using the --default-template flag) and exit.")
    parser.add_argument("--infer",
                        dest="infer",
                        help="Infer template from the name of the given argument.")
    parser.add_argument("types",
                        nargs="*",
                        help="Positional type arguments to use in the substitution",
                        type=json.loads)
    args = parser.parse_args()

    if args.print_default_template:
        print(json.dumps(DEFAULT_TEMPLATE))
        exit(0)

    if args.template_json_file is not None:
        with open(args.template_json_file) as f:
            for t in json.load(f):
                args.types.append(t)

    if args.default_template:
        for t in DEFAULT_TEMPLATE:
            args.types.append(t)

    if args.infer is not None:
        infer: str = args.infer

        name, _ = path.splitext(path.basename(args.file))
        if name in infer:
            infer = path.basename(infer.replace(name, ""))

        infer = infer.strip('_')
        infer, _ = path.splitext(infer)

        for tx in DEFAULT_TEMPLATE:
            t = tx[0]
            if infer == t["typename"] or infer == t["short"]:
                if len(args.types) == 0:
                    args.types = [ t ]
                else:
                    args.types.append([ t ])
                break

    if len(args.types) == 0:
        print("No template specified, exiting...", file=stderr)
        exit(1)

    try:

        if isinstance(args.types, list) and len(args.types) > 0 and isinstance(args.types[0], list):
            for types in args.types:
                new_args = copy.deepcopy(args)
                new_args.types = types
                process(new_args)
            exit(0)

        process(args)
    except KeyboardInterrupt:
        exit(1)
if __name__ == "__main__":
    main()

