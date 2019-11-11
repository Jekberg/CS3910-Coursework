#include <cassert>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <iostream>
#include <numeric>
#include <string>

//template<typename ForwardItA>
//auto Error(ForwardItA )

// 35 pallets per trucks
// Trucks = pallets / 35

template<
    typename ForwardItEst,
    typename ForwardItDem>
constexpr auto Cost(
    ForwardItEst firstEst,
    ForwardItEst lastEst,
    ForwardItDem demandIt)
    noexcept
{
    assert(firstEst != lastEst && "Cannot calculate costs of an empty range");
    auto const Count{static_cast<std::size_t>(std::distance(
        firstEst,
        lastEst))};

    auto Total = std::transform_reduce(
        firstEst,
        lastEst,
        demandIt,
        0.0,
        [](auto a, auto b) noexcept {return std::abs(a - b);},
        std::plus<double>{});

    return Total / Count;
}

template<typename ForwardIt>
constexpr double Estemate(
    ForwardIt first,
    ForwardIt last,
    ForwardIt weightIt)
    noexcept
{
    return std::transform_reduce(
        first,
        last,
        weightIt,
        1.0,
        std::multiplies<double>{},
        std::plus<double>{});
}

bool Read(char const* fileName)
{
    assert(fileName != nullptr && "The fileName must not be the nullptr");
    std::ifstream file{};

    file.open(fileName);
    if(!file.is_open())
        return false;

    std::string line;
    while(file >> line)
    {
        std::cout << line << '\n';
    }

    return true;
}

int main()
{
    Read("sample/cwk_train.csv");
}