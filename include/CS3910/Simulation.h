#ifndef CS3910__SIMULATION_H_
#define CS3910__SIMULATION_H_

template<typename SimulationPolicy>
auto Simulate(SimulationPolicy&& policy)
    noexcept(noexcept(policy.Initialise())
        && noexcept(policy.Terminate())
        && noexcept(policy.Step())
        && noexcept(policy.Complete()))
{
    policy.Initialise();
    while(!policy.Terminate())
        policy.Step();
    return policy.Complete();
};

#endif // !CS3910__SIMULATION_H_