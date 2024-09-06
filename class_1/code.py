#
# SC2001
# Example Class 1
#

import sys
from concurrent.futures import Future, as_completed
from concurrent.futures.process import ProcessPoolExecutor
from csv import DictReader, DictWriter
from datetime import datetime
from os import path
from random import randint, seed
from sys import maxsize
from timeit import timeit
from typing import Dict, Iterable


def rand_range(x: int, n: int) -> list[int]:
    """Generate n integers in range 1..x"""
    return [randint(1, x) for i in range(n)]


def insert_sort(items: list[int]) -> tuple[list[int], int]:
    """Performs insertion sort on given items.

    Returns sorted list, along with no. of key comparisons.
    """
    n_compare = 0

    for i in range(1, len(items)):
        for j in reversed(range(1, i + 1)):
            n_compare += 1
            if items[j - 1] <= items[j]:
                break
            items[j - 1], items[j] = items[j], items[j - 1]
    return items, n_compare


def merge(left: list[int], right: list[int]) -> tuple[list[int], int]:
    # merge sorted subarray into sorted array
    merged = []
    l, r, n_compare = 0, 0, 0
    while l < len(left) and r < len(right):
        n_compare += 1
        if left[l] <= right[r]:
            merged.append(left[l])
            l += 1
        else:
            merged.append(right[r])
            r += 1
    # append any remaining elements from merging
    for i in range(l, len(left)):
        merged.append(left[i])
    for i in range(r, len(right)):
        merged.append(right[i])

    return merged, n_compare


def hybrid_sort(items: list[int], s=1) -> tuple[list[int], int]:
    """Hybrid Merge-Insertion in place sort

    Performs merge sort > s no. of items. Transitions to insertion sort <= s no. of items.
    Returns sorted list, along with no. of key comparisons.
    """
    if len(items) <= s:
        return insert_sort(items)

    # recursively sort halved subarrays
    mid = len(items) // 2
    left, n_left_compare = hybrid_sort(items[:mid], s)
    right, n_right_compare = hybrid_sort(items[mid:], s)

    merged, n_compare = merge(left, right)
    return merged, n_left_compare + n_right_compare + n_compare


def trial(t: int, n: int, s: int = 1) -> dict[str, float]:
    """Perform a single trial using given parameters as return results as dictionary."""
    results = {
        "t": t,
        "n": n,
        "s": s,
    }  # type: dict[str, float]

    n_compares = 0

    def exec():
        _, n_compares = hybrid_sort(rand_range(n, n))
        results["n_compares"] = n_compares

    results["time_taken_s"] = timeit(exec, number=1)
    return results


if __name__ == "__main__":
    csv_path = sys.argv[1] if len(sys.argv) > 1 else "lab_results.csv"

    # restore already completed experiment from csv if any
    done = set()
    if path.exists(csv_path):
        with open(csv_path, "r") as f:
            for row in DictReader(f):
                done.add((int(row["n"]), int(row["s"]), int(row["t"])))

    # write results to csv for analysis
    with open(csv_path, "a") as f:
        csv = DictWriter(f, fieldnames=["n", "s", "t", "n_compares", "time_taken_s"])
        if f.tell() == 0:
            # new csv: write header
            csv.writeheader()

        # collect trail parameters
        params, n_skip = [], 0
        # try input sizes 1k -> 10M
        for e in range(3, 8):
            n = 10**e
            # try out values for param s
            for s in range(1, 128 + 1):
                # run multiple trials per (n, s) combination
                for t in range(5):
                    # skip trial if already done
                    if (n, s, t) in done:
                        n_skip += 1
                        continue
                    params.append({"t": t, "n": n, "s": s})
        print(f"search: {len(params)} trials")
        if n_skip > 0:
            print(f"skip: {n_skip} trials already completed")

        # search for optimal parameters over multiple processes on all cpu cores
        with ProcessPoolExecutor() as p:
            trials = [p.submit(trial, **param) for param in params]
            for n_done, trail in enumerate(as_completed(trials)):
                csv.writerow(trail.result())

                # progress info
                print(f"{n_done}/{len(params)} {n_done/len(params):.2%}")
