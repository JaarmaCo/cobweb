import re
import json
import argparse
import subprocess
from typing import Any

DIRECTIVE = re.compile(r"\s*//\s*!\s*Template\s+([A-Z]+)\s+(.*)\s*")

def preprocess(args):

    cpp = args.preprocessor

    pp = subprocess.call([
        cpp,
        "-CC", # Keep comments
        "-P", # Discard line numbers
        *args.include,
        *args.define,
        *args.pp_arg,
        args.file
    ])
    print(pp)

ATTRIBUTES = [ "TYPE", "MANGLE" ]
HIDDEN = [ "TYPE", "MANGLE" ]

def parse_directives(args) -> tuple[str, dict[str, list[Any]], dict[str, list[str]]]:
    values: dict[str, list[Any]] = {}
    elements: dict[str, list[str]] = {}
    source: str = ""
    with open(args.file) as f:

        attributed = None
        hidden = False

        for line in f:
            match = DIRECTIVE.match(line)

            if match is None:

                if hidden:
                    if line.strip() == "":
                        hidden = False
                        continue
                    hidden = False

                if attributed is None:
                    source += line
                    continue

                if attributed not in elements:
                    elements[attributed] = []

                if attributed not in HIDDEN:
                    source += line

                elements[attributed].append(line.strip())
                attributed = None
                hidden = True
                continue

            key = match.group(1)
            val = json.loads(match.group(2))

            if key not in values:
                values[key] = []

            if key in ATTRIBUTES:
                attributed = key

            values[key].append(val)

    return source, values, elements

TYPEDEF = re.compile(r"typedef\s+(.*)\s+([a-zA-Z_][a-zA-Z_0-9]*)\s*;")

def get_type_parameters(args, directives, elements) -> dict[int, str]:

    types = {}

    if 'TYPE' not in directives or 'TYPE' not in elements:
        return types

    for number, element in zip(directives['TYPE'], elements['TYPE']):

        match = TYPEDEF.match(element)
        if match is None:
            print(f"Invalid template directive TYPE={number}: {element}")
            continue

        name = match.group(2)
        types[number] = name

    return types

ENVAR = re.compile(r"\$\{([^}]+)\}")

def subst_env(pattern: str, env: dict[str, Any]) -> str:

    def subst_var(name: str) -> str:
        keys = name.split(".")
        value = env
        for key in keys:
            if key not in value:
                break
            value = value[key]
        return str(value)

    return re.sub(ENVAR, lambda match: subst_var(match.group(1)), pattern)

MANGLE = re.compile(r"#\s*define\s+([a-zA-Z_][a-zA-Z_0-9]*)\s+([a-zA-Z_][a-zA-Z_0-9]*)\s*")

def get_mangle_parameters(args, type_substititions: dict[str, Any], directives: dict[str, list[str]], elements: dict[str, list[str]]) -> dict[str, str]:

    mangles = {}

    if 'MANGLE' not in directives or 'MANGLE' not in elements:
        return mangles

    for pattern, element in zip(directives['MANGLE'], elements['MANGLE']):

        match = MANGLE.match(element)
        if match is None:
            print(f"Invalid template directive MANGLE={pattern}: {element}")
            continue

        target = match.group(1)
        base = match.group(2)
        output = subst_env(pattern, {
            "1": base,
            **type_substititions,
        })

        mangles[target] = output

    return mangles

def map_type_parameters(args, types: dict[int, Any]) -> dict[str, Any]:
    results = {}
    for index, value in types.items():
        if index > len(args.types):
            print(f"Missing positional type parameter {index}, defaulting to int")
            results[value] = "int"
            continue
        results[value] = args.types[index - 1]
    return results

WHITESPACE = re.compile(r"\s+", flags=re.MULTILINE)
CSTRING = re.compile(r'"(?:[^"]|\")*"')
CCHAR = re.compile(r"'(?:[^']|\')*'")
CNAME = re.compile(r"[a-zA-Z_][a-zA-Z_0-9]*")
CPREPROCESSOR = re.compile(r"\s*#.*$", flags=re.MULTILINE)
CCOMMENT = re.compile(r"//.*$")
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

def substitute(source: str, type_substitutions: dict[str, Any], mangles: dict[str, str]) -> str:
    result = ""
    next = source
    while True:
        kind, tok, next = token(next)

        if tok in type_substitutions:
            if type_substitutions[tok] is str:
                result += type_substitutions[tok]
            else:
                result += type_substitutions[tok]["typename"]
            continue

        if tok in mangles:
            result += mangles[tok]
            continue

        result += tok

        if next == "" or tok == "":
            break

    return result

def main():
    parser = argparse.ArgumentParser(
        prog="c-template",
        description="""
        A Python-based template generator for C.
        """)
    parser.add_argument("-I",
                        dest="include",
                        action="append",
                        default=[],
                        help="Pass an include argument to the C preprocessor")
    parser.add_argument("-D",
                        dest="define",
                        action="append",
                        default=[],
                        help="Pass a definition to the C preprocessor")
    parser.add_argument("-W",
                        dest="pp_arg",
                        action="append",
                        default=[],
                        help="Pass a miscellaneous argument to the C preprocessor")
    parser.add_argument("-p",
                        "--preprocessor",
                        dest="preprocessor",
                        default="cpp",
                        help="Override the default C preprocessor command")
    parser.add_argument("-o",
                        "--output-file",
                        dest="out",
                        default="/dev/stdout",
                        help="Specify the destination of the generated source file")
    parser.add_argument("-i",
                        "--input-file",
                        dest="file",
                        default="/dev/stdin",
                        help="C source file to generate a template from")
    parser.add_argument("-J",
                        "--template-json",
                        dest="template_json_file",
                        help="JSON template file to parse type arguments from")
    parser.add_argument("types",
                        nargs="*",
                        help="Positional type arguments to use in the substitution",
                        type=json.loads)
    args = parser.parse_args()

    if args.template_json_file is not None:
        with open(args.template_json_file) as f:
            for t in json.load(f):
                args.types.append(t)

    try:
        source, directives, elements = parse_directives(args)
        type_params = get_type_parameters(args, directives, elements)
        type_substititions = map_type_parameters(args, type_params)
        mangles = get_mangle_parameters(args, type_substititions, directives, elements)
        with open(args.out, "w") as f:
            f.write(substitute(source, type_substititions, mangles))

    except KeyboardInterrupt:
        exit(1)
if __name__ == "__main__":
    main()

