#ifndef CS3910__PSO_H_
#define CS3910__PSO_H_

#include <random>

template<
    typename RealT,
    typename ForwardPositionIt,
    typename InputVelocityIt,
    typename OutputVelocityIt,
    typename RngT>
void NextVelocity(
    ForwardPositionIt firstX,
    ForwardPositionIt lastX,
    ForwardPositionIt pBestIt,
    ForwardPositionIt gBestIt,
    InputVelocityIt vIt,
    OutputVelocityIt nextVIt,
    RngT& rng,
    RealT inertia,
    RealT cognitiveAttraction,
    RealT socialAttraction)
    noexcept
{
    std::uniform_real_distribution<> d{ 0.0, 1.0 };
    for(; firstX != lastX; ++firstX, ++pBestIt, ++gBestIt, ++vIt, ++nextVIt)
        *nextVIt = inertia * *vIt
            + cognitiveAttraction * d(rng) * (*gBestIt - *firstX)
            + socialAttraction * d(rng) * (*pBestIt - *firstX);
}

template<
    typename RealT,
    typename ForwardPositionIt,
    typename ForwardVelocityIt,
    typename OutputPositionIt,
    typename RngT>
void NextPosition(
    ForwardPositionIt firstX,
    ForwardPositionIt lastX,
    ForwardPositionIt pBestIt,
    ForwardPositionIt gBestIt,
    ForwardVelocityIt vIt,
    OutputPositionIt nextPositionIt,
    RngT& rng,
    RealT inertia,
    RealT cognitiveAttraction,
    RealT socialAttraction)
{
    NextVelocity(
        firstX,
        lastX,
        pBestIt,
        gBestIt,
        vIt,
        vIt,
        rng,
        inertia,
        cognitiveAttraction,
        socialAttraction);

    std::transform(
        firstX,
        lastX,
        vIt,
        nextPositionIt,
        std::plus<double>{});
}

#endif // !CS3910__PSO_H_
