/*
    Simple reminder for descriptive statistics
*/

#include <cmath>
#include <vector>
#include <numeric>
#include <iostream>
#include <algorithm>
#include <unordered_map>

constexpr inline bool is_even(const std::size_t x) noexcept
{
    return x % 2 == 0;
}

constexpr inline double lerp(const double a, const double b, const double t) noexcept
{
    return (1.0 - t) * a + t * b;
}

constexpr inline double square(const double x) noexcept
{
    return x * x;
}

template<typename T>
double average(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute average on non-arithmetic data");

    double avg = 0.0;

    for(std::size_t i = 0; i < data.size(); i++)
    {
        const double t = 1.0 / (static_cast<double>(i) + 1.0);
        avg = lerp(avg, static_cast<double>(data[i]), t);
    }

    return avg;
}

template<typename T>
double median(std::vector<T> data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute average on non-arithmetic data");

    std::sort(data.begin(), data.end());

    if(is_even(data.size()))
    {
        const std::size_t mid = data.size() / 2;

        return static_cast<double>(data[mid - 1] + data[mid]) / 2.0;
    }
    else
    {
        const std::size_t mid = data.size() / 2;

        return static_cast<double>(data[mid]);
    }
}

template<typename T>
double variance(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute average on non-arithmetic data");

    const double avg = average(data);

    double variance = 0.0f;

    for(std::size_t i = 0; i < data.size(); i++)
    {
        const double t = 1.0 / (static_cast<double>(i) + 1.0);
        variance = lerp(variance, static_cast<double>(square(data[i] - avg)), t);
    }
    
    return variance;
}

template<typename T>
double stdev(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute average on non-arithmetic data");

    const double var = variance(data);

    return std::sqrt(var);
}

template<typename T>
std::tuple<double, double, double> quartiles(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute quartiles on non-arithmetic data");

    std::vector<T> data_copy = data;
    std::sort(data_copy.begin(), data_copy.end());

    const double low = is_even(data.size()) ? static_cast<double>(data_copy[static_cast<std::size_t>(data.size() * 0.25)]) : 
                                              static_cast<double>(data_copy[static_cast<std::size_t>(data.size() * 0.25) - 1] + data_copy[static_cast<std::size_t>(data.size() * 0.25)]) / 2.0;

    const double med = is_even(data.size()) ? static_cast<double>(data_copy[data.size() / 2 - 1] + data_copy[data.size() / 2]) / 2.0 : 
                                              static_cast<double>(data_copy[data.size() / 2]);

    const double up = is_even(data.size()) ? static_cast<double>(data_copy[static_cast<std::size_t>(data.size() * 0.75)]) : 
                                             static_cast<double>(data_copy[static_cast<std::size_t>(data.size() * 0.75) - 1] + data_copy[static_cast<std::size_t>(data.size() * 0.75)]) / 2.0;

    return std::make_tuple(low, med, up);
}

template<typename T>
double range(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute range on non-arithmetic data");

    T max = std::numeric_limits<T>::min();
    T min = std::numeric_limits<T>::max();

    for(const auto& x : data)
    {
        min = std::min(x, min);
        max = std::max(x, max);
    }

    return static_cast<double>(max - min);
}

template<typename T>
T mode(const std::vector<T>& data) noexcept
{
    static_assert(std::is_arithmetic<T>::value, "Cannot compute mode on non-arithmetic data");

    std::unordered_map<T, std::size_t> freqs;
    T* mode_value = nullptr;
    std::size_t current_max = 0;

    for(const auto& x : data)
    {
        freqs[x]++; 

        if(mode_value == nullptr || freqs[x] > current_max)
        {
            mode_value = const_cast<T*>(std::addressof(x));
            current_max = freqs[x];
        }
    }

    return *mode_value;
}

static constexpr std::size_t DATA_SIZE = 100000;

int main(int argc, char** argv) noexcept
{
    std::vector<double> data(DATA_SIZE);

    std::iota(data.begin(), data.end(), 0.0);

    std::cout << "Average: " << average(data) << "\n";
    std::cout << "Median: " << median(data) << "\n";
    std::cout << "Variance: " << variance(data) << "\n";
    std::cout << "Stddev: " << stdev(data) << "\n";

    const auto [low, med, up] = quartiles(data);
    std::cout << "Lower quartile: " << low << "\n";
    std::cout << "Median quartile: " << med << "\n";
    std::cout << "Upper quartile: " << up << "\n";

    std::cout << "Range: " << range(data) << "\n";
    std::cout << "Mode: " << mode(data) << "\n";

    return 0;
}