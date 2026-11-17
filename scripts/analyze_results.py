"""Aggregate a benchmark run using Python's standard library only."""
import csv
import statistics
import sys
from collections import defaultdict
from pathlib import Path

folder = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parents[1] / 'results'
groups = defaultdict(list)
with (folder / 'raw.csv').open(encoding='utf-8') as stream:
    for row in csv.DictReader(stream):
        groups[row['type'], int(row['n']), row['algorithm']].append(row)
summary = []
for (kind, n, algorithm), rows in groups.items():
    values = [float(r['time_us']) for r in rows]
    summary.append(dict(type=kind, n=n, algorithm=algorithm, samples=len(rows),
        mean_us=statistics.mean(values), median_us=statistics.median(values),
        sd_us=statistics.stdev(values), min_us=min(values), max_us=max(values),
        mean_comparisons=statistics.mean(int(r['comparisons']) for r in rows),
        mean_writes=statistics.mean(int(r['writes']) for r in rows),
        max_stack=max(int(r['max_stack']) for r in rows)))
with (folder / 'summary.csv').open('w', newline='', encoding='utf-8') as stream:
    writer = csv.DictWriter(stream, fieldnames=list(summary[0]))
    writer.writeheader()
    writer.writerows(summary)
print('Aggregated', len(groups), 'groups into', folder / 'summary.csv')
