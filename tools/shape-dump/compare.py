import sys


def load(path):
    shapes = {}
    with open(path, encoding="utf-8") as f:
        for line in f:
            fields = line.split()
            shapes[fields[0]] = (int(fields[1]), [float(x) for x in fields[2:]])
    return shapes


def main():
    reference = load(sys.argv[1])
    ported = load(sys.argv[2])
    if reference.keys() != ported.keys():
        print("different entries:", sorted(set(reference) ^ set(ported)))
        return 1
    worst = 0.0
    failures = 0
    for key, (count, values) in reference.items():
        other_count, other_values = ported[key]
        if count != other_count:
            print(f"{key}: {count} cubics in the reference, {other_count} in the port")
            failures += 1
            continue
        diff = max((abs(a - b) for a, b in zip(values, other_values)), default=0.0)
        worst = max(worst, diff)
    print(f"{len(reference)} entries, {failures} with a different cubic count, largest difference {worst:.3g}")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
