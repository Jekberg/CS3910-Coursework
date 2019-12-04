#ifndef CS3910__CORE_H_
#define CS3910__CORE_H_

#include "Pallets.h"
#include <exception>
#include <ostream>

struct Dataset
{
    PalletData trainingData;
    PalletData testingData;
};

class DataSizeMismatch final: public std::exception
{
public:
    char const* what() const noexcept final
    {
        return "The training data and the testing data are incompatible";
    }
};

Dataset ReadPalletData(int argc, char const** argv, std::ostream& outs)
{
    auto trainDataFile = "sample/cwk_train.csv";
    auto testDataFile = "sample/cwk_test.csv";

    if (argc < 3)
    {
        outs << "Too few arguments, 2 arguments are required: "
            << "Expected a file containing training data "
            << "and a file containing test data\n"
            << "Using the default files "
            << trainDataFile << " and " << testDataFile
            << '\n';
    }
    else
    {
        trainDataFile = argv[1];
        testDataFile = argv[2];
    }

    Dataset data{PalletData{trainDataFile}, PalletData{testDataFile}};
    if(data.trainingData.DataCount() != data.testingData.DataCount())
        throw DataSizeMismatch{};
    else
        return data;
}
#endif // !CS3910__CORE_H_
