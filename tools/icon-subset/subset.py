import argparse
import pathlib

from fontTools import subset
from fontTools.ttLib import TTFont

NAMES = [
    "add", "remove", "close", "check", "check_small", "search", "menu", "settings", "home",
    "favorite", "star", "bookmark", "delete", "edit", "share", "content_copy", "send", "mail",
    "person", "notifications", "info", "warning", "error", "visibility", "visibility_off",
    "refresh", "download", "upload", "play_arrow", "pause", "stop", "light_mode", "dark_mode",
    "palette", "arrow_back", "arrow_forward", "arrow_upward", "arrow_downward", "chevron_left",
    "chevron_right", "expand_more", "expand_less", "more_vert", "more_horiz", "menu_open",
    "view_list", "grid_view", "backspace", "crop_square", "filter_none",
]

CPP_KEYWORDS = {"delete"}

FONT_NAME = "MaterialSymbolsRounded-subset.ttf"


def read_codepoints(path):
    table = {}
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        name, _, code = line.partition(" ")
        if code:
            table.setdefault(name, int(code, 16))
    return table


def cpp_name(name):
    return name + "_" if name in CPP_KEYWORDS else name


def write_header(path, entries):
    path.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        "#pragma once",
        "",
        "#include <array>",
        "#include <cstddef>",
        "",
        "namespace lamina::icons {",
        "",
        "enum class Symbol : char32_t {",
    ]
    lines += [f"    {cpp_name(n)} = 0x{c:04x}," for n, c in entries]
    lines += [
        "};",
        "",
        f"inline constexpr std::array<Symbol, {len(entries)}> all_symbols{{",
    ]
    lines += [f"    Symbol::{cpp_name(n)}," for n, _ in entries]
    lines += [
        "};",
        "",
        f'inline constexpr const char* symbols_font_file = "{FONT_NAME}";',
        "",
        "}",
        "",
    ]
    pathlib.Path(path).write_text("\n".join(lines), encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("font")
    parser.add_argument("codepoints")
    parser.add_argument("repo")
    args = parser.parse_args()

    table = read_codepoints(args.codepoints)
    missing = [n for n in NAMES if n not in table]
    if missing:
        raise SystemExit("missing in codepoints: " + ", ".join(missing))
    entries = [(n, table[n]) for n in NAMES]

    repo = pathlib.Path(args.repo)
    out_font = repo / "assets" / "icons" / FONT_NAME
    out_font.parent.mkdir(parents=True, exist_ok=True)

    options = subset.Options()
    options.layout_features = []
    options.hinting = False
    options.name_IDs = ["*"]
    options.notdef_outline = True
    font = subset.load_font(args.font, options)
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=[c for _, c in entries])
    subsetter.subset(font)
    subset.save_font(font, str(out_font), options)

    check = TTFont(str(out_font))
    cmap = check.getBestCmap()
    absent = [n for n, c in entries if c not in cmap]
    if absent:
        raise SystemExit("not in subset: " + ", ".join(absent))

    write_header(repo / "src" / "lamina" / "icons" / "symbols.hpp", entries)


if __name__ == "__main__":
    main()
