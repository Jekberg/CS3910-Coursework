#ifndef CS3910__PALLETS_H_
#define CS3910__PALLETS_H_

#include <vector>

class HistoricalPalletData
{
public:
    explicit HistoricalPalletData(char const* fileName);
private:
    std::vector<double> demand_;
    std::vector<double> dataPoints_;
    std::size_t dataPointCount_;

    template<
        typename OutputDemandIt,
        typename OutputDataIt>
    static void ReadHistoricalDataFrom(
        char const* fileName,
        OutputDemandIt outDemandIt,
        OutputDataIt outDataIt)
    {
        
    }
};

#endif // !CS3910__PALLETS_H_