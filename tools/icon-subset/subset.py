import argparse
import pathlib

from fontTools import subset
from fontTools.varLib import instancer

OPTIONS = {
    "layout_features": [],
    "hinting": False,
    "name_IDs": ["*"],
    "notdef_outline": True,
    "flavor": None,
}


def read_manifest(path):
    fonts = []
    codes = []
    for number, line in enumerate(pathlib.Path(path).read_text(encoding="utf-8").splitlines(), 1):
        kind, _, rest = line.partition(" ")
        if kind == "font":
            name, *axes = rest.split()
            fonts.append((name, {tag: float(value) for tag, _, value in (a.partition("=") for a in axes)}))
        elif kind == "icon":
            codes.append(int(rest, 16))
        elif line.strip():
            raise SystemExit(f"{path}:{number}: cannot read: {line}")
    return fonts, codes


def save(font, path, options):
    font.recalcTimestamp = False
    subset.save_font(font, str(path), options)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("font")
    parser.add_argument("manifest")
    parser.add_argument("out")
    args = parser.parse_args()

    fonts, codes = read_manifest(args.manifest)
    options = subset.Options(**OPTIONS)
    base = subset.load_font(args.font, options)
    subsetter = subset.Subsetter(options)
    subsetter.populate(unicodes=codes)
    subsetter.subset(base)
    absent = [c for c in codes if c not in base.getBestCmap()]
    if absent:
        raise SystemExit(f"{args.font} lacks " + ", ".join(f"U+{c:04X}" for c in absent))

    out = pathlib.Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    for name, axes in fonts:
        font = instancer.instantiateVariableFont(base, axes) if axes else base
        save(font, out / f"{name}.ttf", options)


if __name__ == "__main__":
    main()
