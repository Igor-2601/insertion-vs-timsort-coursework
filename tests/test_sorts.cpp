#include "datasets.hpp"
#include "sorts.hpp"
#include <iostream>
#include <stdexcept>

struct Item {
    int key = 0;
    std::size_t original = 0;
    bool operator<(const Item& other) const { return key < other.key; }
    bool operator==(const Item& other) const {
        return key == other.key && original == other.original;
    }
};
std::size_t cases = 0;
void check(const std::vector<int>& keys) {
    std::vector<Item> input;
    for (std::size_t i = 0; i < keys.size(); ++i) input.push_back({keys[i],i});
    auto expected = input;
    std::stable_sort(expected.begin(), expected.end());
    auto a = input, b = input;
    coursework::TimWorkspace<Item> work;
    work.prepare(input.size());
    coursework::Counters ca, cb;
    coursework::insertion_sort<Item,true>(a,&ca);
    coursework::timsort<Item,true>(b,work,&cb);
    if (a != expected || b != expected) throw std::runtime_error("Correctness/stability failure");
    a = input; b = input;
    coursework::insertion_sort(a);
    coursework::timsort(b,work);
    if (a != expected || b != expected) throw std::runtime_error("Uncounted path failure");
    ++cases;
}
int main() {
    // Exhaust all arrays over three keys, including ties and empty input.
    std::size_t combinations = 1;
    for (std::size_t n = 0; n <= 8; ++n) {
        for (std::size_t code = 0; code < combinations; ++code) {
            auto q = code;
            std::vector<int> a(n);
            for (auto& x : a) { x = static_cast<int>(q % 3) - 1; q /= 3; }
            check(a);
        }
        combinations *= 3;
    }
    for (std::size_t n = 0; n <= 260; ++n)
        for (std::size_t type = 0; type < 5; ++type)
            for (std::size_t sample = 0; sample < 4; ++sample)
                check(coursework::make_dataset(n,type,sample));
    for (auto n : {511u,512u,513u,1000u,4096u,10000u})
        for (std::size_t type = 0; type < 5; ++type)
            check(coursework::make_dataset(n,type,21));
    for (std::size_t n : {1u,2u,32u,64u,100u,1000u}) {
        auto a = coursework::make_dataset(n,1,0);
        coursework::Counters c;
        coursework::insertion_sort<int,true>(a,&c);
        if (c.comparisons != n-1 || c.writes != n-1) throw std::runtime_error("Best-case counts");
        a = coursework::make_dataset(n,2,0); c = {};
        coursework::insertion_sort<int,true>(a,&c);
        if (c.comparisons != n*(n-1)/2 || c.writes != n*(n-1)/2+n-1)
            throw std::runtime_error("Worst-case counts");
    }
    // Structured runs, equal boundaries, and many galloping opportunities.
    for (std::size_t pass = 0; pass < 120; ++pass) {
        std::mt19937_64 rng(8911 + pass);
        std::vector<int> a;
        for (std::size_t run = 0; run < 25; ++run) {
            const auto length = 1 + coursework::uniform_below(rng,180);
            const int base = static_cast<int>(coursework::uniform_below(rng,5000));
            for (std::size_t i = 0; i < length; ++i)
                a.push_back(base + static_cast<int>(i/2));
        }
        check(a);
    }
    std::cout << "PASS " << cases << " datasets, both counted and uncounted paths; stability verified.\n";
}
