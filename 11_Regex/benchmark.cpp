#include <iostream>
#include <chrono>
#include <vector>
#include <string>
#include <regex>
#include <random>
#include <iomanip>

#include "regex.hpp"

class BenchmarkTimer 
{
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() noexcept
    {
        this->start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed_ms() const noexcept
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - this->start_time);
        return duration.count() / 1000.0;
    }
};

class TestCase
{
public:
    std::string pattern;
    std::vector<std::string> test_strings;
    std::string description;
    
    TestCase(const std::string& p, 
             const std::vector<std::string>& tests,
             const std::string& desc) : pattern(p),
                                        test_strings(tests),
                                        description(desc) {}
};

std::vector<std::string> generateNumericStrings(int count, int min_length, int max_length) 
{
    std::vector<std::string> result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> len_dist(min_length, max_length);
    std::uniform_int_distribution<> digit_dist(0, 9);
    
    for(int i = 0; i < count; ++i) 
    {
        int length = len_dist(gen);
        std::string str;

        for(int j = 0; j < length; ++j) 
        {
            str += std::to_string(digit_dist(gen));
        }

        result.push_back(str);
    }

    return result;
}

std::vector<std::string> generateMixedStrings(int count, int min_length, int max_length) noexcept
{
    std::vector<std::string> result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> len_dist(min_length, max_length);
    std::uniform_int_distribution<> char_dist(0, 35); // 0-9, a-z
    
    for (int i = 0; i < count; ++i) {
        int length = len_dist(gen);
        std::string str;
        for (int j = 0; j < length; ++j) {
            if (char_dist(gen) < 10) {
                str += std::to_string(char_dist(gen) % 10);
            } else {
                str += static_cast<char>('a' + (char_dist(gen) - 10));
            }
        }
        result.push_back(str);
    }
    return result;
}

std::vector<std::string> generatePatternStrings(int count) noexcept
{
    std::vector<std::string> result;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> a_count(0, 10);
    std::uniform_int_distribution<> choice(0, 3);
    
    for(int i = 0; i < count; ++i) 
    {
        std::string str;
        int num_a = a_count(gen);

        for(int j = 0; j < num_a; ++j) 
        {
            str += 'a';
        }
        
        switch(choice(gen)) 
        {
            case 0: str += "b"; break;
            case 1: str = "cd"; break;
            case 2: str += "d"; break;
            default: str += "b"; break;
        }

        result.push_back(str);
    }
    return result;
}

void runBenchmark(const std::string& test_name, const TestCase& test_case, int iterations = 10000) noexcept
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "Benchmark: " << test_name << std::endl;
    std::cout << "Pattern: " << test_case.pattern << std::endl;
    std::cout << "Description: " << test_case.description << std::endl;
    std::cout << "Test strings: " << test_case.test_strings.size() << std::endl;
    std::cout << "Iterations: " << iterations << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    BenchmarkTimer timer;
    
    timer.start();
    
    Regex custom_regex(test_case.pattern, false);

    for(int i = 0; i < iterations; ++i) 
    {
        for(const auto& test_str : test_case.test_strings) 
        {
            custom_regex.match(test_str);
        }
    }

    double custom_time = timer.elapsed_ms();
    
    timer.start();

    std::regex std_regex(test_case.pattern);

    for(int i = 0; i < iterations; ++i) 
    {
        for(const auto& test_str : test_case.test_strings) 
        {
            std::regex_match(test_str, std_regex);
        }
    }

    double std_time = timer.elapsed_ms();
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Custom Regex: " << custom_time << " ms" << std::endl;
    std::cout << "std::regex:   " << std_time << " ms" << std::endl;
    
    if(custom_time < std_time) 
    {
        double speedup = std_time / custom_time;
        std::cout << "Custom regex is " << speedup << "x faster" << std::endl;
    } 
    else 
    {
        double slowdown = custom_time / std_time;
        std::cout << "Custom regex is " << slowdown << "x slower" << std::endl;
    }
}

void runCompilationBenchmark(const std::vector<TestCase>& test_cases, int iterations = 1000) noexcept
{
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "COMPILATION BENCHMARK" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    
    BenchmarkTimer timer;
    
    timer.start();

    for(int i = 0; i < iterations; ++i) 
    {
        for(const auto& test_case : test_cases) 
        {
            Regex custom_regex(test_case.pattern, false);
        }
    }

    double custom_compilation_time = timer.elapsed_ms();
    
    timer.start();

    for(int i = 0; i < iterations; ++i) 
    {
        for(const auto& test_case : test_cases) 
        {
            std::regex std_regex(test_case.pattern);
        }
    }

    double std_compilation_time = timer.elapsed_ms();
    
    std::cout << std::fixed << std::setprecision(2);
    std::cout << "Custom Regex compilation: " << custom_compilation_time << " ms" << std::endl;
    std::cout << "std::regex compilation:   " << std_compilation_time << " ms" << std::endl;
    
    if(custom_compilation_time < std_compilation_time) 
    {
        double speedup = std_compilation_time / custom_compilation_time;
        std::cout << "Custom regex compilation is " << speedup << "x faster" << std::endl;
    } 
    else 
    {
        double slowdown = custom_compilation_time / std_compilation_time;
        std::cout << "Custom regex compilation is " << slowdown << "x slower" << std::endl;
    }
}

int main(int argc, char** argv) noexcept 
{
    std::cout << "Regex Performance Benchmark" << std::endl;
    std::cout << "Comparing Custom Regex vs std::regex" << std::endl;
    
    std::vector<TestCase> test_cases = {
        TestCase(
            "[0-9]*",
            generateNumericStrings(100, 5, 20),
            "Numeric strings with * quantifier"
        ),
        TestCase(
            "[0-9]+",
            generateNumericStrings(100, 5, 20),
            "Numeric strings with + quantifier"
        ),
        TestCase(
            "a*b|cd",
            generatePatternStrings(100),
            "Alternation with * quantifier"
        ),
        TestCase(
            "a?[b-e]+",
            []() {
                std::vector<std::string> strings = {
                    "abcdebcde", "bcdebcde", "rbcdebcde", "bcde", "abcde",
                    "eeeeebbbbb", "acde", "bcdefgh", "abcdefghijk"
                };

                for(int i = 0; i < 50; ++i) 
                {
                    strings.push_back("a" + std::string(i % 10 + 1, 'b' + (i % 4)));
                    strings.push_back(std::string(i % 8 + 1, 'b' + (i % 4)));
                }

                return strings;
            }(),
            "Optional with character range"
        ),
        TestCase(
            "[a-z]*[0-9]+",
            generateMixedStrings(100, 5, 25),
            "Mixed alphanumeric pattern"
        )
    };
    
    test_cases.push_back(TestCase(
        "[0-9]*",
        generateNumericStrings(1000, 50, 100),
        "Large numeric strings stress test"
    ));
    
    test_cases.push_back(TestCase(
        "a*b*c*d*e*",
        []() {
            std::vector<std::string> strings;

            for(int i = 0; i < 200; ++i) 
            {
                std::string str;

                for(char c = 'a'; c <= 'e'; ++c) 
                {
                    int count = i % 10;
                    str += std::string(count, c);
                }

                strings.push_back(str);
            }

            return strings;
        }(),
        "Multiple consecutive * quantifiers"
    ));
    
    for(size_t i = 0; i < test_cases.size(); ++i) 
    {
        runBenchmark("Test " + std::to_string(i + 1), test_cases[i]);
    }
    
    runCompilationBenchmark(test_cases);
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "BENCHMARK COMPLETE" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Note: Results may vary based on:" << std::endl;
    std::cout << "- Compiler optimizations (-O2, -O3)" << std::endl;
    std::cout << "- Hardware specifications" << std::endl;
    std::cout << "- System load and background processes" << std::endl;
    std::cout << "- Implementation details of your custom regex" << std::endl;
    
    return 0;
}