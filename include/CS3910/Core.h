#ifndef CS3910__CORE_H_
#define CS3910__CORE_H_

#include <algorithm>
#include <cassert>
#include <execution>
#include <iterator>
#include <numeric>
#include <random>
#include <vector>

template<
    typename OutputIt,
    typename RngT>
void InitialiseRandomWeights(
    OutputIt first,
    OutputIt last,
    RngT& rng)
{
    std::uniform_real_distribution<double> d{0.0, 1.0};
    std::generate(
        first,
        last,
        [&](){ return d(rng); });
}


template<
    typename ForwardEstemateIt,
    typename ForwardDemandIt>
constexpr auto Cost(
    ForwardEstemateIt firstEst,
    ForwardEstemateIt lastEst,
    ForwardDemandIt demandIt)
    noexcept
{
    assert(firstEst != lastEst && "Cannot calculate costs of an empty range");
    auto const Count{static_cast<std::size_t>(std::distance(
        firstEst,
        lastEst))};

    auto Total = std::transform_reduce(
        std::execution::seq,
        firstEst,
        lastEst,
        demandIt,
        0.0,
        std::plus<double>{},
        [](auto a, auto b) noexcept {return std::abs(a - b);});

    return Total / Count;
}

template<
    typename ForwardIt,
    typename ForwardWeightIt>
constexpr double Estemate(
    ForwardIt first,
    ForwardIt last,
    ForwardWeightIt weightIt)
    noexcept
{
    return std::transform_reduce(
        std::execution::seq,
        first,
        last,
        weightIt,
        double{},
        std::plus<double>{},
        std::multiplies<double>{});
}

template<
    typename ForwardDataIt,
    typename ForwardDemandIt,
    typename ForwardWeightIt>
double EstemateCost(
    ForwardDataIt first,
    ForwardDataIt last,
    ForwardDemandIt demandIt,
    ForwardWeightIt weightIt)
{
    std::vector<double> estemates{};
    std::transform(
        first,
        last,
        std::back_inserter(estemates),
        [&](auto&& d) { return Estemate(d.begin(), d.end(), weightIt);});

    return Cost(estemates.begin(), estemates.end(), demandIt);
}

#endif // !CS3910__CORE_H_
