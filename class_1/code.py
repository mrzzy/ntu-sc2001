#
# SC2001
# Example Class 1
#

from concurrent.futures import Future, as_completed
from concurrent.futures.process import ProcessPoolExecutor
from csv import DictWriter
from datetime import datetime
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


def trial(n: int, s: int = 1) -> dict[str, float]:
    """Perform a single trial using given parameters as return results as dictionary."""
    results = {
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
    # seed rng for reproducible results
    seed(42)

    # write results to csv for analysis
    with open(f"lab_1_results_{datetime.utcnow().isoformat()}.csv", "w+") as f:
        csv = DictWriter(f, fieldnames=["n", "s", "n_compares", "time_taken_s"])
        csv.writeheader()
        # search for optimals over multiple processes on all cpu cores
        with ProcessPoolExecutor() as p:
            trials = []  # type: list[Future]
            # try input sizes 1k -> 10M
            for e in range(3, 8):
                n = 10**e
                # try out values for param s
                for s in range(1, 128 + 1):
                    # run multiple trials per (n, s) combination
                    for t in range(5):
                        trials.append(p.submit(trial, n=n, s=s))

            for trail in as_completed(trials):
                csv.writerow(trail.result())
                f.flush()
