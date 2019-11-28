#include "CS3910/Pallets.h"
#include "CS3910/PSO.h"
#include "CS3910/Simulation.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <execution>
#include <fstream>
#include <numeric>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

// 35 pallets per trucks
// Trucks = pallets / 35

class BasicPSOPolicy
{
public:

    struct Result
    {
        std::vector<double> position;
        double fitness;
    };

    struct Parameters
    {
        double inertia;
        double cogitiveAttraction;
        double socialAttraction;
    };


    explicit BasicPSOPolicy(
        PalletData historicalData,
        std::size_t populationSize,
        std::size_t iterations = 100000,
        Parameters parameters = {
            1.0 / (2.0 * std::log(2)),
            1.0 / 2.0 + std::log(2),
            1.0 / 2.0 + std::log(2)});

    void Initialise() noexcept;

    void Step() noexcept;

    bool Terminate() noexcept;

    Result Complete() const noexcept;

private:

    Particles particles_;

    PalletData historicalData_;

    Parameters params_;

    std::vector<std::minstd_rand> rngs_;

    double globalBestFitness_ = std::numeric_limits<double>::infinity();
    
    std::vector<double> globalBestPosition_;

    std::size_t iteration_{0};

    std::size_t MaxIteration;
};

class HyperPSOPolicy final
{
public:
    struct Result
    {
        std::vector<double> position;
        double fitness;
    };

    explicit HyperPSOPolicy(
        PalletData const& data);

    void Initialise();

    void Step();

    bool Terminate();

    Result Complete();
private:
    Particles particles_;

    PalletData historicalData_;

    std::minstd_rand rng_;

    double globalBestFitness_ = std::numeric_limits<double>::infinity();
    
    std::vector<double> globalBestPosition_;

    std::size_t iteration_{0};
};

template<typename ForwardIt>
double Estemate(PalletData& data, ForwardIt weightIt);

template<
    typename OutputIt,
    typename RngT>
void InitialiseRandomWeights(
    OutputIt first,
    OutputIt last,
    RngT& rng,
    double maxWeight = 1.0);

int main(int argc, char const** argv)
{
    auto runPSO = [](char const* arg)
    {
        PalletData data{arg};
        HyperPSOPolicy hyperPSO{data};
        auto HyperResult = Simulate(hyperPSO);

        typename BasicPSOPolicy::Parameters params;
        std::memcpy(&params, HyperResult.position.data(), sizeof(params));

        auto const Particles = static_cast<std::size_t>(
            20 + std::sqrt(data.DataCount()));
        BasicPSOPolicy pso{
            std::move(data),
            Particles,
            100000,
            params};
        auto result = Simulate(pso);
    
        std::cout << result.fitness << '|';
        for(auto&& x: result.position)
            std::cout << ' ' << x;

        std::cout << "| (";
        for(auto&& x: HyperResult.position)
            std::cout << ' ' << x;
        std::cout << " )\n";
    };

    if(1 < argc)
        std::for_each_n(
            std::execution::seq,
            argv + 1,
            argc - 1,
            runPSO);
    else
        runPSO("sample/cwk_train.csv");
}

BasicPSOPolicy::BasicPSOPolicy(
    PalletData historicalData,
    std::size_t populationSize,
    std::size_t iterations,
    Parameters parameters)
    : particles_{populationSize, historicalData.DataCount()}
    , historicalData_{std::move(historicalData)}
    , params_{parameters}
    , rngs_(populationSize)
    , MaxIteration{iterations}
{
}

void BasicPSOPolicy::Initialise() noexcept
{
    auto const count = particles_.VectorSize();

    std::random_device rand{};
    std::for_each(rngs_.begin(), rngs_.end(), [&](auto& rng)
    {
        rng.seed(rand());
    });

    particles_.ForAll([&](auto&& p)
    {
        InitialiseRandomWeights(p.position, p.position + count, rngs_[p.id]);
        std::copy(p.position, p.position + count, p.bestPosition);
        std::fill(p.velocity, p.velocity + count, 0.0);

        p.fitness = Estemate(historicalData_, p.position);
        p.bestFitness = p.fitness;
    });
}

void BasicPSOPolicy::Step() noexcept
{
    std::vector<double> bestPosition;
    auto globalBestCost = particles_.FindMin(
        std::back_inserter(bestPosition),
        [](auto const& a, auto const& b)
        {
            return a.fitness < b.fitness;
        });

    if(globalBestCost < globalBestFitness_)
    {
        globalBestFitness_ = globalBestCost;
        globalBestPosition_ =  std::move(bestPosition);
    }

    auto const count = particles_.VectorSize();
    particles_.ForAll([&](auto&& p)
    {
        NextPosition(
            p.position,
            p.position + count,
            p.bestPosition,
            globalBestPosition_.data(),
            p.velocity,
            p.position,
            rngs_[p.id],
            params_.inertia,
            params_.cogitiveAttraction,
            params_.socialAttraction);
        
        std::for_each(
            p.position,
            p.position + count,
            [](auto& x)
            {
                if(x < 0)
                    x = 0; 
            });

        p.fitness = Estemate(historicalData_, p.position);
        if (p.fitness < p.bestFitness)
        {
            p.bestFitness = p.fitness;
            std::copy(p.position, p.position + count, p.bestPosition);
        }
    });
}

bool BasicPSOPolicy::Terminate() noexcept
{
    return MaxIteration < ++iteration_;
}

typename BasicPSOPolicy::Result
BasicPSOPolicy::Complete()
    const noexcept
{
    return {globalBestPosition_, globalBestFitness_};
}

HyperPSOPolicy::HyperPSOPolicy(PalletData const& data)
    : particles_{21, 3}
    , historicalData_{data}
{
}

void HyperPSOPolicy::Initialise()
{
    auto const count = particles_.VectorSize();

    rng_.seed(std::random_device{}());
    particles_.ForEach([&](auto&& p)
    {
        InitialiseRandomWeights(p.position, p.position + count, rng_, 2);
        std::copy(p.position, p.position + count, p.bestPosition);
        std::fill(p.velocity, p.velocity + count, 0.0);

        typename BasicPSOPolicy::Parameters params;
        std::memcpy(&params, p.position, sizeof(typename BasicPSOPolicy::Parameters));
        BasicPSOPolicy subPSO{historicalData_, 20, 100, params};
        auto const SubResult = Simulate(subPSO);
        p.fitness = SubResult.fitness;
        p.bestFitness = p.fitness;
    });
}

void HyperPSOPolicy::Step()
{
    std::vector<double> bestPosition;
    auto globalBestCost = particles_.FindMin(
        std::back_inserter(bestPosition),
        [](auto const& a, auto const& b)
        {
            return a.fitness < b.fitness;
        });

    if(globalBestCost < globalBestFitness_)
    {
        globalBestFitness_ = globalBestCost;
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
        
        std::for_each(
            p.position,
            p.position + count,
            [](auto& x)
            {
                if(x < 0)
                    x = 0; 
            });

        typename BasicPSOPolicy::Parameters params;
        std::memcpy(&params, p.position, sizeof(typename BasicPSOPolicy::Parameters));
        BasicPSOPolicy subPSO{historicalData_, 20, 100, params};
        auto const SubResult = Simulate(subPSO);

        p.fitness = SubResult.fitness;
        if (p.fitness < p.bestFitness)
        {
            p.bestFitness = p.fitness;
            std::copy(p.position, p.position + count, p.bestPosition);
        }
    });
}

bool HyperPSOPolicy::Terminate()
{
    return 100 < ++iteration_;
}

typename HyperPSOPolicy:: Result HyperPSOPolicy::Complete()
{
    return {globalBestPosition_, globalBestFitness_};
}

template<typename ForwardIt>
double Estemate(PalletData& data, ForwardIt weightIt)
{
    std::vector<double> estemates{};
    for (std::size_t i{}; i != data.RowCount(); ++i)
        estemates.push_back(std::transform_reduce(
            std::execution::seq,
            data.BeginRowData(i),
            data.EndRowData(i),
            weightIt,
            double{},
            std::plus<double>{},
            std::multiplies<double>{}));

    auto const Total = std::transform_reduce(
        std::execution::seq,
        estemates.cbegin(),
        estemates.cend(),
        data.BeginDemand(),
        0.0,
        std::plus<double>{},
        [](auto a, auto b) noexcept {return std::abs(a - b); });

    return Total / data.RowCount();
}

template<
    typename OutputIt,
    typename RngT>
void InitialiseRandomWeights(
    OutputIt first,
    OutputIt last,
    RngT& rng,
    double maxWeight)
{
    std::uniform_real_distribution<double> d{ 0.0, maxWeight };
    std::generate(
        first,
        last,
        [&]() { return d(rng); });
}