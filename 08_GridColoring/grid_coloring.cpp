#include <vector>
#include <iostream>
#include <cstdint>
#include <ranges>
#include <numeric>
#include <algorithm>
#include <tuple>

using Row = std::vector<uint8_t>;
using Row64 = std::vector<uint64_t>;
using Rows = std::vector<Row>;

Rows generate_rows(std::size_t n, std::size_t colors) 
{
    Rows result;
    Row current;
    current.reserve(n);

    auto dfs = [&](auto&& self, int pos) -> void {
        if(pos == n) 
        {
            result.push_back(current);
            return;
        }

        for(std::uint64_t c = 0; c < colors; c++) 
        {
            if(pos == 0 || c != current[pos - 1]) 
            {
                current.push_back(c);
                self(self, pos + 1);
                current.pop_back();
            }
        }
    };

    dfs(dfs, 0);
    
    return result;
}

bool rows_are_compatible(const Row& a, const Row& b) noexcept
{
    if(a.size() != b.size())
    {
        return false;
    }

    for(std::size_t i = 0; i < a.size(); i++)
    {
        if(a[i] == b[i])
        {
            return false;
        }
    }

    return true;
}

using TransitionMatrix = std::vector<std::vector<uint8_t>>;
using TransitionMatrix64 = std::vector<std::vector<uint64_t>>;

TransitionMatrix64 get_rows_transition_matrix(const Rows& rows)
{
    const std::size_t S = rows.size();

    TransitionMatrix64 T(S, Row64(S, 0));

    for(std::size_t i = 0; i < S; i++)
    {
        for(std::size_t j = 0; j < S; j++)
        {
            if(rows_are_compatible(rows[i], rows[j]))
            {
                T[i][j] = 1;
            }
        }
    }

    return T;
}

TransitionMatrix64 matmul(const TransitionMatrix64& A, const TransitionMatrix64& B)
{
    const std::size_t N = A.size();

    TransitionMatrix64 C(N, Row64(N, 0));

    for(std::size_t i = 0; i < N; i++)
    {
        for(std::size_t j = 0; j < N; j++)
        {
            for(std::size_t k = 0; k < N; k++)
            {
                C[j][k] += A[i][j] * B[k][j];
            }
        }
    }

    return C;
}

Row64 matmul_vec(const TransitionMatrix64& M, const Row64& V)
{
    const std::size_t N = M.size();

    Row64 result(N, 0);

    for(std::size_t i = 0; i < N; i++)
    {
        for(std::size_t j = 0; j < N; j++)
        {
            result[i] += M[i][j] * V[j];
        }
    }

    return result;
}

TransitionMatrix64 matexp(TransitionMatrix64 base, std::size_t exp)
{
    const std::size_t N = base.size();

    TransitionMatrix64 result(N, Row64(N, 0));

    for(std::size_t i = 0; i < N; i++)
    {
        result[i][i] = 1;
    }

    while(exp > 0)
    {
        if(exp % 2 == 1)
        {
            result = matmul(result, base);
        }

        base = matmul(base, base);
        exp /= 2;
    }

    return result;
}

int main(int argc, char** argv)
{
    constexpr std::size_t M = 5;
    constexpr std::size_t N = 1000;
    constexpr std::size_t N_COLORS = 3;
    constexpr std::size_t MOD = 1e9 + 7;

    Rows rows = generate_rows(N, N_COLORS);
    
    std::cout << "Total number of rows: " << rows.size() << '\n';

    if(M == 1)
    {
        std::cout << "Number of colorings: " << rows.size() << "\n";
        return 0;
    }

    TransitionMatrix64 T = get_rows_transition_matrix(rows);

    TransitionMatrix64 T_exp = matexp(T, M - 1);

    Row64 S(rows.size(), 1);

    Row64 V = matmul_vec(T_exp, S);

    std::size_t result = 0;

    for(const auto& v : V)
    {
        result = (result + v) % MOD;
    }

    std::cout << "Number of colorings: " << result << "\n";

    return 0;
}