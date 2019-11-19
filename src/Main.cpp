#include "CS3910/Core.h"
#include "CS3910/PSO.h"
#include "CS3910/Simulation.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

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

class HistoricalPalletData
{
public:
    explicit HistoricalPalletData(char const* fileName);
private:
    std::vector<double> demand_;
    std::vector<double> dataPoints_;
    std::size_t dataPointCount_;
};

class HyperParticleSwarmOptimisationPolicy
{
};

class BasicParticleSwarmOptimisationPolicy
{
public:

    explicit BasicParticleSwarmOptimisationPolicy();

    void Initialise() noexcept;

    void Step() noexcept;

    bool Terminate() const noexcept
    {
        return false;
    }

    void Complete() const noexcept
    {
    }

private:

    ParticleSwarmPopulation particles_{100, 13 /* HARDCODED!!! */};

    double globalBestFitness_ = std::numeric_limits<double>::infinity();
    
    std::vector<double> globalBestPosition_;

    std::random_device rng_{};

    // TODO encapsulate...
    std::vector<double> demand_{};
    std::vector<std::vector<double>> data_{};
};

int main()
{
    BasicParticleSwarmOptimisationPolicy pso{};
    Simulate(pso);
}

BasicParticleSwarmOptimisationPolicy::BasicParticleSwarmOptimisationPolicy()
{
    std::size_t count;
    Read(
        "sample/cwk_train.csv",
        count,
        std::back_inserter(demand_),
        std::back_inserter(data_));
}

void BasicParticleSwarmOptimisationPolicy::Initialise() noexcept
{
    auto const count = particles_.VectorSize();
    particles_.ForEach([&](auto&& p)
    {
        InitialiseRandomWeights(p.position, p.position + count, rng_);
        std::copy(p.position, p.position + count, p.bestPosition);
        std::fill(p.velocity, p.velocity + count, 0.0);

        *p.fitness = EstemateCost(
            data_.begin(), 
            data_.end(), 
            demand_.begin(),
            p.position);

        *p.bestFitness = *p.fitness;
    });
}

void BasicParticleSwarmOptimisationPolicy::Step() noexcept
{
    std::vector<double> bestPosition;
    auto globalBestCost = particles_.FindMin(
        std::back_inserter(bestPosition),
        [](auto const& a, auto const& b)
        {
            return *a.fitness < *b.fitness;
        });

    if(globalBestCost < globalBestFitness_)
    {
        globalBestFitness_ = globalBestCost;
        std::cout << globalBestCost << '|';
        for(auto&& x: bestPosition)
            std::cout << ' ' << x;
        std::cout << '\n';

        globalBestPosition_ =  std::move(bestPosition);
    }

    auto const count = particles_.VectorSize();
    particles_.ForEach([&](auto&& p)
    {
        NextPosition(
            p.position,
            p.position + count,
            p.bestPosition,
            globalBestPosition_.data(),
            p.velocity,
            p.position,
            rng_,
            1.0 / (2.0 * std::log(2)),
            1.0 / 2.0 + std::log(2),
            1.0 / 2.0 + std::log(2));
        *p.fitness = EstemateCost(
            data_.begin(),
            data_.end(),
            demand_.begin(),
            p.position);
        if (*p.fitness < *p.bestFitness)
        {
            *p.bestFitness = *p.fitness;
            std::copy(p.position, p.position + count, p.bestPosition);
        }
    });
}