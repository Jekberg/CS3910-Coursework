#ifndef CS3910__PSO_H_
#define CS3910__PSO_H_

#include <execution>
#include <random>

struct PSOParameters
{
    double inertia;
    double cognitiveAttraction;
    double socialAttraction;
};

template<
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
    PSOParameters const& params)
    noexcept;

template<
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
    PSOParameters const& params);

class Particles
{
public:
    struct Individual
    {
        std::size_t id;
        double* position;
        double* velocity;
        double* bestPosition;
        double& fitness;
        double& bestFitness;
    };

    explicit Particles(
        std::size_t populationSize,
        std::size_t count);

    template<typename Consumer>
    void ForEach(Consumer&& consumer);

    template<typename Consumer>
    void ForAll(Consumer&& consumer);

    constexpr std::size_t VectorSize() const noexcept;

    std::size_t PopulationSize() const noexcept;

    template<typename Compare>
    Individual FindBest(Compare&& compare);
private:
    std::vector<double> positions_;

    std::vector<double> velocities_;

    std::vector<double> bestPositions_;

    std::vector<double> fitness_;

    std::vector<double> bestFitness_;

    std::vector<Individual> population_;

    std::size_t populationSize_;

    std::size_t vectorSize_;
};

Particles::Particles(
    std::size_t populationSize,
    std::size_t vectorSize)
    : positions_(populationSize * vectorSize)
    , velocities_(populationSize * vectorSize)
    , bestPositions_(populationSize * vectorSize)
    , fitness_(populationSize)
    , bestFitness_(populationSize)
    , population_{}
    , populationSize_{populationSize}
    , vectorSize_{ vectorSize }
{

    for (std::size_t i{}; i != populationSize_; ++i)
    {
        population_.emplace_back(Individual{
            i,
            positions_.data() + vectorSize * i,
            velocities_.data() + vectorSize * i,
            bestPositions_.data() + vectorSize * i,
            fitness_[i],
            bestFitness_[i]});
    }
}

template<typename Consumer>
void Particles::ForEach(Consumer&& consumer)
{
    std::for_each(
        population_.begin(),
        population_.end(),
        consumer);
}

template<typename Consumer>
void Particles::ForAll(Consumer&& consumer)
{
    std::for_each(
        std::execution::par,
        population_.begin(),
        population_.end(),
        consumer);
}

constexpr std::size_t Particles::VectorSize() const noexcept
{
    return vectorSize_;
}

std::size_t Particles::PopulationSize() const noexcept
{
    return population_.size();
}

template<typename Compare>
Particles::Individual Particles::FindBest(Compare&& compare)
{
    auto it = std::min_element(
        population_.begin(),
        population_.end(),
        [&](auto&& a, auto&& b)
        {
            return compare(a.bestFitness, b.bestFitness);
        });
    return *it;
}

template<typename EnvT, typename ControlPolicy>
class BasicPSO: private ControlPolicy
{
public:
    struct Result
    {
        std::vector<double> position;
        double fitness;
    };

    explicit BasicPSO(
        EnvT const& env,
        std::size_t populationSize,
        std::size_t iterations = 100000,
        PSOParameters parameters = {
            1.0 / (2.0 * std::log(2)),
            1.0 / 2.0 + std::log(2),
            1.0 / 2.0 + std::log(2) });

    void Initialise() noexcept;

    void Step();

    bool Terminate() noexcept;

    Result Complete() const noexcept;

private:
    Particles particles_;

    PSOParameters params_;

    double globalBestFitness_ = ControlPolicy::StartingFitness;

    std::vector<double> globalBestPosition_;

    std::size_t iteration_{};

    std::size_t const MaxIteration_;
};

template<typename EnvT, typename ControlPolicy>
BasicPSO<EnvT, ControlPolicy>::BasicPSO(
    EnvT const& env,
    std::size_t populationSize,
    std::size_t iterations,
    PSOParameters parameters)
    : ControlPolicy{ env }
    , particles_{ populationSize, ControlPolicy::Dimension() }
    , params_{ parameters }
    , MaxIteration_{ iterations }
{
}

template<typename EnvT, typename ControlPolicy>
void BasicPSO<EnvT, ControlPolicy>::Initialise() noexcept
{
    globalBestFitness_ = ControlPolicy::StartingFitness;
    iteration_ = 0;
    ControlPolicy::Init(particles_);
}

template<typename EnvT, typename ControlPolicy>
void BasicPSO<EnvT, ControlPolicy>::Step()
{
    auto globalBest = particles_.FindBest(ControlPolicy::Compare);
    auto const Count = particles_.VectorSize();
    if (ControlPolicy::Compare(globalBest.bestFitness, globalBestFitness_))
    {
        // Uncomment this line if you dare to look at the best value per
        // iteration!
        // The output is a bit mangled due to the nested PSO algorithms
        // running.
        //std::cout << ">>> " << iteration_
        //    << ": " << globalBest.fitness;
        //
        //std::cout << " [" << *globalBest.position;
        //std::for_each(
        //    globalBest.position + 1,
        //    globalBest.position + particles_.VectorSize(),
        //    [](auto x){std::cout << ' ' << x;});
        //std::cout << "]\n";

        globalBestFitness_ = globalBest.bestFitness;
        globalBestPosition_.assign(
            globalBest.bestPosition,
            globalBest.bestPosition + Count);
    }

    ControlPolicy::Update(particles_, globalBestPosition_.data(), params_);

    particles_.ForAll([&](auto&& p)
    {
        // ControlPolicy::Evaluate issues a compiler bug on GCC
        p.fitness = this->Evaluate(p);
        if (ControlPolicy::Compare(p.fitness, p.bestFitness))
        {
            p.bestFitness = p.fitness;
            std::copy(p.position, p.position + Count, p.bestPosition);
        }
    });
}


template<typename EnvT, typename ControlPolicy>
bool BasicPSO<EnvT, ControlPolicy>::Terminate() noexcept
{
    return MaxIteration_ < ++iteration_;
}

template<typename EnvT, typename ControlPolicy>
typename BasicPSO<EnvT, ControlPolicy>::Result
BasicPSO<EnvT, ControlPolicy>::Complete()
    const noexcept
{
    return { globalBestPosition_, globalBestFitness_ };
}

template<
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
    PSOParameters const& params)
    noexcept
{
    std::uniform_real_distribution<> d{ 0.0, 1.0 };
    for(; firstX != lastX; ++firstX, ++pBestIt, ++gBestIt, ++vIt, ++nextVIt)
        *nextVIt = params.inertia * *vIt
            + params.cognitiveAttraction * d(rng) * (*gBestIt - *firstX)
            + params.socialAttraction * d(rng) * (*pBestIt - *firstX);
}

template<
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
    PSOParameters const& params)
{
    NextVelocity(
        firstX,
        lastX,
        pBestIt,
        gBestIt,
        vIt,
        vIt,
        rng,
        params);

    std::transform(
        firstX,
        lastX,
        vIt,
        nextPositionIt,
        std::plus<double>{});
}

#endif // !CS3910__PSO_H_
