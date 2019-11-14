#include "CS3910/Core.h"
#include "CS3910/PSO.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

//template<typename ForwardItA>
//auto Error(ForwardItA )

// 35 pallets per trucks
// Trucks = pallets / 35

template<
    typename OutputDemandIt,
    typename OutputDataIt>
bool Read(
    char const* fileName,
    std::size_t& outMaxColumnCount,
    OutputDemandIt outDemandIt,
    OutputDataIt outDataIt)
{
    assert(fileName != nullptr && "The fileName must not be the nullptr");
    std::ifstream file{};

    file.open(fileName);
    if(!file.is_open())
        return false;

    std::string line;
    outMaxColumnCount = 0;
    for(;file >> line; ++outDemandIt, ++outDataIt)
    {
        auto i{std::find(line.begin(), line.end(), ',')};

        if(i == line.end())
            return false;

        *outDemandIt = std::stod(std::string{line.begin(), i});

        std::vector<double> temp{};
        for(++i; i < line.end(); ++i)
        {
            auto it = std::find(i, line.end(), ',');
            if(it == line.end())
                break;
            temp.push_back(std::stod(std::string{ i, it }));
            i = it;
        }

        if(outMaxColumnCount < temp.size())
            outMaxColumnCount = temp.size();
        *outDataIt = std::move(temp);
    }

    return true;
}

int main()
{
    std::vector<double> demand{};
    std::vector<std::vector<double>> data{};
    std::size_t count;

    Read(
        "sample/cwk_train.csv",
        count,
        std::back_inserter(demand),
        std::back_inserter(data));

    ParticleSwarmPopulation population{100, count};
    std::random_device rng{};

    population.ForEach([&](auto&& p)
    {
        InitialiseRandomWeights(p.position, p.position + count, rng);
        std::copy(p.position, p.position + count, p.bestPosition);
        std::fill(p.velocity, p.velocity + count, 0.0);

        p.cost = EstemateCost(
            data.begin(), 
            data.end(), 
            demand.begin(),
            p.position);

        p.bestCost = p.cost;
    });

    double allTimeBest = std::numeric_limits<double>::infinity();

    while(true)
    {
        std::vector<double> globalBestPosition;
        auto globalBestCost = population.FindMin(
            std::back_inserter(globalBestPosition),
            [](auto const& a, auto const& b)
            {
                return a.cost < b.cost;
            });

        if(globalBestCost < allTimeBest)
        {
            allTimeBest = globalBestCost;
            std::cout << globalBestCost << '|';
            for(auto&& x: globalBestPosition)
                std::cout << ' ' << x;
            std::cout << '\n';
        }

        population.ForEach([&](auto&& p)
        {
            NextPosition(
                p.position,
                p.position + count,
                p.bestPosition,
                globalBestPosition.data(),
                p.velocity,
                p.position,
                rng,
                1.0 / (2.0 * std::log(2)),
                1.0 / 2.0 + std::log(2),
                1.0 / 2.0 + std::log(2));

            p.cost = EstemateCost(
                data.begin(),
                data.end(),
                demand.begin(),
                p.position);

            if (p.cost < p.bestCost)
            {
                p.bestCost = p.cost;
                std::copy(p.position, p.position + count, p.bestPosition);
            }
        });
    };

    population.ForEach([&](auto&& p)
    {
        std::for_each(p.position, p.position + count, [](auto& x)
        {
            std::cout << ' ' << x; 
        });

        std::cout << '\n';
    });
}