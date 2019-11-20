#ifndef CS3910__PALLETS_H_
#define CS3910__PALLETS_H_

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <vector>

class PalletData
{
public:
    explicit PalletData(char const* fileName);

    double const* BeginDemand() const;
    double const* EndDemand() const;

    double const* BeginRowData(std::size_t row) const;
    double const* EndRowData(std::size_t row) const;

private:
    std::vector<double> demand_{};
    std::vector<double> dataPoints_{};
    std::size_t dataPointCount_{};

    template<
        typename OutputDemandIt,
        typename OutputDataIt>
    static bool ReadHistoricalDataFrom(
        char const* fileName,
        std::size_t& count,
        OutputDemandIt outDemandIt,
        OutputDataIt outDataIt)
    {
        assert(fileName != nullptr && "The fileName must not be the nullptr");
        std::ifstream file{};

        file.open(fileName);
        if(!file.is_open())
            return false;

        std::string line;
        std::vector<std::size_t> data{};
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
                if(it == line.end())
                    break;
                temp.push_back(std::stod(std::string{ i, it }));
                i = it;
            }

            if(!data.empty() && data.back() != temp.size())
                return false;

            std::copy(temp.begin(), temp.end(), outDataIt);
            data.emplace_back(temp.size());
        }

        count = data.back();
        return true;
    }
};

PalletData::PalletData(char const* fileName)
{
    if(!ReadHistoricalDataFrom(
        fileName,
        dataPointCount_,
        std::back_inserter(demand_),
        std::back_inserter(dataPoints_)))
        throw 0;
}

double Estemate(PalletData& data)
{
}

#endif // !CS3910__PALLETS_H_