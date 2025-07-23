#include <fstream>
#include <iostream>
#include <memory_resource>
#include <vector>

import fifo_calculator;

using namespace fifo_calculator;

int main(int const argc, char const* const* const argv)
{
    if (argc < 2)
    {
        return -1;
    }

    char const* const filepath = argv[1];

    Entry_format const entry_format
    {
        .time_column_index = 4,
        .volume_column_index = 10,
        .price_column_index = 7,
        .fee_column_index = 9,
        .buy_or_sell_column_index = 5,
        .separator = ',',
    };

    std::ifstream input_stream{ filepath };

    File_entries const file_entries = read_file_entries(
        input_stream,
        entry_format,
        {},
        {}
    );

    std::pmr::vector<Result> const results = calculate_fifo(
        file_entries.buy_entries,
        file_entries.sell_entries,
        {},
        {}
    );

    std::cout << "{\n    \"transactions\": [\n";
    for (std::size_t index = 0; index < results.size(); ++index)
    {
        Result const& result = results[index];
        std::cout << "        " << result;

        if (index + 1 < results.size())
            std::cout << ',';
    
        std::cout << '\n';
    }
    std::cout << "    ]\n}" << std::endl;

    {
        double total_profit = 0.0;
        for (Result const& result : results)
        {
            total_profit += result.volume * (result.sell_price - result.buy_price);
        }
        std::cout << total_profit << std::endl;
    }

    return 0;
}
