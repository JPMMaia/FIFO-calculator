module;

#include <algorithm>
#include <cassert>
#include <chrono>
#include <istream>
#include <memory_resource>
#include <ostream>
#include <span>
#include <string>
#include <vector>

export module fifo_calculator;

namespace fifo_calculator
{
    export struct Entry
    {
        std::chrono::utc_clock::time_point time;
        double volume;
        double price;
        double fee;

        friend auto operator<=>(Entry const&, Entry const&) = default;
    };

    std::ostream& operator<<(std::ostream& output_stream, Entry const& entry)
    {
        output_stream << "{";
        {
            output_stream << "\"time\":" << entry.time << ",";
            output_stream << "\"volume\":" << entry.volume << ",";
            output_stream << "\"price\":" << entry.price << ",";
            output_stream << "\"fee\":" << entry.fee;
        }
        output_stream << "}";

        return output_stream;
    }

    export struct Result
    {
        std::chrono::utc_clock::time_point buy_date;
        std::chrono::utc_clock::time_point sell_date;
        double volume;
        double buy_price;
        double buy_fee;
        double sell_price;
        double sell_fee;

        friend auto operator<=>(Result const&, Result const&) = default;
    };

    export std::ostream& operator<<(std::ostream& output_stream, Result const& result)
    {
        output_stream << "{";
        {
            output_stream << "\"buy_date\":" << result.buy_date << ",";
            output_stream << "\"sell_date\":" << result.sell_date << ",";
            output_stream << "\"volume\":" << result.volume << ",";
            output_stream << "\"buy_price\":" << result.buy_price << ",";
            output_stream << "\"buy_fee\":" << result.buy_fee << ",";
            output_stream << "\"sell_price\":" << result.sell_price << ",";
            output_stream << "\"sell_fee\":" << result.sell_fee;
        }
        output_stream << "}";

        return output_stream;
    }

    export std::pmr::vector<Result> calculate_fifo(
        std::span<Entry const> const buy_entries,
        std::span<Entry const> const sell_entries,
        std::pmr::polymorphic_allocator<> const& output_allocator,
        std::pmr::polymorphic_allocator<> const& temporaries_allocator
    )
    {
        if (buy_entries.empty() || sell_entries.empty())
        {
            return std::pmr::vector<Result>{ output_allocator };
        }

        std::size_t current_buy_entry_index = 0;
        double remaining_buy_volume = buy_entries[0].volume;
        double remaining_buy_fee = buy_entries[0].fee;
        std::size_t current_sell_entry_index = 0;
        double remaining_sell_volume = sell_entries[0].volume;
        double remaining_sell_fee = sell_entries[0].fee;

        std::pmr::vector<Result> results{ output_allocator };
        results.reserve(std::max(buy_entries.size(), sell_entries.size()));

        while (true)
        {
            if (remaining_buy_volume == 0.0)
            {
                assert(remaining_buy_fee == 0.0);

                ++current_buy_entry_index;

                if (current_buy_entry_index >= buy_entries.size())
                {
                    break;
                }

                remaining_buy_volume = buy_entries[current_buy_entry_index].volume;
                remaining_buy_fee = buy_entries[current_buy_entry_index].fee;
            }

            if (remaining_sell_volume == 0)
            {
                assert(remaining_sell_fee == 0.0);

                ++current_sell_entry_index;

                if (current_sell_entry_index >= sell_entries.size())
                {
                    break;
                }

                remaining_sell_volume = sell_entries[current_sell_entry_index].volume;
                remaining_sell_fee = sell_entries[current_sell_entry_index].fee;
            }

            Entry const& buy_entry = buy_entries[current_buy_entry_index];
            Entry const& sell_entry = sell_entries[current_sell_entry_index];

            if (remaining_sell_volume <= remaining_buy_volume)
            {
                Result const result
                {
                    .buy_date = buy_entry.time,
                    .sell_date = sell_entry.time,
                    .volume = remaining_sell_volume,
                    .buy_price = buy_entry.price,
                    .buy_fee = (remaining_sell_volume / remaining_buy_volume) * remaining_buy_fee,
                    .sell_price = sell_entry.price,
                    .sell_fee = remaining_sell_fee,
                };

                remaining_buy_volume -= result.volume;
                remaining_buy_fee -= result.buy_fee;
                remaining_sell_volume = 0.0;
                remaining_sell_fee = 0.0;

                results.push_back(result);
            }
            else
            {
                Result const result
                {
                    .buy_date = buy_entry.time,
                    .sell_date = sell_entry.time,
                    .volume = remaining_buy_volume,
                    .buy_price = buy_entry.price,
                    .buy_fee = remaining_buy_fee,
                    .sell_price = sell_entry.price,
                    .sell_fee = (remaining_buy_volume / remaining_sell_volume) * remaining_sell_fee,
                };

                remaining_sell_volume -= result.volume;
                remaining_sell_fee -= result.sell_fee;
                remaining_buy_volume = 0.0;
                remaining_buy_fee = 0.0;

                results.push_back(result);
            }
        }

        return results;
    }

    export struct File_entries
    {
        std::pmr::vector<Entry> buy_entries;
        std::pmr::vector<Entry> sell_entries;
    };

    export struct Entry_format
    {
        std::size_t time_column_index;
        std::size_t volume_column_index;
        std::size_t price_column_index;
        std::size_t fee_column_index;
        std::size_t buy_or_sell_column_index;
        char separator;
    };

    export File_entries read_file_entries(
        std::istream& input_stream,
        Entry_format const& entry_format,
        std::pmr::polymorphic_allocator<> const& output_allocator,
        std::pmr::polymorphic_allocator<> const& temporaries_allocator
    )
    {
        std::pmr::vector<Entry> buy_entries{ output_allocator };
        std::pmr::vector<Entry> sell_entries{ output_allocator };

        while (input_stream.good())
        {
            std::string line; // TODO use pmr
            std::getline(input_stream, line, '\n');

            Entry current_entry = {};
            std::optional<bool> is_buy = {};

            {
                auto current_character = line.begin();
                std::size_t column_index = 0;

                while (current_character != line.end())
                {
                    auto const next_separator = std::find(current_character, line.end(), entry_format.separator);

                    std::string_view const value = { current_character, next_separator };

                    if (column_index == entry_format.time_column_index)
                    {
                        std::string_view const value_without_quotes = { value.begin() + 1, value.end() - 1 };
                        std::stringstream ss;
                        ss << value_without_quotes;

                        std::chrono::from_stream(ss, "%F%n%T", current_entry.time);
                    }
                    else if (column_index == entry_format.volume_column_index)
                    {
                        current_entry.volume = std::stod(std::string{ value.begin(), value.end() });
                    }
                    else if (column_index == entry_format.price_column_index)
                    {
                        current_entry.price = std::stod(std::string{ value.begin(), value.end() });
                    }
                    else if (column_index == entry_format.fee_column_index)
                    {
                        current_entry.fee = std::stod(std::string{ value.begin(), value.end() });
                    }
                    else if (column_index == entry_format.buy_or_sell_column_index)
                    {
                        if (value == "\"buy\"")
                        {
                            is_buy = true;
                        }
                        else if (value == "\"sell\"")
                        {
                            is_buy = false;
                        }
                        else
                        {
                            throw std::runtime_error{ std::format("Unexpected buy or sell value: '{}'\n", value) };
                        }
                    }

                    if (next_separator == line.end())
                    {
                        if (!is_buy.has_value())
                        {
                            throw std::runtime_error{ "Could not find out if it was a buy or a sell!" };
                        }

                        if (*is_buy)
                        {
                            buy_entries.push_back(current_entry);
                        }
                        else
                        {
                            sell_entries.push_back(current_entry);
                        }

                        break;
                    }

                    ++column_index;
                    current_character = next_separator + 1;
                }
            }
        }

        return
        {
            .buy_entries = std::move(buy_entries),
            .sell_entries = std::move(sell_entries)
        };
    }
}
