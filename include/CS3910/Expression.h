

enum Operation: std::uint64_t
{
    Const,
    Add,
};

template<typename ContextT, typename RandomIt>
void Process(RandomIt first, RandomIt last, ContextT&& ctx)
{
    assert(first != last && "");

    switch(*first)
    {
    case Operation::Cost: break;
    case Operation::Add: break;
        internal::Add(first, last, ctx);
    }
}

namespace internal
{
    template<typename ContextT, typename RandomIt>
    RandomIt Add(RandomIt first, RandomIt last, ContextT&& ctx)
    {
        
    }
}