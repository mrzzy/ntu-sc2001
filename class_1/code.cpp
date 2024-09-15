/*
 * SC2001
 * Example Class 1
 */

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <limits>
#include <random>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

using IntVec = std::vector<int>;
using IntSpan = std::span<int>;

/**
 * Generate a vector of size n with random numbers from 1 to max_val.
 * @param n size of generated vector.
 * @param max_val inclusive upper bound of random integers to generate.
 * @param seed Seed used to seeds the random number generator (RNG).
 * @returns Generated random vector.
 */
IntVec generate_random(int n, int max_val, int seed) {
  // construct portable rng with seed
  std::mt19937 random(seed);
  std::uniform_int_distribution<> randint(1, max_val);

  // populate vector of size n with random values
  IntVec rand_list(n, 0);
  std::generate(rand_list.begin(), rand_list.end(),
                [&]() { return randint(random); });
  return rand_list;
}

/**
 * Sort the given items in place with insertion sort.
 * @param items Elements to sort.
 * @returns No. of key comparisons.
 */
uint64_t insert_sort(IntVec &items) {
  uint64_t n_compare = 0;

  // skip first element since its always in sorted LHS
  for (int i = 1; i < items.size(); i++) {
    for (int j = i; j > 0; j--) {
      // track key comparison in next line
      n_compare++;
      if (items[j - 1] <= items[j]) {
        // item in sorted position
        break;
      }
      // misplaced item: swap
      std::swap(items[j - 1], items[j]);
    }
  }
  return n_compare;
}

/**
 * Merge items int sorted left and right subarrays into a sorted merge array.
 * Assumes items are sorted in ascending order.
 * @param left Left sorted subarray to merge.
 * @param right Right sorted subarray to merge.
 * @returns Merged sorted array & no. of key comparisons.
 */
std::tuple<IntVec, uint64_t> merge(const IntVec &left, const IntVec &right) {
  int i = 0;
  IntVec merged(left.size() + right.size());

  int l = 0;
  int r = 0;
  uint64_t n_compare = 0;
  while (l < left.size() && r < right.size()) {
    // both subarrays still contain elements
    // track key comparison on next line
    n_compare++;
    // merge the next smaller (or equal) element into the merged list
    if (left[l] <= right[r]) {
      merged[i] = left[l];
      l++;
    } else {
      merged[i] = right[r];
      r++;
    }
    i++;
  }

  // push any remaining unmerged elements into merged list
  while (l < left.size()) {
    merged[i] = left[l];
    l++;
    i++;
  }
  while (r < right.size()) {
    merged[i] = right[r];
    r++;
    i++;
  }

  return {merged, n_compare};
}

/**
 * Sort given items with Hybrid Merge-Insertion sort algorithm.
 * @param items Collect elements to sort.
 * @param s When subarray size falls below s, sorting algorithm performed
 *  switches from merge sort to insertion sort.
 * @returns Sorted array & no. of key comparisons
 */
std::tuple<IntVec, uint64_t> hybrid_sort(const IntSpan &items, int s) {
  // switch to insertion sort when input size falls below or equal s
  if (items.size() <= s) {
    IntVec sorted(items.begin(), items.end());
    uint64_t n_compare = insert_sort(sorted);
    return {sorted, n_compare};
  }

  // perform mergesort by recursively sorting halved subarrays
  int mid = items.size() / 2;
  auto [left, n_left_compare] =
      hybrid_sort(IntSpan{items.begin(), items.begin() + mid}, s);
  auto [right, n_right_compare] =
      hybrid_sort(IntSpan{items.begin() + mid, items.end()}, s);
  auto [merged, n_merge_compare] = merge(left, right);
  return {merged, n_left_compare + n_right_compare + n_merge_compare};
}

/**
 * Run a single Hybrid sort experiment trial.
 * @param n size of generated vector.
 * @param s Threshold s passed to hybrid_sort()
 * @param seed Seed used to seed random number generator.
 * @returns Time taken in microseconds & no. of key comparisons.
 */
std::tuple<uint64_t, uint64_t> run_trial(int n, int s, int seed) {
  // generate items to sort
  IntVec items = generate_random(n, std::numeric_limits<int>::max(), seed);
  // instrument to measure calltime
  using namespace std::chrono;
  auto begin = steady_clock::now();
  auto [sorted, n_compares] = hybrid_sort(IntSpan(items), s);
  auto elapsed = steady_clock::now() - begin;
  // check results of sorting in debug build
  assert(std::is_sorted(sorted.begin(), sorted.end()));

  return {duration_cast<microseconds>(elapsed).count(), n_compares};
}

int main(int argc, char *argv[]) {
  std::string out_path = "lab_results.csv";
  if (argc >= 2) {
    out_path = argv[1];
  }
  std::ofstream out(out_path);
  // write csv header
  out << "n,s,trial,n_compares,time_taken_ms" << "\n";

  // experiment i: try input sizes -> 10 million with fixed s = 42
  const int n_sample = 30;
  const int base_seed = 42;
  // list of (input size n, random seed)
  for (int e = 3; e <= 7; e++) {
    int n = std::pow(10, e);
    for (int i = 0; i < n_sample; i++) {
      int s = 42;
      auto [time_taken, n_compares] = run_trial(n, s, base_seed + i);
      out << n << "," << s << "," << i << "," << n_compares << ","
          << std::setprecision(std::numeric_limits<double>::digits10)
          << time_taken << "\n";
    }
  }

  // experiment 2: try s: 1 -> fixed input size n = 1000
  const int n = 1000;
  for (int s = 1; s <= n; s++) {
    for (int i = 0; i < n_sample; i++) {
      auto [time_taken, n_compares] = run_trial(n, s, base_seed + i);
      out << n << "," << s << "," << i << "," << n_compares << ","
          << std::setprecision(std::numeric_limits<double>::digits10)
          << time_taken << "\n";
    }
  }

  // experiment 3: try input sizes -> 100k with exponential S
  for (int e = 3; e <= 5; e++) {
    int n = std::pow(10, e);
    // by observing experiment 2, we know that S used only matters on
    // 2-exponential intervals as merge sort is only able to succisively half
    // input size n. hence we only need to try out S on 2-exponential scale.
    // divide_e: divide exponent in S = n / 2^divide_e
    for (int divide_e = 0; std::pow(2, divide_e) <= n; divide_e++) {
      int s = n / std::pow(2, divide_e);
      for (int i = 0; i < n_sample; i++) {
        auto [time_taken, n_compares] = run_trial(n, s, base_seed + i);
        out << n << "," << s << "," << i << "," << n_compares << ","
            << std::setprecision(std::numeric_limits<double>::digits10)
            << time_taken << "\n";
      }
    }
  }
}
