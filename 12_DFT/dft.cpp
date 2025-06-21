#include <complex>
#include <array>
#include <iostream>
#include <ranges>
#include <vector>
#include <cmath>

using namespace std::complex_literals;

template<typename Iterable>
std::vector<std::complex<double>> dft(const Iterable& input) noexcept
{
    const std::size_t N = input.size();

    std::vector<std::complex<double>> res(N);   

    for(std::size_t i = 0; i < N; i++)
    {
        std::complex<double> Xn = 0;

        for(std::size_t j = 0; j < N; j++)
        {
            Xn += input[j] * exp(-1i * 
                                 2.0 * 
                                 M_PI * 
                                 static_cast<std::complex<double>>(j) *
                                 static_cast<std::complex<double>>(i) / 
                                 static_cast<std::complex<double>>(N));
        }

        res[i] = Xn;
    }

    return res;
}

int main(int argc, char** argv) noexcept
{
    std::array<std::complex<double>, 4> signal = { 1.0, 2.0, 0.5, -2 };

    std::cout << "Signal: ";

    for(const auto [i, x] : std::ranges::enumerate_view(signal))
    {
        std::cout << x << (i < (signal.size() - 1)) ? ", " : "";
    }

    std::cout << "\n";

    std::vector<std::complex<double>> waves = dft(signal);

    std::cout << "DFT: ";

    for(const auto [i, x] : std::ranges::enumerate_view(waves))
    {
        std::cout << x << (i < (waves.size() - 1)) ? ", " : "";
    }

    std::cout << "\n";

    return 0;
}