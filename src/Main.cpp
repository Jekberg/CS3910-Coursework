#include "CS3910/Core.h"
#include "CS3910/PSO.h"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <iostream>
#include <string>
#include <vector>

//template<typename ForwardItA>
//auto Error(ForwardItA )

// 35 pallets per trucks
// Trucks = pallets / 35

template<
    typename OutputDemandIt,
    typename OutputDataIt>
bool Read(
    char const* fileName,
    std::size_t& outMaxColumnCount,
    OutputDemandIt outDemandIt,
    OutputDataIt outDataIt)
{
    assert(fileName != nullptr && "The fileName must not be the nullptr");
    std::ifstream file{};

    file.open(fileName);
    if(!file.is_open())
        return false;

    std::string line;
    outMaxColumnCount = 0;
    for(;file >> line; ++outDemandIt, ++outDataIt)
    {
        auto i{std::find(line.begin(), line.end(), ',')};

        if(i == line.end())
            return false;

        *outDemandIt = std::stod(std::string{line.begin(), i});

        std::vector<double> temp{};
        for(++i; i < line.end(); ++i)
        {
            auto it = std::find(i, line.end(), ',');
            temp.push_back(std::stod(std::string{i, it}));
            i = it;
        }

        if(outMaxColumnCount < temp.size())
            outMaxColumnCount = temp.size();
        *outDataIt = std::move(temp);
    }

    return true;
}

struct CandidateSolution
{
    std::unique_ptr<double[]> position;
    std::unique_ptr<double[]> velocity;
    std::unique_ptr<double[]> bestPosition;
    double cost;
    double bestCost;
};

template<
    typename OutputIt,
    typename RngT>
void InitialisePopulation(
    OutputIt first,
    OutputIt last,
    RngT& rng,
    std::size_t count)
{
    std::generate(
        first,
        last,
        [&]()
        {
            auto position {std::make_unique<double[]>(count)};
            InitialiseRandomWeights(position.get(), position.get() + count, rng);

            auto bestPosition {std::make_unique<double[]>(count)};
            std::copy(position.get(), position.get() + count, bestPosition.get());

            auto velocity {std::make_unique<double[]>(count)};
            std::fill(velocity.get(), velocity.get() + count, 0);

            return CandidateSolution{
                std::move(position),
                std::move(velocity),
                std::move(bestPosition),
                std::numeric_limits<double>::infinity(),
                std::numeric_limits<double>::infinity()};
        });
}

template<
    typename ForwardIt,
    typename ForwardDataIt,
    typename ForwardDemandIt>
void EvaluatePopulation(
    ForwardIt first,
    ForwardIt last,
    ForwardDataIt firstData,
    ForwardDataIt lastData,
    ForwardDemandIt demandIt,
    std::size_t count)
{
    std::for_each(
        first,
        last,
        [=](auto&& p)
        {
            p.cost = EstemateCost(
                firstData,
                lastData,
                demandIt,
                p.position.get());

            if(p.cost < p.bestCost)
            {
                p.bestCost = p.cost;
                std::copy(
                    p.position.get(),
                    p.position.get() + count,
                    p.vestPosition.get());
            }
        });
}

class ParticleSwarmPopulation
{
public:
    struct Candidate
    {
        double* position;
        double* velocity;
        double* bestPosition;
    };

    explicit ParticleSwarmPopulation(
        std::size_t populationSize,
        std::size_t count);

    template<typename Consumer>
    void ForEach(Consumer&& consumer);

private:
    std::vector<double> positions_;
    std::vector<double> velocities_;
    std::vector<double> bestPositions_;
    std::vector<Candidate> population_;
}



int main()
{
    std::vector<double> demand{};
    std::vector<std::vector<double>> data{};
    std::size_t count;

    Read(
        "sample/cwk_train.csv",
        count,
        std::back_inserter(demand),
        std::back_inserter(data));

    std::vector<double> population(10);
    std::random_device rng{};
    
    InitialisePopulation(population.begin(), population.end(), rng, count);

    while(true)
    {
        EvaluatePopulation(
            population.begin(),
            population.end(),
            data.begin(),
            data.end(),
            demand.begin(),
            count);
    }
}

ParticleSwarmPopulation::ParticleSwarmPopulation(
    std::size_t populationSize,
    std::size_t count)
    : positions_(populationSize * count)
    , velocities_(populationSize * count)
    , bestPositions_(populationSize * count)
    , population_(populationSize)
{
}

template<typename Consumer>
void ParticleSwarmPopulation::ForEach(Consumer&& consumer)
{
    std::for_each(
        population.begin(),
        population.end(),
        consumer);
}