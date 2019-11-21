#ifndef CS3910__GP_H_
#define CS3910__GP_H_

#include <cassert>
#include <cstdint>
#include <cstring>
#include <ostream>
#include <vector>
#include <random>

namespace internal
{
    enum OpCode: std::uint64_t
    {
        LoadConst,
        LoadArg,
        Add,
        Mul
    };

    template<typename ForwardIt>
    ForwardIt EndOfExpr(ForwardIt first, ForwardIt last)
    {
        assert(first != last && "Cannot find the end of an empty Expr");
        switch (*(first++))
        {
        case OpCode::LoadConst:
        case OpCode::LoadArg:
            return ++first;
        case OpCode::Add:
        case OpCode::Mul:
            auto i = EndOfExpr(first, last);
            i = EndOfExpr(i, last);
            return i;
        }

        // Unreachable!!
        return last;
    }

    template<typename ForwardIt, typename RandomIt>
    double EvalExpr(ForwardIt first, ForwardIt last, RandomIt argIt)
    {
        assert(first != last && "Cannot evaluate an empty Expr");
        switch (*(first++))
        {
        case OpCode::LoadConst:
            {
                double temp;
                std::memcpy(&temp, &*first, 8);
                return temp;
            }
        case OpCode::LoadArg:
            return argIt[*first];
        case OpCode::Add:
            {
                auto i = EndOfExpr(first, last);
                return EvalExpr(first, i, argIt) + EvalExpr(i, last, argIt);
            }
        case OpCode::Mul:
            {
                auto i = EndOfExpr(first, last);
                return EvalExpr(first, i, argIt) * EvalExpr(i, last, argIt);
            }
        }

        // Unreachable!!
        return 0.0;
    }

    template<typename ForwardIt>
    std::ostream& PrintExpr(
        ForwardIt first,
        ForwardIt last,
        std::ostream& outs)
    {
        auto const Op = *(first++);
        switch (Op)
        {
        case OpCode::LoadConst:
            {
                double temp;
                std::memcpy(&temp, &*first, 8);
                outs << temp;
            }
            break;
        case OpCode::LoadArg:
            outs << 'x' << *first;
            break;
        case OpCode::Add:
            {
                auto i = EndOfExpr(first, last);
                PrintExpr(first, i, outs);
                outs << " + ";
                PrintExpr(i, last, outs);
            }
            break;
        case OpCode::Mul:
            {
                auto i = EndOfExpr(first, last);
                if(*first == internal::OpCode::Add)
                    PrintExpr(first, i, outs << '(') << ')';
                else 
                    PrintExpr(first, i, outs);
                outs << " * ";
                if(*i == internal::OpCode::Add)
                    PrintExpr(i, last, outs << '(') << ')';
                else 
                    PrintExpr(i, last, outs);
            }
            break;
        }

        return outs;
    }

    template<typename OutputIt, typename RngT>
    OutputIt RandomExpr(OutputIt outIt, RngT& rng, std::size_t argCount)
    {
        std::uniform_real_distribution<double> d{0.0, 1.0};
        auto const X = d(rng);
        if(X < 0.30)
        {
            *outIt = OpCode::LoadConst;
            ++outIt;
            auto const Val = std::uniform_real_distribution<double>{-100, 100}(rng);
            std::size_t temp;
            std::memcpy(&temp, &Val, 8);
            *outIt = temp;
            return ++outIt;
        }
        else if(X < 0.60)
        {
            *outIt = OpCode::LoadArg;
            ++outIt;
            auto const Val = std::uniform_int_distribution<std::size_t>{0, argCount - 1}(rng);
            *outIt = Val;
            return ++outIt;
        }
        else if(X < 0.80)
        {
            *outIt = internal::OpCode::Add;
            ++outIt;
            outIt = RandomExpr(outIt, rng, argCount);
            return RandomExpr(outIt, rng, argCount);
        }
        else
        {
            *outIt = internal::OpCode::Mul;
            ++outIt;
            outIt = RandomExpr(outIt, rng, argCount);
            return RandomExpr(outIt, rng, argCount);
        }
    }

    template<typename ForwardIt>
    std::size_t CountExpr(ForwardIt first, ForwardIt last)
    {
        switch (*(first++))
        {
        case OpCode::LoadConst:
        case OpCode::LoadArg:
            return 1;
        case OpCode::Add:
        case OpCode::Mul:
            auto i = EndOfExpr(first, last);
            return CountExpr(first, i) + CountExpr(i, last) + 1;
        }

        return 0;
    }

    template<typename ForwardIt>
    ForwardIt FindExpr(ForwardIt first, ForwardIt last, std::size_t id)
    {
        if(id-- == 0)
            return first;
        
        switch (*(first++))
        {
        case OpCode::LoadConst:
        case OpCode::LoadArg:
            return last;
        case OpCode::Add:
        case OpCode::Mul:
            auto i = EndOfExpr(first, last);
            auto const Count = CountExpr(first, i);
            return id < Count
                ? FindExpr(first, i, id)
                : FindExpr(i, last, id - Count);
        }

        return last;
    }
}

class Expression final
{
    friend Expression Add(Expression const& lhs, Expression const& rhs);
    friend Expression Add(Expression&& lhs, Expression&& rhs);
    friend Expression Mul(Expression const& lhs, Expression const& rhs);
    friend Expression Mul(Expression&& lhs, Expression&& rhs);
public:
    explicit Expression(double constVal);

    template<typename RngT>
    explicit Expression(RngT& rng, std::size_t argCount);

    template<typename RandomIt>
    double Eval(RandomIt argIt) const;

    std::ostream& Print(std::ostream& outs) const;

    std::size_t Count() const noexcept;

    Expression SubExpr(std::size_t id);

    bool Replace(std::size_t id, Expression const& expr);

private:
    std::vector<std::uint64_t> expr_{};

    template<typename ForwardIt>
    explicit Expression(ForwardIt first, ForwardIt last)
        : expr_{first, last}
    {
    }

    explicit Expression(
        internal::OpCode op,
        Expression const& lhs,
        Expression const& rhs)
    {
        expr_.reserve(lhs.expr_.size() + rhs.expr_.size() + 1);
        expr_.push_back(op);
        std::copy(
            lhs.expr_.begin(),
            lhs.expr_.end(),
            std::back_inserter(expr_));
        std::copy(
            rhs.expr_.begin(),
            rhs.expr_.end(),
            std::back_inserter(expr_));
    }

    explicit Expression(
        internal::OpCode op,
        Expression&& lhs,
        Expression&& rhs)
    {
        expr_.reserve(lhs.expr_.size() + rhs.expr_.size() + 1);
        expr_.push_back(op);
        std::copy(
            lhs.expr_.begin(),
            lhs.expr_.end(),
            std::back_inserter(expr_));
        std::copy(
            rhs.expr_.begin(),
            rhs.expr_.end(),
            std::back_inserter(expr_));
    }
};

Expression::Expression(double constVal)
{
    expr_.push_back(internal::OpCode::LoadConst);
    std::uint64_t temp;
    std::memcpy(&temp, &constVal, 8);
    expr_.push_back(temp);
}

template<typename RngT>
Expression::Expression(RngT& rng, std::size_t argCount)
{
    internal::RandomExpr(std::back_inserter(expr_), rng, argCount);
}

template<typename RandomIt>
double Expression::Eval(RandomIt argIt) const
{
    return internal::EvalExpr(expr_.begin(), expr_.end(), argIt);
}

Expression Expression::SubExpr(std::size_t id)
{
    auto i = internal::FindExpr(expr_.begin(), expr_.end(), id);
    return Expression{
        i,
        i != expr_.end()
            ? internal::EndOfExpr(i, expr_.end())
            : expr_.end()};
}

std::size_t Expression::Count() const noexcept
{
    return internal::CountExpr(expr_.begin(), expr_.end());
}

std::ostream& Expression::Print(std::ostream& outs) const
{
    return internal::PrintExpr(expr_.begin(), expr_.end(), outs);
}

Expression Add(Expression const& lhs, Expression const& rhs)
{
    return Expression(internal::OpCode::Add, lhs, rhs);
}

Expression Add(Expression&& lhs, Expression&& rhs)
{
    return Expression(internal::OpCode::Add, lhs, rhs);
}

Expression Mul(Expression const& lhs, Expression const& rhs)
{
    return Expression(internal::OpCode::Mul, lhs, rhs);
}

Expression Mul(Expression&& lhs, Expression&& rhs)
{
    return Expression(internal::OpCode::Mul, lhs, rhs);
}

std::ostream& operator<<(std::ostream& outs, Expression const& expr)
{
    return expr.Print(outs);
}

#endif // !CS3910__GP_H_