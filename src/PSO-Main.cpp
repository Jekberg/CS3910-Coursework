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

template<typename ForwardIt>
double Estemate(PalletData& data, ForwardIt weightIt);

class PSOPalletDemandOptimisation
{
public:
    constexpr static auto StartingFitness
        = std::numeric_limits<double>::infinity();

    explicit PSOPalletDemandOptimisation(PalletData const& historicalData);

    void Init(Particles& particles);

    void Update(
        Particles& particles,
        double* globalBestPosition,
        PSOParameters const& params);

    double Evaluate(typename Particles::Individual const& particle);
    
    std::size_t Dimension();

    constexpr static bool Compare(double a, double b)
    {
        return a < b;
    }
private:
    PalletData historicalData_;

    std::vector<std::minstd_rand> rngs_;
};

using PSOPalletDemandMinimisation = BasicPSO<
    PalletData,
    PSOPalletDemandOptimisation>;


template<typename EnvT, typename PSOAlgorithmT>
class PSOMetaOptimisation
{
public:
    constexpr static auto StartingFitness
        = std::numeric_limits<double>::infinity();

    explicit PSOMetaOptimisation(EnvT const& env);

    void Init(Particles& particles);

    void Update(
        Particles& particles,
        double* globalBestPosition,
        PSOParameters const& params);

    double Evaluate(typename Particles::Individual const& particle);

    std::size_t Dimension()
    {
        return 3; // PSOParameter members
    }

    constexpr static bool Compare(double a, double b)
    {
        return a < b;
    }
private:
    EnvT env_;

    std::minstd_rand rng_;
};

using MetaPSOPalletDemandMinimisation = BasicPSO<
    PalletData,
    PSOMetaOptimisation<PalletData, PSOPalletDemandMinimisation>>;

int main(int argc, char const** argv)
{
    auto runPSO = [](char const* arg)
    {
        PalletData data{arg};
        MetaPSOPalletDemandMinimisation hyperPSO{data, 21, 100};
        auto hyperResult = Simulate(hyperPSO);
        
        PSOParameters params;
        std::memcpy(&params, hyperResult.position.data(), sizeof(params));
        
        auto const Particles = static_cast<std::size_t>(
            20 + std::sqrt(data.DataCount()));
        PSOPalletDemandMinimisation pso{
            data,
            Particles,
            100000,
            params};
        auto result = Simulate(pso);
    
        std::cout << result.fitness << '|';
        for(auto&& x: result.position)
            std::cout << ' ' << x;

        std::cout << "| (";
        for(auto&& x: hyperResult.position)
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

PSOPalletDemandOptimisation::PSOPalletDemandOptimisation(
    PalletData const& historicalData)
    : historicalData_{ historicalData }
{
}

void PSOPalletDemandOptimisation::Init(Particles& particles)
{
    auto const Count = particles.VectorSize();

    std::random_device rand{};
    rngs_.resize(particles.PopulationSize());
    std::for_each(rngs_.begin(), rngs_.end(), [&](auto& rng)
    {
        rng.seed(rand());
    });

    particles.ForAll([&](auto&& p)
    {
        std::generate(
            p.position,
            p.position + Count,
            [&]()
            {
                using Distribution = std::uniform_real_distribution<>;
                return Distribution{0, 1}(rngs_[p.id]);
            });
        std::copy(p.position, p.position + Count, p.bestPosition);
        std::fill(p.velocity, p.velocity + Count, 0.0);

        p.fitness = Evaluate(p);
        p.bestFitness = p.fitness;
    });
}

void PSOPalletDemandOptimisation::Update(
    Particles& particles,
    double* globalBestPosition,
    PSOParameters const& params)
{
    auto const Count = particles.VectorSize();
    particles.ForAll([&](auto&& p)
    {
        NextPosition(
            p.position,
            p.position + Count,
            p.bestPosition,
            globalBestPosition,
            p.velocity,
            p.position,
            rngs_[p.id],
            params);
    });
}

double PSOPalletDemandOptimisation::Evaluate(
    typename Particles::Individual const& particle)
{
    return Estemate(historicalData_, particle.position);
}

std::size_t PSOPalletDemandOptimisation::Dimension()
{
    return historicalData_.DataCount();
}

template<typename EnvT, typename PSOAlgorithmT>
PSOMetaOptimisation<EnvT, PSOAlgorithmT>::PSOMetaOptimisation(EnvT const& env)
    : env_{env}
{
}

template<typename EnvT, typename PSOAlgorithmT>
void PSOMetaOptimisation<EnvT, PSOAlgorithmT>::Init(Particles& particles)
{
    auto const count = particles.VectorSize();
    rng_.seed(std::random_device{}());
    particles.ForAll([&](auto&& p)
    {
        std::generate(
            p.position,
            p.position + count,
            [&]()
            {
                using Distribution = std::uniform_real_distribution<>;
                return Distribution{ 0, 2 }(rng_);
            });
        std::copy(p.position, p.position + count, p.bestPosition);
        std::fill(p.velocity, p.velocity + count, 0.0);

        p.fitness = Evaluate(p);
        p.bestFitness = p.fitness;
    });
}

template<typename EnvT, typename PSOAlgorithmT>
void PSOMetaOptimisation<EnvT, PSOAlgorithmT>::Update(
    Particles& particles,
    double* globalBestPosition,
    PSOParameters const& params)
{
    auto const Count = particles.VectorSize();
    particles.ForEach([&](auto&& p)
    {
        NextPosition(
            p.position,
            p.position + Count,
            p.bestPosition,
            globalBestPosition,
            p.velocity,
            p.position,
            rng_,
            params);

        std::for_each(
            p.position,
            p.position + Count,
            [](auto& x)
            {
                if (x < 0)
                    x = 0;
            });
    });
}

template<typename EnvT, typename PSOAlgorithmT>
double PSOMetaOptimisation<EnvT, PSOAlgorithmT>::Evaluate(
    typename Particles::Individual const& particle)
{
    PSOParameters params;
    std::memcpy(&params, particle.position, sizeof(params));
    PSOAlgorithmT sub{env_, 20, 100, params};
    auto subResult = Simulate(sub);
    return subResult.fitness;
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