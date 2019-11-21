#ifndef CS3910__PALLETS_H_
#define CS3910__PALLETS_H_

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

class PalletData
{
public:
    explicit PalletData(char const* fileName);

    inline std::size_t RowCount() const noexcept;

    inline std::size_t DataCount() const noexcept;

    inline double const* BeginDemand() const noexcept;

    inline double const* EndDemand() const noexcept;

    inline double const* BeginData() const noexcept;

    inline double const* EndData() const noexcept;

    inline double const* BeginRowData(std::size_t row) const noexcept;

    inline double const* EndRowData(std::size_t row) const noexcept;

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
                temp.push_back(std::stod(std::string{ i, it }));
                if(it == line.end())
                    break;
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

std::size_t PalletData::RowCount() const noexcept
{
    return demand_.size();
}

std::size_t PalletData::DataCount() const noexcept
{
    return dataPointCount_;
}

double const* PalletData::BeginDemand() const noexcept
{
    return demand_.data();
}

double const* PalletData::EndDemand() const noexcept
{
    return demand_.data() + demand_.size();
}

double const* PalletData::BeginData() const noexcept
{
    return dataPoints_.data();
}

double const* PalletData::EndData() const noexcept
{
    return dataPoints_.data() + dataPoints_.size();
}

double const* PalletData::BeginRowData(std::size_t row)
    const noexcept
{
    assert(row < demand_.size() && "Out of bounds row");
    return dataPoints_.data() + row * dataPointCount_;
}

double const* PalletData::EndRowData(std::size_t row)
    const noexcept
{
    assert(row < demand_.size() && "Out of bounds row");
    return dataPoints_.data() + row * dataPointCount_ + dataPointCount_;
}

#endif // !CS3910__PALLETS_H_