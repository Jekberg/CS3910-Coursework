#include "CS3910/Simulation.h"
#include "CS3910/Pallets.h"
#include "CS3910/GP.h"
#include <execution>
#include <iostream>

double Estemate(PalletData& data, Expr const& expr);

struct Candidate
{
    Expr function;
    double fitness;
};

template<typename RngT>
std::pair<Expr, Expr> Crossover(
    Expr const& parentA,
    Expr const& parentB,
    RngT& rng)
{
    auto a{parentA};
    auto b{parentB};

    auto const CountA = parentA.Count();
    auto const CountB = parentB.Count();

    auto const NodeA = std::uniform_int_distribution<std::size_t>{1, CountA - 1}(rng);
    auto const NodeB = std::uniform_int_distribution<std::size_t>{1, CountB - 1}(rng);

    a.Replace(NodeA, parentB.SubExpr(NodeB));
    b.Replace(NodeB, parentA.SubExpr(NodeA));

    return {a, b};
}

template<typename RandomIt, typename RngT>
RandomIt SampleGroup(RandomIt first, RandomIt last, std::size_t k, RngT& rng)
{
    // Move k elements selected at random to the front of the range.
    assert(first != last);
    assert(k != 0);
    assert(first + k <= last);

    for (auto i = first; i != first + k; ++i)
    {
        auto c = std::uniform_int_distribution<std::size_t>{
            0,
            static_cast<std::size_t>(std::distance(i, last) - 1) }(rng);
        if(i != i + c)
            std::swap(*i, i[c]);
    }

    // The end of the sample group
    return first + k;
}

class GAPolicy final
{
public:
    explicit GAPolicy(
        PalletData historicalData,
        std::size_t populationSize)
        noexcept;

    void Initialise();

    void Step();

    void Complete()
    {

    }

    bool Terminate() noexcept;
private:
    struct Candidate
    {
        Expr function;
        double fitness;
    };

    std::vector<Candidate> population_;

    PalletData historicalData_;

    std::random_device rng_{};

    double bestFitness_ = std::numeric_limits<double>::infinity();

    std::size_t iteration_{};
};

//template<typename RandomIt, typename RngT>
//std::tuple<Candidate, Candidate, RandomIt> SelectParents(
//    RandomIt first,
//    RandomIt last,
//    RngT& rng)
//{
//    auto const K = 2;
//    if (first + K != last)
//    {
//        // Find the first parent
//        auto it = SampleGroup(first, last, params_.k, rng_);
//        auto minIt = std::min_element(
//            first,
//            it,
//            [](auto& a, auto& b) { return a.cost < b.cost; });
//        if(first != minIt)
//            std::swap(first[0], *minIt);
//
//        auto& parentA = first[0];
//
//        // Find the second parent
//        it = SampleGroup(first + 1, last, params_.k, rng_);
//        minIt = std::min_element(
//            first + 1,
//            it,
//            [](auto& a, auto& b)
//            {
//                return a.cost < b.cost;
//            });
//        if (first + 1 != minIt)
//            std::swap(first[1], *minIt);
//    }
//
//    return {first[0], first[1], first + 2};
//}

//template<typename RandomIt>
//void SelectNext(
//    RandomIt first,
//    RandomIt last)
//{
//    for (auto i = population_.get(); i != population_.get() + params_.eliteSize; ++i)
//    {
//        auto it = Roulette(
//            i,
//            PopulationEnd,
//            rng_,
//            [](auto& path) {return 1 / path.cost; });
//        if(i != it)
//            std::swap(*i, *it);
//    }
//
//    MoveRandom(first, last, population_.get() + params_.eliteSize, PopulationEnd, rng_);
//}

template<typename ForwardIt, typename RngT, typename F>
ForwardIt Roulette(ForwardIt first, ForwardIt last, RngT& rng, F&& f)
{
    auto const Total = std::accumulate(
        first,
        last,
        double{}, [&](auto&& acc, auto&& x)
        {
            return acc + f(x);
        });

    auto r = std::uniform_real_distribution<>{ 0.0, Total }(rng);
    for (auto i = first; i != last; ++i)
        if (Total <= (r += f(*i)))
            return i;
    return last;
}

template<typename RandomItFrom, typename ForwardItTo, typename RngT>
void MoveRandom(
    RandomItFrom firstFrom,
    RandomItFrom lastFrom,
    ForwardItTo firstTo,
    ForwardItTo lastTo,
    RngT& rng)
{
    for (; firstFrom != lastFrom && firstTo != lastTo; ++firstFrom, ++firstTo)
    {
        auto const Length{ static_cast<std::size_t>(std::distance(
            firstFrom,
            lastFrom)) };
        auto dis{ std::uniform_int_distribution<std::size_t>{0, Length - 1} };
        auto toMoveIt = firstFrom + dis(rng);
        std::swap(*firstFrom, *toMoveIt);
        *firstTo = std::move(*firstFrom);
    }
}

template<typename RngT>
Expr GenerateRandomExpr(
    RngT& rng,
    std::uint64_t argCount,
    std::size_t maxDepth)
{
    if (maxDepth == 0)
        return std::uniform_real_distribution<>{ 0, 1 }(rng) < 1.0 / 2.0
            ? Const(std::uniform_real_distribution<>{ 0, 1000 }(rng))
            : Arg(std::uniform_int_distribution<std::uint64_t>{0, argCount - 1}(rng));
    else
        return std::uniform_real_distribution<>{ 0, 1 }(rng) < 1.0 / 2.0
            ? GenerateRandomExpr(rng, argCount, maxDepth - 1) + GenerateRandomExpr(rng, argCount, maxDepth - 1)
            : GenerateRandomExpr(rng, argCount, maxDepth - 1) * GenerateRandomExpr(rng, argCount, maxDepth - 1);
}

int main()
{
    GAPolicy gp{PalletData{"sample/cwk_train.csv"}, 100};
    Simulate(gp);
}

GAPolicy::GAPolicy(
    PalletData historicalData,
    std::size_t populationSize)
    noexcept
    : population_{populationSize}
    , historicalData_{std::move(historicalData)}
{
}

void GAPolicy::Initialise()
{
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

void GAPolicy::Step()
{
    auto const Elite = population_.size() / 2;
    auto const K = 2;
    std::vector<Candidate> intermidiate{};
    // Select & Crossover
    for (auto i{ population_.begin() }; i != population_.end();)
    {
        if(std::distance(i, population_.end()) <= K)
            break;
        
        // Parent A...
        auto j = SampleGroup(i, population_.end(), K, rng_);
        auto mintIt = std::min_element(
            i,
            j,
            [](auto& a, auto& b){return a.fitness < b.fitness;});
        std::swap(*mintIt, *i);

        // Parent B...
        j = SampleGroup(i + 1, population_.end(), K, rng_);
        mintIt = std::min_element(
            i + 1,
            j,
            [](auto& a, auto& b){return a.fitness < b.fitness;});
        std::swap(*mintIt, i[1]);

        auto& parentA = i[0];
        auto& parentB = i[1];

        auto [childA, childB] = Crossover(parentA.function, parentB.function, rng_);
        i += 2;

        auto const FitnessA = Estemate(historicalData_, childA);
        auto const FitnessB = Estemate(historicalData_, childB);
        intermidiate.emplace_back(Candidate{std::move(childA), FitnessA});
        intermidiate.emplace_back(Candidate{std::move(childB), FitnessB });
    }

    // Next gen
    for(auto i = population_.begin(); i != population_.begin() + Elite; ++i)
    {
        auto it = Roulette(
            i,
            population_.end(),
            rng_,
            [](auto& c) {return 1 / c.fitness; });
        if(i != it)
            std::swap(*i, *it);
    }

    std::for_each(
        population_.begin(),
        population_.begin() + Elite,
        [&](auto& c)
        {
            if (std::uniform_real_distribution<double>{0, 1}(rng_) < 0.05)
            {
                auto x = std::uniform_int_distribution<std::size_t>{1, c.function.Count() - 1}(rng_);
                c.function.Replace(x, GenerateRandomExpr(rng_, historicalData_.DataCount(), 1));
            }
        });

    MoveRandom(
        intermidiate.begin(),
        intermidiate.end(),
        population_.begin() + Elite,
        population_.end(),
        rng_);

    auto i = std::min_element(
        population_.begin(),
        population_.end(),
        [](auto a, auto b) {return a.fitness < b.fitness; });

    //if (i->fitness < bestFitness_)
    {
        bestFitness_ = i->fitness;
        std::cout
            << "\n\n>>> " << iteration_ << ": "
            << i->fitness
            << " [" << i->function << "]\n";
    }
}

double Estemate(PalletData& data, Expr const& expr)
{
    std::vector<double> estemates{};
    for(std::size_t i{}; i != data.RowCount(); ++i)
        estemates.push_back(expr.Eval(data.BeginRowData(i)));

    auto const Total = std::transform_reduce(
        std::execution::seq,
        estemates.cbegin(),
        estemates.cend(),
        data.BeginDemand(),
        0.0,
        std::plus<double>{},
        [](auto a, auto b) noexcept {return std::abs(a - b);});

    return Total / data.RowCount();
}

bool GAPolicy::Terminate() noexcept
{
    return 10000 < ++iteration_;
}