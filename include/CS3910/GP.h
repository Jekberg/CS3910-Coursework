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
        Sub,
        Mul,
        Div
    };

    int PrecOf(std::uint64_t op)
    {
        switch(op)
        {
        case OpCode::LoadConst:
        case OpCode::LoadArg:
            return std::numeric_limits<int>::max();
        case OpCode::Add:
        case OpCode::Sub:
        case OpCode::Mul:
        case OpCode::Div:
            return static_cast<int>(op);
        }

        return -1;
    }

    std::ostream& PrintInfix(std::uint64_t op, std::ostream& outs)
    {
        switch (op)
        {
        case OpCode::Add:
            return outs << " + ";
        case OpCode::Sub:
            return outs << " - ";
        case OpCode::Mul:
            return outs << " * ";
        case OpCode::Div:
            return outs << " / ";
        }

        return outs;
    }

    // Find the exclusive end of a prefix expressions
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
        case OpCode::Sub:
        case OpCode::Mul:
        case OpCode::Div:
            auto i = EndOfExpr(first, last);
            i = EndOfExpr(i, last);
            return i;
        }

        // Unreachable!!
        return last;
    }

    // Evaluate a prefix expression
    template<typename ForwardIt, typename RandomIt>
    std::pair<double, ForwardIt> EvalExpr(
        ForwardIt first,
        ForwardIt last,
        RandomIt argIt)
    {
        assert(first != last && "Cannot evaluate an empty Expr");
        switch (*first)
        {
        case OpCode::LoadConst:
            {
                double temp;
                std::memcpy(&temp, &*std::next(first), 8);
                return {temp, std::next(first, 2)};
            }
        case OpCode::LoadArg:
            return {argIt[*std::next(first)], std::next(first, 2)};
        case OpCode::Add:
            {
                auto [lhsVal, lhsEnd] = EvalExpr(std::next(first), last, argIt);
                auto [rhsVal, rhsEnd] = EvalExpr(lhsEnd, last, argIt);
                return {lhsVal + rhsVal, rhsEnd};
            }
        case OpCode::Sub:
            {
                auto [lhsVal, lhsEnd] = EvalExpr(std::next(first), last, argIt);
                auto [rhsVal, rhsEnd] = EvalExpr(lhsEnd, last, argIt);
                return {lhsVal - rhsVal, rhsEnd};
            }
        case OpCode::Mul:
            {
                auto [lhsVal, lhsEnd] = EvalExpr(std::next(first), last, argIt);
                auto [rhsVal, rhsEnd] = EvalExpr(lhsEnd, last, argIt);
                return {lhsVal * rhsVal, rhsEnd};
            }
        case OpCode::Div:
            {
                auto [lhsVal, lhsEnd] = EvalExpr(std::next(first), last, argIt);
                auto [rhsVal, rhsEnd] = EvalExpr(lhsEnd, last, argIt);
                if(rhsVal == 0.0)
                    // Protected division...
                    return {std::numeric_limits<double>::infinity(), rhsEnd};
                else
                    return { lhsVal / rhsVal, rhsEnd };
            }
        }

        // Unreachable!!
        return {0.0, last};
    }

    // Print a prefix expression as infix
    template<typename ForwardIt>
    ForwardIt PrintExpr(
        ForwardIt first,
        ForwardIt last,
        std::ostream& outs)
    {
        assert(first != last && "Cannot print empty expression");
        switch (*first)
        {
        case OpCode::LoadConst:
            {
                double temp;
                std::memcpy(&temp, &*std::next(first), 8);
                outs << temp;
                return std::next(first, 2);
            }
        case OpCode::LoadArg:
            outs << 'x' << *std::next(first);
            return std::next(first, 2);
            
        case OpCode::Add:
        case OpCode::Sub:
        case OpCode::Mul:
        case OpCode::Div:
            {
                auto const Op = *first;
                ++first;
                if(PrecOf(*first) < PrecOf(Op))
                    outs << '(';
                auto lhsEnd = PrintExpr(first, last, outs);
                if (PrecOf(*first) < PrecOf(Op))
                    outs << ')';
                
                PrintInfix(Op, outs);
                
                if(PrecOf(*lhsEnd) < PrecOf(Op))
                    outs << '(';
                auto rhsEnd = PrintExpr(lhsEnd, last, outs);
                if (PrecOf(*lhsEnd) < PrecOf(Op))
                    outs << ')';
                return rhsEnd;
            }
        }

        return last;
    }

    // Count the total number of terminals and non-terminals in a prefix
    // expressions
    template<typename ForwardIt>
    std::size_t CountExpr(ForwardIt first, ForwardIt last)
    {
        std::size_t count{};
        while(first != last) {
            ++count;
            switch (*first)
            {
            case OpCode::LoadConst:
            case OpCode::LoadArg:
                first += 2;
                break;
            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul:
            case OpCode::Div:
                ++first;
                break;
            }
        }

        return count;
    }

    // Find a terminal or non-terminal in the prefix expression by traversing
    // the tree in deptht-first order. The id of a node corresponds to it's
    // order of visitation.
    template<typename ForwardIt>
    ForwardIt FindExpr(ForwardIt first, ForwardIt last, std::size_t id)
    {
        if(id == 0)
            return first;
        
        while (first != last)
        {
            if(id-- == 0)
                return first;
            switch (*first)
            {
            case OpCode::LoadConst:
            case OpCode::LoadArg:
                first += 2;
                break;
            case OpCode::Add:
            case OpCode::Sub:
            case OpCode::Mul:
            case OpCode::Div:
                ++first;
                break;
            }
        }

        return first;
    }
}


// Expr encapsulate the prefix notations and provide some basic manipulation.
class Expr final
{
    // Functions for composing expressions.
    friend Expr Const(double constVal);
    friend Expr Arg(std::uint64_t argId);
    friend Expr operator+(Expr const& lhs, Expr const& rhs);
    friend Expr operator-(Expr const& lhs, Expr const& rhs);
    friend Expr operator*(Expr const& lhs, Expr const& rhs);
    friend Expr operator/(Expr const& lhs, Expr const& rhs);
public:
    // Replace a node in the Expr with another Expr.
    bool Replace(std::size_t id, Expr const& expr);

    // Evaluate the expression using a range of arguments.
    template<typename RandomIt>
    double Eval(RandomIt argIt) const;

    // Print the expression.
    std::ostream& Print(std::ostream& outs) const;

    // Count the number of elements in the expression.
    std::size_t Count() const noexcept;

    // Extract a Sub Expr from this Expr starting from a given node.
    Expr SubExpr(std::size_t id) const;


private:
    std::vector<std::uint64_t> expr_{};

    template<typename ForwardIt>
    explicit Expr(ForwardIt first, ForwardIt last)
        : expr_{first, last}
    {
    }

    explicit Expr(double constVal)
    {
        expr_.push_back(internal::OpCode::LoadConst);
        std::uint64_t temp;
        std::memcpy(&temp, &constVal, 8);
        expr_.push_back(temp);
    }

    explicit Expr(std::uint64_t argId)
    {
        expr_.push_back(internal::OpCode::LoadArg);
        expr_.push_back(argId);
    }

    explicit Expr(
        internal::OpCode op,
        Expr const& lhs,
        Expr const& rhs)
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

// Members of Expr
bool Expr::Replace(std::size_t id, Expr const& expr)
{
    auto i = internal::FindExpr(expr_.begin(), expr_.end(), id);
    if(i == expr_.begin())
        return false;
    i = expr_.erase(i, internal::EndOfExpr(i, expr_.end()));
    std::copy(
        expr.expr_.cbegin(),
        expr.expr_.cend(),
        std::inserter(expr_, i));
    return true;
}

template<typename RandomIt>
double Expr::Eval(RandomIt argIt) const
{
    auto [val, it] = internal::EvalExpr(expr_.begin(), expr_.end(), argIt);
    if(it != expr_.end())
        return 0.0;
    return val;
}

Expr Expr::SubExpr(std::size_t id) const
{
    auto i = internal::FindExpr(expr_.begin(), expr_.end(), id);
    return Expr{
        i,
        i != expr_.end()
            ? internal::EndOfExpr(i, expr_.end())
            : expr_.end()};
}

std::size_t Expr::Count() const noexcept
{
    return internal::CountExpr(expr_.begin(), expr_.end());
}

std::ostream& Expr::Print(std::ostream& outs) const
{
    internal::PrintExpr(expr_.begin(), expr_.end(), outs);
    return outs;
}
// Friends of Expr
Expr Const(double constVal)
{
    return Expr{constVal};
}

Expr Arg(std::uint64_t argId)
{
    return Expr{argId};
}

Expr operator+(Expr const& lhs, Expr const& rhs)
{
    return Expr(internal::OpCode::Add, lhs, rhs);
}

Expr operator-(Expr const& lhs, Expr const& rhs)
{
    return Expr(internal::OpCode::Sub, lhs, rhs);
}

Expr operator*(Expr const& lhs, Expr const& rhs)
{
    return Expr(internal::OpCode::Mul, lhs, rhs);
}

Expr operator/(Expr const& lhs, Expr const& rhs)
{
    return Expr(internal::OpCode::Div, lhs, rhs);
}

std::ostream& operator<<(std::ostream& outs, Expr const& expr)
{
    return expr.Print(outs);
}

// Utillities
template<typename ForwardIt, typename RngT, typename F>
ForwardIt Roulette(ForwardIt first, ForwardIt last, RngT& rng, F&& f)
{
    auto const Total = std::accumulate(
        first,
        last,
        double{},
        [&](auto&& acc, auto&& x) { return acc + f(x); });

    auto r = std::uniform_real_distribution<>{ 0.0, Total }(rng);
    for (auto i = first; i != last; ++i)
        if (Total <= (r += f(*i)))
            return i;
    return last;
}

template<typename RandomIt, typename RngT>
RandomIt SampleGroup(RandomIt first, RandomIt last, std::size_t k, RngT& rng)
{
    // Move k elements selected at random to the front of the range.
    assert(first != last && "Cannot create a sample group of an empty range");
    assert(k != 0 && "Cannot create sample group of size 0");

    if (auto count = static_cast<std::size_t>(std::distance(first, last));
        count < k)
        k = count;

    for (auto i = first; i != first + k; ++i)
    {
        auto c = std::uniform_int_distribution<std::size_t>{
            0,
            static_cast<std::size_t>(std::distance(i, last) - 1) }(rng);
        if (i != i + c)
            std::swap(*i, i[c]);
    }

    // The end of the sample group
    return first + k;
}

template<typename RngT>
std::pair<Expr, Expr> SubtreeCrossover(
    Expr const& a,
    Expr const& b,
    RngT& rng)
{
    using Distribution = std::uniform_int_distribution<std::size_t>;
    auto const IdA = Distribution{1, a.Count() - 1}(rng);
    auto subExprA = a.SubExpr(IdA);

    auto idB = Distribution{ 1, b.Count() - 1 }(rng);
    auto subExprB = b.SubExpr(idB);

    // If one of the sub-trees is much larger than the other, then keep
    // selecting sub-trees from one of the sub-trees so that the sizes may
    // match.
    //while(subExprA.Count() + 1 < subExprB.Count())
    //{
    //    auto const SubIdB = Distribution{ 1, subExprB.Count() - 1 }(rng);
    //    subExprB = subExprB.SubExpr(SubIdB);
    //    idB += SubIdB;
    //}

    auto childA{ a };
    childA.Replace(IdA, subExprB);
    auto childB{ b };
    childB.Replace(idB, a.SubExpr(IdA));
    return {childA, childB};
}

#endif // !CS3910__GP_H_