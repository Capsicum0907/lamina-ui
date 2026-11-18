import collections
import sys


def load(path):
    values = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            if line.startswith("#") or not line.strip():
                continue
            *key, value = line.split()
            values[tuple(key)] = value
    return values


def main():
    reference = load(sys.argv[1])
    ported = load(sys.argv[2])
    missing = sorted(set(reference) - set(ported))
    if missing:
        print(f"{len(missing)} entries of the reference are missing from the port, first: {' '.join(missing[0])}")
        return 1
    compared = collections.Counter()
    skipped = collections.Counter()
    mismatches = collections.Counter()
    shown = 0
    for key, value in reference.items():
        spec = key[0]
        if value == "null":
            skipped[spec] += 1
            continue
        compared[spec] += 1
        if ported[key] != value:
            mismatches[spec] += 1
            if shown < 20:
                print(f"{' '.join(key)}: reference {value}, port {ported[key]}")
                shown += 1
    for spec in sorted(set(compared) | set(skipped)):
        print(f"spec {spec}: {compared[spec]} compared, {mismatches[spec]} different, {skipped[spec]} not in the reference")
    return 1 if sum(mismatches.values()) else 0


if __name__ == "__main__":
    sys.exit(main())
