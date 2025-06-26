#define _USE_MATH_DEFINES

#include <vector>
#include <array>
#include <cmath>
#include <iostream>
#include <ranges>

/*
    Simple implementation of DCT-II (most commonly used form):
    https://en.wikipedia.org/wiki/Discrete_cosine_transform#DCT-II

    Xk = sum(x[n] * cos((pi / N) * (n + 0.5) * k)) for n ∈ [0 .. N - 1] and k ∈ [0 .. N - 1]

*/

template<typename Iterable>
std::vector<double> dct(const Iterable& data) noexcept
{
    const std::size_t N = data.size();
    const double scale = std::sqrt(2.0 / static_cast<double>(N));

    std::vector<double> res(N, 0.0);

    for(std::size_t k = 0; k < N; k++)
    {
        double sum = 0.0;

        for(std::size_t n = 0; n < N; n++)
        {
            sum += data[n] * std::cos((M_PI / static_cast<double>(N)) * (static_cast<double>(n) + 0.5) * static_cast<double>(k));
        }

        res[k]= sum * scale * ((k == 0) ? (1.0 / std::sqrt(2.0)) : 1.0);
    }

    return res;
}

int main(int argc, char** argv) noexcept
{
    std::array<double, 8> points = { -1.0, 2.0, 3.0, 6.0, -3.0, -2.0, 0.0, 3.0 };

    std::cout << "Points: ";

    for(const auto [i, x] : std::ranges::enumerate_view(points))
    {
        std::cout << x << (i == (points.size() - 1) ? "" : ",");
    }

    std::cout << "\n";

    std::vector<double> dct_res = dct(points);

    std::cout << "DCT: ";

    for(const auto [i, x] : std::ranges::enumerate_view(dct_res))
    {
        std::cout << x << (i == (dct_res.size() - 1) ? "" : ",");
    }

    std::cout << "\n";

    return 0;
}
