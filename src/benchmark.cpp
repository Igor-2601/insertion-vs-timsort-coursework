#include "datasets.hpp"
#include "sorts.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>

using Clock = std::chrono::steady_clock;
volatile std::uint64_t observed_checksum = 0;

template<class Sort>
double timed_batch(const std::vector<int>& input, std::size_t repetitions, Sort sort) {
    std::vector<std::vector<int>> copies(repetitions, input);
    // Generation, copying, allocation and initialization occur before the timer.
    const auto begin = Clock::now();
    for (auto& a : copies) sort(a);
    const auto end = Clock::now();
    for (const auto& a : copies) {
        if (!std::is_sorted(a.begin(), a.end())) throw std::runtime_error("Sort failed");
        if (!a.empty()) observed_checksum ^= std::uint64_t(a.front()) + a.back();
    }
    return std::chrono::duration<double, std::micro>(end - begin).count() / repetitions;
}

int main(int argc, char** argv) {
    const std::filesystem::path out = argc > 1 ? argv[1] : "results";
    std::filesystem::create_directories(out);
    std::ofstream raw(out / "raw.csv");
    raw << "type,n,sample,seed,algorithm,repetitions,time_us,comparisons,writes,max_stack\n";
    raw << std::setprecision(12);
    const std::vector<std::size_t> sizes{32,64,100,300,1000,3000,5000,10000};
    constexpr std::size_t samples = 100;
    std::size_t total = 0;
    for (std::size_t type = 0; type < coursework::data_types.size(); ++type) {
        for (const auto n : sizes) {
            coursework::TimWorkspace<int> work;
            work.prepare(n);
            const auto repetitions = std::max<std::size_t>(1, 16384 / n);
            for (std::size_t sample = 0; sample < samples; ++sample) {
                const auto input = coursework::make_dataset(n, type, sample);
                auto reference = input;
                std::sort(reference.begin(), reference.end());
                auto warm = input;
                coursework::insertion_sort(warm);
                warm = input;
                coursework::timsort(warm, work);
                // The order follows the methodology: A, then B, same input.
                const double insertion_us = timed_batch(input, repetitions, [](auto& a) {
                    coursework::insertion_sort(a);
                });
                const double timsort_us = timed_batch(input, repetitions, [&](auto& a) {
                    coursework::timsort(a, work);
                });
                coursework::Counters ci, ct;
                auto ai = input, at = input;
                coursework::insertion_sort<int, true>(ai, &ci);
                coursework::timsort<int, true>(at, work, &ct);
                if (ai != reference || at != reference)
                    throw std::runtime_error("Output differs from std::sort");
                for (int algorithm = 0; algorithm < 2; ++algorithm) {
                    const auto& c = algorithm == 0 ? ci : ct;
                    raw << coursework::data_types[type] << ',' << n << ',' << sample << ','
                        << coursework::dataset_seed(n,type,sample) << ','
                        << (algorithm == 0 ? "insertion" : "timsort") << ',' << repetitions << ','
                        << (algorithm == 0 ? insertion_us : timsort_us) << ','
                        << c.comparisons << ',' << c.writes << ',' << c.max_stack << '\n';
                }
                ++total;
            }
            raw.flush();
            std::cout << coursework::data_types[type] << " n=" << n << " samples=100 done\n" << std::flush;
        }
    }
    std::ofstream meta(out / "run_metadata.txt");
    meta << "seed=" << coursework::experiment_seed << "\nunique_datasets=" << total
         << "\nraw_rows=" << total * 2 << "\ncompiler=" << __VERSION__
         << "\ncplusplus=" << __cplusplus << "\nclock_is_steady=" << Clock::is_steady
         << "\nclock_period=" << Clock::period::num << '/' << Clock::period::den
         << "\nchecksum=" << observed_checksum << '\n';
    std::cout << "Completed " << total << " unique datasets.\n";
}
