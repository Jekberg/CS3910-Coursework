#include "CS3910/Core.h"
#include "CS3910/Simulation.h"
#include "CS3910/Pallets.h"
#include "CS3910/GP.h"
#include <cmath>
#include <execution>
#include <iostream>

double Estemate(PalletData& data, Expr const& expr);

template<typename RngT>
Expr GenerateRandomExpr(
    RngT& rng,
    std::uint64_t argCount,
    std::size_t maxDepth);

class GPPalletDemandMinimisation final
{
public:
    struct Result
    {
        Expr function;
        double fitness;
    };

    explicit GPPalletDemandMinimisation(
        PalletData historicalData,
        std::size_t populationSize)
        noexcept;

    void Initialise();

    void Step();

    bool Terminate() noexcept;
    
    Result Complete();
private:
    struct Individual
    {
        Expr function;
        double fitness;
    };

    // Configuration
    constexpr static std::size_t MaxExpressionSize = 1000;
    constexpr static std::size_t TournamentSize = 4;
    constexpr static std::size_t InitialDepth = 2;
    constexpr static std::size_t MutationDepth = 2;
    constexpr static std::size_t MaxIteration = 1000;
    constexpr static double MutationPropability = 0.05;
    constexpr static double ReplicationPropabillity = 0.15;
    constexpr static double CrossoverProbabillity = 1 - (MutationPropability
        + ReplicationPropabillity);

    std::vector<Individual> population_;

    PalletData historicalData_;

    std::minstd_rand rng_{};

    double bestFitness_ = std::numeric_limits<double>::infinity();

    Expr bestFunction_ = Const(0);

    std::size_t iteration_{};

    std::size_t PopulationSize_;
};

int main(int argc, char const** argv) try
{
    auto dataSet = ReadPalletData(argc, argv, std::cout);

    auto const PopulationSize = 100;
    GPPalletDemandMinimisation gp{ dataSet.trainingData, PopulationSize };
    auto result = Simulate(gp);

    std::cout
        << Estemate(dataSet.testingData, result.function) << "| "
        << result.function << "\n";
}
catch (InvalidFileName& e)
{
    std::cout << "Cannot read file: " << e.what();
}
catch (std::exception& e)
{
    std::cout << e.what();
}

GPPalletDemandMinimisation::GPPalletDemandMinimisation(
    PalletData historicalData,
    std::size_t populationSize)
    noexcept
    : population_{}
    , historicalData_{std::move(historicalData)}
    , PopulationSize_{populationSize}
{
}

void GPPalletDemandMinimisation::Initialise()
{
    rng_.seed(std::random_device{}());
    population_.clear();
    std::generate_n(
        std::back_inserter(population_),
        PopulationSize_,
        [&]()
        {
            Individual c{
                GenerateRandomExpr(
                    rng_,
                    historicalData_.DataCount(),
                    InitialDepth),
                0.0};
            c.fitness = Estemate(historicalData_, c.function);
            return c;
        });

}

void GPPalletDemandMinimisation::Step()
{
    std::vector<Individual> newGeneration{};
    while(newGeneration.size() < population_.size())
    {
        auto const Total = MutationPropability + ReplicationPropabillity
            + CrossoverProbabillity;
        auto const X = std::uniform_real_distribution<>{0, Total }(rng_);
        if(X < MutationPropability)
        {
            // Select and mutate
            auto it = Roulette(
                population_.cbegin(),
                population_.cend(),
                rng_,
                [](auto const& c) noexcept { return 1 / c.fitness; });

            auto temp = it->function;
            using Distribution = std::uniform_int_distribution<std::size_t>;
            auto const Id = Distribution{ 1, temp.Count() - 1 }(rng_);
            temp.Replace(
                Id,
                GenerateRandomExpr(
                    rng_,
                    historicalData_.DataCount(),
                    MutationDepth));

            newGeneration.emplace_back(Individual{std::move(temp), 0.0});
        }
        else if(X < MutationPropability + ReplicationPropabillity)
        {
            // Select and replicate
            auto it = Roulette(
                population_.cbegin(),
                population_.cend(),
                rng_,
                [](auto const& c) noexcept { return 1 / c.fitness; });
            newGeneration.emplace_back(*it);
        }
        else
        {
            // Select and crossover
            // Find the first parent using a tornament
            auto i = SampleGroup(
                population_.begin(),
                population_.end(),
                TournamentSize, rng_);
            i = std::min_element(
                population_.begin(),
                i,
                [](auto const& a, auto const& b) noexcept
                {
                    return a.fitness < b.fitness;
                });
            std::iter_swap(population_.begin(), i);

            // Find the second parent using a tornament
            i = SampleGroup(
                population_.begin() + 1,
                population_.end(),
                TournamentSize,
                rng_);
            i = std::min_element(
                population_.begin(),
                i,
                [](auto const& a, auto const& b) noexcept
                {
                    return a.fitness < b.fitness;
                });
            std::iter_swap(population_.begin() + 1, i);

            auto [childA, childB] = SubtreeCrossover(
                population_[0].function,
                population_[1].function,
                rng_);

            newGeneration.emplace_back(Individual{std::move(childA), 0.0});
            newGeneration.emplace_back(Individual{std::move(childB), 0.0});
        }
    }

    std::for_each(newGeneration.begin(),
        newGeneration.end(),
        [&](auto& c)
        {
            // Control the growth
            // weed out the large trees and replace them with new ones
            if(MaxExpressionSize < c.function.Count())
                c.function = GenerateRandomExpr(
                    rng_,
                    historicalData_.DataCount(),
                    InitialDepth);
        });

    // Evaluate all the new individuals
    std::for_each(
        std::execution::par,
        newGeneration.begin(),
        newGeneration.end(),
        [&](auto& c)
        {
            c.fitness = Estemate(historicalData_, c.function);
        });


    // Fit the next generation
    while(population_.size() < newGeneration.size())
    {
        auto it = std::max_element(
            std::execution::par,
            newGeneration.cbegin(),
            newGeneration.cend(),
            [](auto const& a, auto const& b) noexcept
            {
                return a.fitness < b.fitness;
            });
        newGeneration.erase(it);
    }

    // Next generation
    population_ = std::move(newGeneration);

    // Find the best individual
    auto i = std::min_element(
        std::execution::par,
        population_.cbegin(),
        population_.cend(),
        [](auto const& a, auto const& b) noexcept
        {
            return a.fitness < b.fitness;
        });

    // Is the best individual the best overall?
    if (i->fitness < bestFitness_)
    {
        // Uncomment this to see the development... Useful for debugging!
        //std::cout << ">>> " << iteration_
        //    << ": " << i->fitness
        //    << " [" << i->function << "]\n";
        bestFunction_ = i->function;
        bestFitness_ = i->fitness;
    }
}

bool GPPalletDemandMinimisation::Terminate() noexcept
{
    return MaxIteration < ++iteration_;
}

typename GPPalletDemandMinimisation::Result
GPPalletDemandMinimisation::Complete()
{
    return {bestFunction_, bestFitness_};
}

double Estemate(PalletData& data, Expr const& expr)
{
    std::vector<double> estemates{};
    for(std::size_t i{}; i != data.RowCount(); ++i)
        estemates.push_back(expr.Eval(data.BeginRowData(i)));

    auto const Total = std::transform_reduce(
        std::execution::par,
        estemates.cbegin(),
        estemates.cend(),
        data.BeginDemand(),
        0.0,
        std::plus<double>{},
        [](auto a, auto b) noexcept {return std::abs(a - b);});

    auto const Estemation = Total / data.RowCount();
    return std::isnan(Estemation)
        ? std::numeric_limits<double>::infinity()
        : Estemation;
}

template<typename RngT>
Expr GenerateRandomExpr(
    RngT& rng,
    std::uint64_t argCount,
    std::size_t maxDepth)
{
    auto const X = std::uniform_real_distribution<>{ 0, 1 }(rng);
    if (maxDepth == 0)
    {
        if(X < 0.50)
            return Const(std::uniform_real_distribution<>{ 0, 100 }(rng));
        else
            return Arg(std::uniform_int_distribution<std::uint64_t>{0, argCount - 1}(rng));
    }
    else
    {
        auto const Lhs = GenerateRandomExpr(rng, argCount, maxDepth - 1);
        auto const Rhs = GenerateRandomExpr(rng, argCount, maxDepth - 1);
        if(X < 0.33)
            return Lhs + Rhs;
        else if (X < 0.67)
            return Lhs - Rhs;
        else //if (X < 0.90)
            return Lhs * Rhs;
        // Division seem to be a bit dangerous since division by a small
        // number will generate VERY large numbers... so forbid division.
        //else
        //    return Lhs / Rhs;
    }
}
