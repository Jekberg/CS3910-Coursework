#ifndef CS3910__PSO_H_
#define CS3910__PSO_H_

template<
    typename ForwardPosIt,
    typename RngT>
void Velocity(
    ForwardPosIt position,
    double* velocity,
    double const* personalBest,
    double const* globalBest,
    RngT& rng)
{
    std::uniform_real_distribution<> d{ 0.0, 1.0 };

    for (auto i = 0; i < env_.count() - 1; ++i)
    {
        velocity[i] = params_.n * velocity[i]
            + params_.o1 * d(rng) * (globalBest[i] - position[i])
            + params_.o2 * d(rng) * (personalBest[i] - position[i]);
        position[i] += velocity[i];
    }
}

#endif // !CS3910__PSO_H_
