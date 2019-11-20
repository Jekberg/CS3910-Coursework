#include <cstdint>
#include <iostream>
#include <vector>


enum OpCode: std::uint64_t
{
    LoadConst,
    Add,
    Mul
};


template<typename ForwardIt>
ForwardIt Eval(ForwardIt first, ForwardIt last)
{
    // WIP
    switch (*(first++))
    {
    case OpCode::LoadConst:
        context.Terminal(*first);
        return first;
    case OpCode::Add:
        return first;
    }
}

std::vector<std::uint64_t> code{};

template<typename OutputIt>
OutputIt InsertLoadConst(OutputIt outIt, double constant)
{
    *outIt = OpCode::LoadConst;
    ++outIt;

    std::uint64_t bits;
    std::memcpy(&bits, &constant, sizeof(std::uint64_t));
    
    *outIt = bits;
    return ++outIt;
}

int main()
{
    auto i = std::back_inserter(code);
    i = InsertLoadConst(i, 100);

    std::cout << code.size();
}