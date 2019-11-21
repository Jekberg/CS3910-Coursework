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
ForwardIt FindEndOfExpr(ForwardIt first, ForwardIt last)
{
    // WIP
    switch (*(first++))
    {
    case OpCode::LoadConst:
        return ++first;
    case OpCode::Add:
    case OpCode::Mul:
        auto i = FindEndOfExpr(first, last);
        i = FindEndOfExpr(i, last);
        return i;
    }
}

template<typename ForwardIt>
double EvalExpr(ForwardIt first, ForwardIt last) 
{
    switch (*(first++))
    {
    case OpCode::LoadConst:
        double bits;
        std::memcpy(&bits, &*first, sizeof(double));
        return bits;
    case OpCode::Add:
        {
            auto i = FindEndOfExpr(first, last);
            if (i == last)
                return 0.0;
            return EvalExpr(first, i) + EvalExpr(i, last);
        }
    case OpCode::Mul:
        {
            auto i = FindEndOfExpr(first, last);
            if (i == last)
                return 0.0;
            return EvalExpr(first, i) + EvalExpr(i, last);
        }
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
    code.push_back(Add);
    auto i = std::back_inserter(code);
    i = InsertLoadConst(i, 100);
    i = InsertLoadConst(i, 200);

    std::cout << code.size() << '\n';

    auto i1 = FindEndOfExpr(code.begin(), code.end());
    std::cout << (i1 == code.end()) << '\n';
    std::cout << EvalExpr(code.begin(), code.end());
}