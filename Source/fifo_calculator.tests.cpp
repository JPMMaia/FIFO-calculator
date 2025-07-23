#include <catch2/catch_all.hpp>

#include <array>
#include <chrono>
#include <memory_resource>
#include <sstream>
#include <vector>

import fifo_calculator;

namespace fifo_calculator::tests {
TEST_CASE("1 buy, 2 sells") {
  using namespace fifo_calculator;
  using namespace std::literals;

  std::array<Entry, 1> const buy_entries{
      Entry{.time = std::chrono::utc_clock::now(),
            .volume = 10.0,
            .price = 2.0,
            .fee = 0.1}};

  std::array<Entry, 2> const sell_entries{
      Entry{.time = std::chrono::utc_clock::now() + 1s,
            .volume = 5.0,
            .price = 1.0,
            .fee = 0.05},
      Entry{
          .time = std::chrono::utc_clock::now() + 3s,
          .volume = 5.0,
          .price = 1.5,
          .fee = 0.08,
      }};

  std::pmr::vector<Result> const results =
      calculate_fifo(buy_entries, sell_entries, {}, {});

  REQUIRE(results.size() == 2);

  {
    Result const expected{
        .buy_date = buy_entries[0].time,
        .sell_date = sell_entries[0].time,
        .volume = 5,
        .buy_price = 2.0,
        .buy_fee = 0.05,
        .sell_price = 1.0,
        .sell_fee = 0.05,
    };

    Result const &actual = results[0];

    CHECK(actual == expected);
  }

  {
    Result const expected{
        .buy_date = buy_entries[0].time,
        .sell_date = sell_entries[1].time,
        .volume = 5,
        .buy_price = 2.0,
        .buy_fee = 0.05,
        .sell_price = 1.5,
        .sell_fee = 0.08,
    };

    Result const &actual = results[1];

    CHECK(actual == expected);
  }
}

TEST_CASE("2 buys. 1 sell") {
  using namespace fifo_calculator;
  using namespace std::literals;

  std::array<Entry, 2> const buy_entries{
      Entry{.time = std::chrono::utc_clock::now(),
            .volume = 10.0,
            .price = 2.0,
            .fee = 0.1},
      Entry{.time = std::chrono::utc_clock::now(),
            .volume = 20.0,
            .price = 3.0,
            .fee = 0.2}};

  std::array<Entry, 1> const sell_entries{
      Entry{.time = std::chrono::utc_clock::now() + 1s,
            .volume = 20.0,
            .price = 4.0,
            .fee = 0.5}};

  std::pmr::vector<Result> const results =
      calculate_fifo(buy_entries, sell_entries, {}, {});

  REQUIRE(results.size() == 2);

  {
    Result const expected{
        .buy_date = buy_entries[0].time,
        .sell_date = sell_entries[0].time,
        .volume = 10,
        .buy_price = 2.0,
        .buy_fee = 0.1,
        .sell_price = 4.0,
        .sell_fee = 10.0 / 20.0 * 0.5,
    };

    Result const &actual = results[0];

    CHECK(actual == expected);
  }

  {
    Result const expected{
        .buy_date = buy_entries[1].time,
        .sell_date = sell_entries[0].time,
        .volume = 10,
        .buy_price = 3.0,
        .buy_fee = 10.0 / 20.0 * 0.2,
        .sell_price = 4.0,
        .sell_fee = 10.0 / 20.0 * 0.5,
    };

    Result const &actual = results[1];

    CHECK(actual == expected);
  }
}

TEST_CASE("Read csv") {
  using namespace fifo_calculator;
  using namespace std::literals;

  std::array<Entry, 1> const expected_buy_entries{
      Entry{.time = std::chrono::utc_clock::now(),
            .volume = 10.0,
            .price = 2.0,
            .fee = 0.1}};

  std::array<Entry, 2> const expected_sell_entries{
      Entry{.time = std::chrono::utc_clock::now() + 1s,
            .volume = 5.0,
            .price = 1.0,
            .fee = 0.05},
      Entry{
          .time = std::chrono::utc_clock::now() + 3s,
          .volume = 5.0,
          .price = 1.5,
          .fee = 0.08,
      }};

  std::stringstream string_stream;

  for (Entry const &entry : expected_buy_entries) {
    string_stream << entry.time << ",";
    string_stream << "buy,";
    string_stream << entry.volume << ",";
    string_stream << entry.price << ",";
    string_stream << entry.fee << "\n";
  }

  for (Entry const &entry : expected_sell_entries) {
    string_stream << entry.time << ",";
    string_stream << "sell,";
    string_stream << entry.volume << ",";
    string_stream << entry.price << ",";
    string_stream << entry.fee << "\n";
  }

  Entry_format const entry_format{
      .time_column_index = 0,
      .volume_column_index = 2,
      .price_column_index = 3,
      .fee_column_index = 4,
      .buy_or_sell_column_index = 1,
      .separator = ',',
  };

  File_entries const file_entries =
      read_file_entries(string_stream, entry_format, {}, {});

  REQUIRE(file_entries.buy_entries.size() == expected_buy_entries.size());
  REQUIRE(file_entries.sell_entries.size() == expected_sell_entries.size());

  CHECK(file_entries.buy_entries[0] == expected_buy_entries[0]);
  CHECK(file_entries.sell_entries[0] == expected_sell_entries[0]);
  CHECK(file_entries.sell_entries[1] == expected_sell_entries[1]);
}
} // namespace fifo_calculator::tests
