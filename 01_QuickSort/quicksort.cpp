/*
    Simple quicksort implementation
*/

#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <random>
#include <type_traits>

template<typename It, typename Cmp = std::less<typename std::iterator_traits<It>::value_type>>
void quicksort(It begin, It end, Cmp cmp = Cmp()) noexcept
{
    if(begin >= end)
    {
        return;
    }

    auto sort = [&](auto&& self, It low, It high) -> void {
        if(low >= high)
        {
            return;
        }

        auto pivot = std::prev(high);
        auto i = low;

        for(auto j = low; j < high; j++)
        {
            if(cmp(*j, *pivot))
            {
                std::swap(*i, *j);
                i++;
            }
        }

        std::swap(*i, *pivot);

        self(self, low, i);
        self(self, std::next(i), high);
    };

    sort(sort, begin, end);
}

constexpr int VECTOR_SIZE = 100;

int main(int argc, char** argv)
{
    std::random_device rd;
    std::mt19937 rng(rd());

    std::vector<int> vec(VECTOR_SIZE);
    std::iota(vec.begin(), vec.end(), 0);
    std::shuffle(vec.begin(), vec.end(), rng);

    std::cout << "Original vector: ";

    for(const int x : vec) 
    {
        std::cout << x << ' ';
    }

    std::cout << '\n';

    quicksort(vec.begin(), vec.end());

    std::cout << "Sorted vector: ";

    for(const int x : vec)
    {
        std::cout << x << ' ';
    }

    std::cout << '\n';

    return 0;
}