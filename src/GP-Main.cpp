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
    struct Candidate
    {
        Expr function;
        double fitness;
    };

    constexpr static std::size_t MaxExpressionSize = 300;
    constexpr static double MutationProbabillity = 0.05;

    std::vector<Candidate> population_;

    PalletData historicalData_;

    std::minstd_rand rng_{};

    double bestFitness_ = std::numeric_limits<double>::infinity();

    Expr bestFunction_;

    std::size_t iteration_{};

    template<
        typename ForwardIt,
        typename OutputIt,
        typename RngT>
    ForwardIt SelectAndCrossover(
        ForwardIt first,
        ForwardIt last,
        OutputIt outIt,
        RngT& rng)
    {
        if(std::distance(first, last) < 3)
            // Can't crossover fewwer than 2 individuals so do not proceed...
            return first;

        // The location where the parents can be found
        auto parentIt = first;
        std::size_t const TournamentSize = 5;

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
        *(outIt++) = Candidate{std::move(childA), 0};
        *(outIt++) = Candidate{std::move(childB), 0};
        return std::next(first);
    }

    template<
        typename ForwardIt,
        typename OutputIt,
        typename RngT>
    ForwardIt SelectAndReplicate(
        ForwardIt first,
        ForwardIt last,
        OutputIt outIt,
        RngT& rng)
    {
        auto it = Roulette(
            first,
            last,
            rng,
            [](auto const& c) noexcept { return 1 / c.fitness; });

        if (it == last)
            return it;

        if(std::uniform_real_distribution<>{0, 1}(rng) < MutationProbabillity)
        {
            auto& temp = it->function;
            using Distribution = std::uniform_int_distribution<std::size_t>;
            auto const Id = Distribution{1, temp.Count() - 1}(rng);
            temp.Replace(Id, GenerateRandomExpr(rng, historicalData_.DataCount(), 2));
        }

        *outIt = std::move(*it);
        ++outIt;
        std::iter_swap(first, it);
        return std::next(first);
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
            Candidate c{ GenerateRandomExpr(rng_, historicalData_.DataCount(), 1) , 0.0};
            c.fitness = Estemate(historicalData_, c.function);
            return c;
        });

}

void GPPolicy::Step()
{
    std::vector<Candidate> newGeneration{};
    for(auto i = population_.begin(); i != population_.end();)
    {
        i = SelectAndReplicate(
            i,
            population_.end(),
            std::back_inserter(newGeneration),
            rng_);
        i = SelectAndCrossover(
            i,
            population_.end(),
            std::back_inserter(newGeneration),
            rng_);
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
        return std::uniform_real_distribution<>{ 0, 1 }(rng) < 0.5
            ? Const(std::uniform_real_distribution<>{ 0, 100 }(rng))
            : Arg(std::uniform_int_distribution<std::uint64_t>{0, argCount - 1}(rng));
    else
        return std::uniform_real_distribution<>{ 0, 1 }(rng) < 0.5
            ? GenerateRandomExpr(rng, argCount, maxDepth - 1) + GenerateRandomExpr(rng, argCount, maxDepth - 1)
            : GenerateRandomExpr(rng, argCount, maxDepth - 1) * GenerateRandomExpr(rng, argCount, maxDepth - 1);
}