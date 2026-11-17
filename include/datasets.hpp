#pragma once
#include <algorithm>
#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace coursework {
inline constexpr std::uint64_t experiment_seed = 20261117;
inline const std::vector<std::string> data_types{
    "random", "sorted", "reversed", "nearly_sorted", "few_unique"};

inline std::uint64_t uniform_below(std::mt19937_64& rng, std::uint64_t bound) {
    const auto threshold = (std::uint64_t(0) - bound) % bound;
    std::uint64_t x;
    do { x = rng(); } while (x < threshold);
    return x % bound;
}
inline std::uint64_t dataset_seed(std::size_t n, std::size_t type, std::size_t sample) {
    return experiment_seed ^ (std::uint64_t(n) * 0x9e3779b97f4a7c15ULL)
         ^ (std::uint64_t(type + 1) * 0xbf58476d1ce4e5b9ULL)
         ^ (std::uint64_t(sample + 1) * 0x94d049bb133111ebULL);
}
inline std::vector<int> make_dataset(std::size_t n, std::size_t type, std::size_t sample) {
    std::mt19937_64 rng(dataset_seed(n, type, sample));
    std::vector<int> a(n);
    // Disjoint value intervals make the 100 samples provably distinct.
    const int base = static_cast<int>(sample * 100003);
    for (std::size_t i = 0; i < n; ++i)
        a[i] = base + static_cast<int>(5 * i + uniform_below(rng, 5));
    if (type == 0) {
        for (auto i = n; i > 1; --i)
            std::swap(a[i - 1], a[uniform_below(rng, i)]);
    } else if (type == 2) {
        std::reverse(a.begin(), a.end());
    } else if (type == 3 && n > 1) {
        std::vector<std::size_t> pairs(n / 2);
        for (std::size_t i = 0; i < pairs.size(); ++i) pairs[i] = i * 2;
        const auto swaps = std::min(pairs.size(), std::max<std::size_t>(1, n / 100));
        for (std::size_t i = 0; i < swaps; ++i) {
            const auto j = i + uniform_below(rng, pairs.size() - i);
            std::swap(pairs[i], pairs[j]);
            const auto at = pairs[i];
            std::swap(a[at], a[at + 1]);
        }
    } else if (type == 4) {
        for (auto& x : a) x = base + static_cast<int>(uniform_below(rng, 16));
    }
    return a;
}
} // namespace coursework
