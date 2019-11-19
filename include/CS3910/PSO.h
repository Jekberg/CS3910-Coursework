#ifndef CS3910__PSO_H_
#define CS3910__PSO_H_

#include <random>

class ParticleSwarmPopulation
{
public:
    struct Candidate
    {
        double* position;
        double* velocity;
        double* bestPosition;
        double* fitness;
        double* bestFitness;
    };

    explicit ParticleSwarmPopulation(
        std::size_t populationSize,
        std::size_t count);

    template<typename Consumer>
    void ForEach(Consumer&& consumer);

    template<typename OutputIt, typename Compare>
    double FindMin(OutputIt outIt, Compare&& compare)
    {
        auto it = std::min_element(
            population_.begin(),
            population_.end(),
            compare);
        std::copy(it->position, it->position + vectorSize_, outIt);
        return *it->fitness;
    }

    constexpr std::size_t VectorSize() const noexcept;

    std::size_t PopulationSize() const noexcept;

private:
    std::vector<double> positions_;
    std::vector<double> velocities_;
    std::vector<double> bestPositions_;
    std::vector<double> fitness_;
    std::vector<double> bestFitness_;
    std::vector<Candidate> population_;
    std::size_t populationSize_;
    std::size_t vectorSize_;
};

ParticleSwarmPopulation::ParticleSwarmPopulation(
    std::size_t populationSize,
    std::size_t vectorSize)
    : positions_(populationSize * vectorSize)
    , velocities_(populationSize * vectorSize)
    , bestPositions_(populationSize * vectorSize)
    , fitness_(populationSize)
    , bestFitness_(populationSize)
    , population_(populationSize)
    , populationSize_{populationSize}
    , vectorSize_{ vectorSize }
{

    for (std::size_t i{}; i != population_.size(); ++i)
    {
        auto& p{population_[i]};
        p.position = positions_.data() + vectorSize * i;
        p.velocity = velocities_.data() + vectorSize * i;
        p.bestPosition = bestPositions_.data() + vectorSize * i;
        p.fitness = fitness_.data() + i;
        p.bestFitness = bestFitness_.data() + i;
    }
}

template<typename Consumer>
void ParticleSwarmPopulation::ForEach(Consumer&& consumer)
{
    std::for_each(
        population_.begin(),
        population_.end(),
        consumer);
}

constexpr std::size_t ParticleSwarmPopulation::VectorSize() const noexcept
{
    return vectorSize_;
}

std::size_t ParticleSwarmPopulation::PopulationSize() const noexcept
{
    return population_.size();
}

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
