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

class GPPolicy final
{
public:
    struct Result
    {
        Expr function;
        double fitness;
    };

    explicit GPPolicy(
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

    constexpr static std::size_t MaxExpressionSize = 500;
    constexpr static std::size_t TournamentSize = 5;
    constexpr static std::size_t InitialDepth = 2;
    constexpr static std::size_t MutationDepth = 2;

    std::vector<Individual> population_;

    PalletData historicalData_;

    std::minstd_rand rng_{};

    double bestFitness_ = std::numeric_limits<double>::infinity();

    Expr bestFunction_;

    std::size_t iteration_{};
};

int main(int argc, char const** argv)
{
    auto runGP = [](char const* arg)
    {
        GPPolicy gp{ PalletData{arg}, 100 };
        auto result = Simulate(gp);

        std::cout
            << result.fitness << "| "
            << result.function << "\n";
    };

    if(1 < argc)
        std::for_each_n(
            std::execution::seq,
            argv + 1,
            argc - 1,
            runGP);
    else
        runGP("sample/cwk_train.csv");
}

GPPolicy::GPPolicy(
    PalletData historicalData,
    std::size_t populationSize)
    noexcept
    : population_{populationSize}
    , historicalData_{std::move(historicalData)}
{
}

void GPPolicy::Initialise()
{
    rng_.seed(std::random_device{}());
    std::generate(
        population_.begin(),
        population_.end(),
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

void GPPolicy::Step()
{
    std::vector<Individual> newGeneration{};
    while(newGeneration.size() < population_.size())
    {
        auto const X = std::uniform_real_distribution<>{0, 1}(rng_);
        if(X < 0.05)
        {
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
        else if(X < 0.55)
        {
            auto it = Roulette(
                population_.cbegin(),
                population_.cend(),
                rng_,
                [](auto const& c) noexcept { return 1 / c.fitness; });
            newGeneration.emplace_back(*it);
        }
        else
        {
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

            auto [childA, childB] = SubTreeCrossover(
                population_[0].function,
                population_[1].function,
                rng_);

            newGeneration.emplace_back(Individual{std::move(childA), 0.0});
            newGeneration.emplace_back(Individual{std::move(childB), 0.0});
        }
    }

    // Fit the next generation
    newGeneration.resize(population_.size());

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
                    2);
        });

    std::for_each(
        std::execution::par,
        newGeneration.begin(),
        newGeneration.end(),
        [&](auto& c)
        {
            c.fitness = Estemate(historicalData_, c.function);
        });
    population_ = std::move(newGeneration);

    auto i = std::min_element(
        std::execution::par,
        population_.begin(),
        population_.end(),
        [](auto const& a, auto const& b) {return a.fitness < b.fitness; });
    
    //std::cout << iteration_ << " " << i->fitness << '\n';
    //std::cout << i->function << '\n';
    if (i->fitness < bestFitness_)
    {
        bestFunction_ = i->function;
        bestFitness_ = i->fitness;
    }
}

bool GPPolicy::Terminate() noexcept
{
    return 10000 < ++iteration_;
}

typename GPPolicy::Result GPPolicy::Complete()
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
    if (maxDepth == 0)
    {
        auto const X = std::uniform_real_distribution<>{ 0, 1 }(rng);
        if(X < 0.25)
            return Const(std::uniform_real_distribution<>{ 0, 100 }(rng));
        else
            return Arg(std::uniform_int_distribution<std::uint64_t>{0, argCount - 1}(rng));
    }
    else
    {
        auto const Lhs = GenerateRandomExpr(rng, argCount, maxDepth - 1);
        auto const Rhs = GenerateRandomExpr(rng, argCount, maxDepth - 1);
        auto const X = std::uniform_real_distribution<>{ 0, 1 }(rng);
        if(X < 0.25)
            return Lhs + Rhs;
        else if (X < 0.50)
            return Lhs - Rhs;
        else if (X < 0.75)
            return Lhs * Rhs;
        else
            return Lhs / Rhs;
    }
}
