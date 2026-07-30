import csv
from collections import Counter, defaultdict
from pathlib import Path

root = Path("my_works")
rows_by_k = {}

for k in (2, 6, 10):
    path = root / f"DNA_Search_Results_K{k}.csv"
    with path.open("r", encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.reader(stream))

    if rows and not rows[0][0].isdigit():
        rows = rows[1:]
    rows_by_k[k] = rows

    by_read = defaultdict(list)
    for row in rows:
        by_read[int(row[0])].append(row)

    successful = 0
    failed = 0
    match_rows = 0
    hamming = Counter()
    candidate_counts = []
    elapsed_values = []
    failed_ids = []
    positions_by_read = {}

    for read_id in sorted(by_read):
        read_rows = by_read[read_id]
        first = read_rows[0]
        candidate_counts.append(int(first[7]))
        elapsed_values.append(float(first[8]))
        matches = [row for row in read_rows if row[6] == "匹配"]
        if matches:
            successful += 1
            match_rows += len(matches)
            hamming.update(int(row[4]) for row in matches)
            positions_by_read[read_id] = [int(row[3]) for row in matches]
        else:
            failed += 1
            failed_ids.append(read_id)

    multi_match_reads = sum(
        1 for positions in positions_by_read.values() if len(positions) > 1
    )
    print(f"K={k}")
    print(f"records={len(rows)} reads={len(by_read)}")
    print(
        f"success={successful} fail={failed} matches={match_rows} "
        f"multi_match_reads={multi_match_reads}"
    )
    print(f"hamming={dict(sorted(hamming.items()))}")
    print(
        "candidates="
        f"{sum(candidate_counts)} mean={sum(candidate_counts)/len(candidate_counts):.2f} "
        f"min={min(candidate_counts)} max={max(candidate_counts)}"
    )
    print(
        "elapsed="
        f"{sum(elapsed_values):.2f} mean={sum(elapsed_values)/len(elapsed_values):.3f} "
        f"min={min(elapsed_values):.2f} max={max(elapsed_values):.2f}"
    )
    print(f"failed_ids={failed_ids}")
    print(f"positions={positions_by_read}")

k10_failed = {
    int(row[0])
    for row in rows_by_k[10]
    if row[6] != "匹配"
}
offsets_for_failed = {}
for row in rows_by_k[6]:
    read_id = int(row[0])
    if read_id not in k10_failed:
        continue
    start = int(row[3])
    absolute_positions = [
        int(item.split(":", 1)[0])
        for item in row[5].split(";")
        if item
    ]
    offsets_for_failed[read_id] = [
        position - start for position in absolute_positions
    ]
print(f"K10_FAILED_MUTATION_OFFSETS={offsets_for_failed}")
