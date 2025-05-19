#include <cstdint>
#include <iostream>
#include <vector>
#include <chrono>

std::vector<std::uint64_t> get_factors(std::uint64_t n) noexcept
{
    std::vector<std::uint64_t> factors;

    while(n % 2 == 0)
    {
        factors.push_back(2);
        n /= 2;
    }

    for(std::uint64_t i = 3; (i * i) < n; i += 2)
    {
        while(n % i == 0)
        {
            factors.push_back(i);
            n /= i;
        }
    }

    if(n > 2)
    {
        factors.push_back(n);
    }

    return factors;
}

int main(int argc, char** argv)
{
    std::uint64_t n;

    std::cout << "Enter n: ";
    std::cin >> n;

    std::cout << "Calculating prime factors of " << n << "\n";

    auto start = std::chrono::steady_clock::now();
    std::vector<std::uint64_t> factors = get_factors(n);
    auto duration = std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(std::chrono::steady_clock::now() - start).count();

    std::cout << "Factors: ";

    for(std::size_t i = 0; i < factors.size(); i++)
    {
        std::cout << (i == 0 ? "" : ", ") << factors[i];
    }

    std::cout << " (calculated in " << duration << " ms)\n";

    return 0;
}