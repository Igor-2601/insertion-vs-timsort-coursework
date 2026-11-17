#pragma once
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace coursework {
struct Counters {
    std::uint64_t comparisons = 0;
    std::uint64_t writes = 0;
    std::size_t max_stack = 0;
};

// Count key comparisons and writes to the main/auxiliary arrays only.
// Index arithmetic, local variables and run descriptors are not counted.
template<class T, bool Count> struct Operations {
    Counters* counters;
    bool less(const T& a, const T& b) {
        if constexpr (Count) ++counters->comparisons;
        return a < b;
    }
    void write(T& to, const T& from) {
        if constexpr (Count) ++counters->writes;
        to = from;
    }
};

template<class T, bool Count>
void insertion_range(std::vector<T>& a, std::size_t first, std::size_t last,
                     Operations<T, Count>& op) {
    for (std::size_t i = first + 1; i < last; ++i) {
        T key = a[i];
        std::size_t j = i;
        while (j > first && op.less(key, a[j - 1])) {
            op.write(a[j], a[j - 1]);
            --j;
        }
        op.write(a[j], key);
    }
}

template<class T, bool Count = false>
void insertion_sort(std::vector<T>& a, Counters* counters = nullptr) {
    Operations<T, Count> op{counters};
    if (a.size() > 1) insertion_range(a, 0, a.size(), op);
}

struct Run { std::size_t base, length; };
template<class T> struct TimWorkspace {
    std::vector<T> buffer;
    std::vector<Run> runs;
    void prepare(std::size_t n) {
        buffer.resize(n);
        runs.reserve(128); // More than sufficient for any addressable array.
    }
};

inline std::size_t minrun(std::size_t n) {
    std::size_t remainder = 0;
    while (n >= 64) {
        remainder |= n & 1;
        n >>= 1;
    }
    return n + remainder;
}

// Return the first index for which a monotone predicate is false.
// Exponential probing followed by binary search implements galloping.
template<class Predicate>
std::size_t gallop_end(std::size_t first, std::size_t last, Predicate pred) {
    if (first == last || !pred(first)) return first;
    const auto length = last - first;
    std::size_t bound = 1, low = first + 1;
    while (bound < length && pred(first + bound)) {
        low = first + bound + 1;
        if (bound > length / 2) { bound = length; break; }
        bound *= 2;
    }
    std::size_t high = std::min(last, first + bound + 1);
    while (low < high) {
        const auto mid = low + (high - low) / 2;
        if (pred(mid)) low = mid + 1;
        else high = mid;
    }
    return low;
}

template<class T, bool Count = false> class TimSorter {
    std::vector<T>& a;
    TimWorkspace<T>& work;
    Operations<T, Count> op;
    int min_gallop = 7;

    std::size_t count_run(std::size_t first) {
        const auto n = a.size();
        if (first + 1 == n) return 1;
        std::size_t last = first + 2;
        if (op.less(a[first + 1], a[first])) {
            // Strict decrease is essential: reversing equal keys is unstable.
            while (last < n && op.less(a[last], a[last - 1])) ++last;
            for (std::size_t l = first, r = last - 1; l < r; ++l, --r) {
                T temporary = a[l];
                op.write(a[l], a[r]);
                op.write(a[r], temporary);
            }
        } else {
            while (last < n && !op.less(a[last], a[last - 1])) ++last;
        }
        return last - first;
    }

    void binary_insertion(std::size_t first, std::size_t last, std::size_t start) {
        for (std::size_t i = start; i < last; ++i) {
            T key = a[i];
            std::size_t low = first, high = i;
            while (low < high) {
                const auto mid = low + (high - low) / 2;
                // Upper bound puts the new key after equal existing keys.
                if (op.less(key, a[mid])) high = mid;
                else low = mid + 1;
            }
            for (auto j = i; j > low; --j) op.write(a[j], a[j - 1]);
            op.write(a[low], key);
        }
    }

    void merge(std::size_t first, std::size_t middle, std::size_t last) {
        if (!op.less(a[middle], a[middle - 1])) return;
        const auto left_length = middle - first;
        for (std::size_t k = 0; k < left_length; ++k)
            op.write(work.buffer[k], a[first + k]);
        std::size_t left = 0, right = middle, out = first;
        int left_streak = 0, right_streak = 0;
        while (left < left_length && right < last) {
            if (op.less(a[right], work.buffer[left])) {
                op.write(a[out++], a[right++]);
                ++right_streak;
                left_streak = 0;
            } else {
                op.write(a[out++], work.buffer[left++]);
                ++left_streak;
                right_streak = 0;
            }
            if (left == left_length || right == last) break;
            if (left_streak >= min_gallop) {
                const auto end = gallop_end(left, left_length, [&](auto k) {
                    return !op.less(a[right], work.buffer[k]);
                });
                const auto copied = end - left;
                while (left < end) op.write(a[out++], work.buffer[left++]);
                min_gallop = copied >= 7 ? std::max(1, min_gallop - 1)
                                        : std::min(64, min_gallop + 1);
                left_streak = 0;
            } else if (right_streak >= min_gallop) {
                const auto end = gallop_end(right, last, [&](auto k) {
                    return op.less(a[k], work.buffer[left]);
                });
                const auto copied = end - right;
                while (right < end) op.write(a[out++], a[right++]);
                min_gallop = copied >= 7 ? std::max(1, min_gallop - 1)
                                        : std::min(64, min_gallop + 1);
                right_streak = 0;
            }
        }
        while (left < left_length) op.write(a[out++], work.buffer[left++]);
        // Remaining right elements already occupy their final positions.
    }

    void merge_at(std::size_t i) {
        auto& stack = work.runs;
        const Run x = stack[i], y = stack[i + 1];
        assert(x.base + x.length == y.base);
        merge(x.base, y.base, y.base + y.length);
        stack[i].length += y.length;
        stack.erase(stack.begin() + static_cast<std::ptrdiff_t>(i + 1));
    }

    void collapse() {
        auto& s = work.runs;
        while (s.size() > 1) {
            std::size_t i = s.size() - 2;
            // Both triples must be checked (the issue23515 correction).
            if ((i > 0 && s[i - 1].length <= s[i].length + s[i + 1].length) ||
                (i > 1 && s[i - 2].length <= s[i - 1].length + s[i].length)) {
                if (s[i - 1].length < s[i + 1].length) --i;
            } else if (s[i].length > s[i + 1].length) {
                break;
            }
            merge_at(i);
        }
    }

public:
    TimSorter(std::vector<T>& array, TimWorkspace<T>& workspace, Counters* counts)
        : a(array), work(workspace), op{counts} {}

    void sort() {
        work.runs.clear();
        const auto n = a.size();
        if (n < 2) return;
        assert(work.buffer.size() >= n);
        const auto minimum = minrun(n);
        for (std::size_t first = 0; first < n;) {
            auto length = count_run(first);
            const auto forced = std::min(minimum, n - first);
            if (length < forced) {
                binary_insertion(first, first + forced, first + length);
                length = forced;
            }
            work.runs.push_back({first, length});
            if constexpr (Count)
                op.counters->max_stack = std::max(op.counters->max_stack, work.runs.size());
            collapse();
            first += length;
        }
        while (work.runs.size() > 1) {
            auto i = work.runs.size() - 2;
            if (i > 0 && work.runs[i - 1].length < work.runs[i + 1].length) --i;
            merge_at(i);
        }
    }
};

template<class T, bool Count = false>
void timsort(std::vector<T>& a, TimWorkspace<T>& workspace, Counters* counters = nullptr) {
    TimSorter<T, Count>(a, workspace, counters).sort();
}
} // namespace coursework
