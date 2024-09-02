#
# SC2001
# Example Class 1
#


from math import inf
from random import randint
from typing import Iterable


def rand_range(x: int, n: int) -> list[int]:
    """Generate n integers in range 1..x"""
    return [randint(1, x) for i in range(n)]


def insert_sort(items: list[int]) -> tuple[list[int], int]:
    """Performs insertion sort on given items in place.

    Returns sorted list, along with no. of key comparisons.
    """
    n_compare = 1

    for i in range(1, len(items)):
        for j in reversed(range(1, i + 1)):
            n_compare += 1
            if items[j - 1] > items[j]:
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
    merged.extend(left[l:])
    merged.extend(right[r:])

    return merged, n_compare


def hybrid_sort(items: list[int], s=inf) -> tuple[list[int], int]:
    """Hybrid Merge-Insertion in place sort

    Performs merge sort > s no. of items. Transitions to insertion sort <= s no. of items.
    Returns sorted list, along with no. of key comparisons.
    """
    if len(items) <= s:
        return insert_sort(items)

    # recursively sort halved subarrays
    mid = len(items) // 2
    left, n_left_compare = hybrid_sort(items[:mid], s)
    right, n_right_compare = hybrid_sort(items[mid + 1 :], s)

    merged, n_compare = merge(left, right)
    return merged, n_left_compare + n_right_compare + n_compare


if __name__ == "__main__":
    print(hybrid_sort(rand_range(10000000, 10000000)))
