

enum Op: std::uint64_t
{
    Const,
    Add,
};

std::uint64_t[] tree[]{
   Op::Const,
   100,
   Op::Const,
   200,
   Op::Add};

template<typename ContextT, typename RandomIt>
void Process(RandomIt first, RandomIt last, ContextT&& ctx)
{
    assert(first != last && "");

    switch(*first)
    {
    case Operation::Const: break;
        internal::Add(first, last, ctx);
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

    template<typename ContextT, typename RandomIt>
    RandomIt Const(RandomIt first, RandomIt last, ContextT&& ctx)
    {
        assert(*first == Op::Const);
        ++first;
        double temp;
        std::memcpy(&temp, &*first, sizeof(double));
        ctx.Push(temp);
    }
}