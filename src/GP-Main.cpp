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

    auto const NodeA = std::uniform_int_distribution<std::size_t>{0, CountA - 1}(rng);
    auto const NodeB = std::uniform_int_distribution<std::size_t>{0, CountB - 1}(rng);

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

template<typename RandomIt, typename RngT>
std::tuple<Candidate, Candidate, RandomIt> SelectParents(
    RandomIt first,
    RandomIt last,
    RngT& rng)
{
    auto const K = 2;
    if (first + K != last)
    {
        // Find the first parent
        auto it = SampleGroup(first, last, params_.k, rng_);
        auto minIt = std::min_element(
            first,
            it,
            [](auto& a, auto& b) { return a.cost < b.cost; });
        if(first != minIt)
            std::swap(first[0], *minIt);

        auto& parentA = first[0];

        // Find the second parent
        it = SampleGroup(first + 1, last, params_.k, rng_);
        minIt = std::min_element(
            first + 1,
            it,
            [](auto& a, auto& b)
            {
                return a.cost < b.cost;
            });
        if (first + 1 != minIt)
            std::swap(first[1], *minIt);
    }

    return {first[0], first[1], first + 2};
}

template<typename RandomIt>
void SelectNext(
    RandomIt first,
    RandomIt last)
{
    for (auto i = population_.get(); i != population_.get() + params_.eliteSize; ++i)
    {
        auto it = Roulette(
            i,
            PopulationEnd,
            rng_,
            [](auto& path) {return 1 / path.cost; });
        if(i != it)
            std::swap(*i, *it);
    }

    MoveRandom(first, last, population_.get() + params_.eliteSize, PopulationEnd, rng_);
}

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

void S()
{
    double best = std::numeric_limits<double>::infinity();
    PalletData data{ "sample/cwk_train.csv" };

    std::random_device rng{};
    std::vector<Candidate> population{};
    std::generate_n(
        std::back_inserter(population),
        1000,
        [&]()
        {
            Candidate c{ Expr{rng, data.DataCount()} , 0.0};
            c.fitness = Estemate(data, c.function);
            return c;
        });

    //for(auto&& c: population)
    //    std::cout
    //        << "[" << c.function.Count() << "] "
    //        << c.fitness
    //        << " <-- " << c.function << '\n';

    auto const Elite = population.size() / 2;
    for (std::size_t count{};; ++count)
    {
        auto const K = 2;
        std::vector<Candidate> intermidiate{};
        // Select & Crossover
        for (auto i{ population.begin() }; i != population.end();)
        {
            if(std::distance(i, population.end()) < 2)
                break;
            auto j = SampleGroup(i, population.end(), K, rng);
            auto& parentA = *i;
            auto& parentB = *j;

            auto [childA, childB] = Crossover(parentA.function, parentB.function, rng);
            i = SampleGroup(j, population.end(), K, rng);

            auto const FitnessA = Estemate(data, childA);
            auto const FitnessB = Estemate(data, childB);
            intermidiate.emplace_back(Candidate{std::move(childA), FitnessA});
            intermidiate.emplace_back(Candidate{std::move(childB), FitnessB });
        }

        // Next gen
        for(auto i = population.begin(); i != population.begin() + Elite; ++i)
        {
            auto it = Roulette(
                i,
                population.end(),
                rng,
                [](auto& c) {return 1 / c.fitness; });
            if(i != it)
                std::swap(*i, *it);
        }

        std::for_each(
            population.begin(),
            population.begin() + Elite,
            [&](auto& c)
            {
                if (std::uniform_real_distribution<double>{0, 1}(rng) < 0.05)
                {
                    auto x = std::uniform_int_distribution<std::size_t>{0, c.function.Count() - 1}(rng);
                    c.function.Replace(x, Expr{rng, data.DataCount()});
                }
            });

        MoveRandom(
            intermidiate.begin(),
            intermidiate.end(),
            population.begin() + Elite,
            population.end(),
            rng);


        auto i = std::min_element(
            population.begin(),
            population.end(),
            [](auto a, auto b) {return a.fitness < b.fitness; });

        if (i->fitness < best)
        {
            best = i->fitness;
            std::cout
                << ">>> " << count << ": "
                << i->fitness
                << " [" << i->function << "]\n";
        }
    }
}

int main()
{
    S();
    //PalletData data{"sample/cwk_train.csv"};
    //std::random_device rng{};
    //
    //double ar[1];
    //
    //auto a = Add(Expr{100}, Mul(Expr{100}, Expr{2}));
    //auto b = Add(Expr{3}, Add(Expr{1}, Mul(Expr{1}, Expr{2})));
    //
    //auto [x, y] = Crossover(a, b, rng);
    //std::cout << x << '\n';
    //std::cout << y << '\n';

    //for(std::size_t c{}; c < 10000; ++c)
    //{
    //    Expr x{rng, data.DataCount()};
    //    std::cout <<"[" << x.Count() << "] " << Estemate(data, x) << " <-- " << x << '\n';
    //}

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
        [](auto a, auto b) noexcept {return std::abs(a - b); });

    return Total / data.RowCount();
}