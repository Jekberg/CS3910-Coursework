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

    constexpr static std::size_t MaxExpressionSize = 300;
    constexpr static std::size_t TournamentSize = 5;
    constexpr static std::size_t InitialDepth = 2;
    constexpr static std::size_t MutationDepth = 2;
    constexpr static double MutationProbabillity = 0.05;

    std::vector<Individual> population_;

    PalletData historicalData_;

    std::minstd_rand rng_{};

    double bestFitness_ = std::numeric_limits<double>::infinity();

    Expr bestFunction_;

    std::size_t iteration_{};

    template<
        typename ForwardIt,
        typename OutputIt,
        typename RngT>
    void SelectAndCrossover(
        ForwardIt first,
        ForwardIt last,
        OutputIt outIt,
        RngT& rng)
    {
        assert(std::distance(first, last) < 3);

        // The location where the parents can be found
        auto parentIt = first;

        // Find the first parent using a tornament
        auto i = SampleGroup(first, last, TournamentSize, rng);
        i = std::min_element(
            first,
            i,
            [](auto const& a, auto const& b) {return a.fitness < b.fitness;});
        std::iter_swap(first, i);
        ++first;

        // Find the second parent using a tornament
        i = SampleGroup(first, last, TournamentSize, rng);
        i = std::min_element(
            first,
            i,
            [](auto const& a, auto const& b) {return a.fitness < b.fitness;});
        std::iter_swap(first, i);

        auto childA = SubTreeCrossover(parentIt->function, std::next(parentIt)->function, rng);
        auto childB = SubTreeCrossover(std::next(parentIt)->function, parentIt->function, rng);
        *(outIt++) = Individual{std::move(childA), 0};
        *(outIt++) = Individual{std::move(childB), 0};
    }

    template<
        typename ForwardIt,
        typename OutputIt,
        typename RngT>
    void SelectAndReplicate(
        ForwardIt first,
        ForwardIt last,
        OutputIt outIt,
        RngT& rng)
    {
        assert (it == last);
        auto it = Roulette(
            first,
            last,
            rng,
            [](auto const& c) noexcept { return 1 / c.fitness; });

        if(std::uniform_real_distribution<>{0, 1}(rng) < MutationProbabillity)
        {
            auto temp = it->function;
            using Distribution = std::uniform_int_distribution<std::size_t>;
            auto const Id = Distribution{1, temp.Count() - 1}(rng);
            temp.Replace(
                Id,
                GenerateRandomExpr(
                    rng,
                    historicalData_.DataCount(),
                    MutationDepth));
            *outIt = Individual{std::move(temp), 0};
        }
        else
            *outIt = *it;
        ++outIt;
    }
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
        if(std::uniform_real_distribution<>{0, 1}(rng_) < 0.5)
            SelectAndReplicate(
                population_.begin(),
                population_.end(),
                std::back_inserter(newGeneration),
                rng_);
        else
            SelectAndCrossover(
                population_.begin(),
                population_.end(),
                std::back_inserter(newGeneration),
                rng_);
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