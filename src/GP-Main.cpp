#include "CS3910/Pallets.h"
#include "CS3910/GP.h"
#include <execution>
#include <iostream>

double Estemate(PalletData& data, Expression const& expr);

int main()
{
    PalletData data{"sample/cwk_train.csv"};
    std::random_device rng{};

    double ar[0];
    auto y = Add(Expression {1}, Expression {100});
    std::cout << y.SubExpr(2).Eval(ar) << '\n';

    //for(std::size_t c{}; c < 10000; ++c)
    //{
    //    Expression x{rng, data.DataCount()};
    //    std::cout <<"[" << x.Count() << "] " << Estemate(data, x) << " <-- " << x << '\n';
    //}

}

double Estemate(PalletData& data, Expression const& expr)
{
    std::vector<double> estemates{};
    for(std::size_t i{}; i != data.RowCount(); ++i)
        estemates.push_back(expr.Eval(data.BeginRowData(i)));

    auto const Total = std::transform_reduce(
        std::execution::seq,
        estemates.cbegin(),
        estemates.cend(),
        data.BeginDemand(),
        0.0,
        std::plus<double>{},
        [](auto a, auto b) noexcept {return std::abs(a - b); });

    return Total / data.RowCount();
}