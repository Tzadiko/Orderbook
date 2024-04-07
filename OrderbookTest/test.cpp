#include "pch.h"

#include "../Orderbook.cpp"

namespace googletest = ::testing;

enum class ActionType
{
    Add,
    Cancel,
    Modify,
};

struct Information
{
    ActionType type_;
    OrderType orderType_;
    Side side_;
    Price price_;
    Quantity quantity_;
    OrderId orderId_;
};

using Informations = std::vector<Information>;

struct Result
{
    std::size_t allCount_;
    std::size_t bidCount_;
    std::size_t askCount_;
};

using Results = std::vector<Result>;

struct InputHandler
{
private:
    std::uint32_t ToNumber(const std::string_view& str) const
    {
        std::int64_t value{};
        std::from_chars(str.data(), str.data() + str.size(), value);
        if (value < 0)
            throw std::logic_error("Value is below zero.");
        return static_cast<std::uint32_t>(value);
    }

    bool TryParseResult(const std::string_view& str, Result& result) const
    {
        if (str.at(0) != 'R')
            return false;

        auto values = Split(str, ' ');
        result.allCount_ = ToNumber(values[1]);
        result.bidCount_ = ToNumber(values[2]);
        result.askCount_ = ToNumber(values[3]);

        return true;
    }

    bool TryParseInformation(const std::string_view& str, Information& action) const
    {
        auto value = str.at(0);
        auto values = Split(str, ' ');
        if (value == 'A')
        {
            action.type_ = ActionType::Add;
            action.side_ = ParseSide(values[1]);
            action.orderType_ = ParseOrderType(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
            action.orderId_ = ParseOrderId(values[5]);
        }
        else if (value == 'M')
        {
            action.type_ = ActionType::Modify;
            action.orderId_ = ParseOrderId(values[1]);
            action.side_ = ParseSide(values[2]);
            action.price_ = ParsePrice(values[3]);
            action.quantity_ = ParseQuantity(values[4]);
        }
        else if (value == 'C')
        {
            action.type_ = ActionType::Cancel;
            action.orderId_ = ParseOrderId(values[1]);
        }
        else return false;

        return true;
    }

    std::vector<std::string_view> Split(const std::string_view& str, char delimeter) const
    {
        std::vector<std::string_view> columns;
        columns.reserve(5);
        std::size_t start_index{}, end_index{};
        while ((end_index = str.find(delimeter, start_index)) && end_index != std::string::npos)
        {
            auto distance = end_index - start_index;
            auto column = str.substr(start_index, distance);
            start_index = end_index + 1;
            columns.push_back(column);
        }
        columns.push_back(str.substr(start_index));
        return columns;
    }

    Side ParseSide(const std::string_view& str) const
    {
        if (str == "B")
            return Side::Buy;
        else if (str == "S")
            return Side::Sell;
        else throw std::logic_error("Unknown Side");
    }

    OrderType ParseOrderType(const std::string_view& str) const
    {
        if (str == "FillAndKill")
            return OrderType::FillAndKill;
        else if (str == "GoodTillCancel")
            return OrderType::GoodTillCancel;
        else if (str == "GoodForDay")
            return OrderType::GoodForDay;
        else if (str == "FillOrKill")
            return OrderType::FillOrKill;
        else if (str == "Market")
            return OrderType::Market;
        else throw std::logic_error("Unknown OrderType");
    }

    Price ParsePrice(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Price");

        return ToNumber(str);
    }

    Quantity ParseQuantity(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Unknown Quantity");

        return ToNumber(str);
    }

    OrderId ParseOrderId(const std::string_view& str) const
    {
        if (str.empty())
            throw std::logic_error("Empty OrderId");

        return static_cast<OrderId>(ToNumber(str));
    }

public:
    std::tuple<Informations, Result> GetInformations(const std::filesystem::path& path) const
    {
        Informations actions;
        actions.reserve(1'000);

        std::string line;
        std::ifstream file{ path };
        while (std::getline(file, line))
        {
            if (line.empty())
                break;

            const bool isResult = line.at(0) == 'R';
            const bool isAction = !isResult;
            
            if (isAction)
            {
                Information action;

                auto isValid = TryParseInformation(line, action);
                if (!isValid)
                    continue;

                actions.push_back(action);
            }
            else
            {
                if (!file.eof())
                    throw std::logic_error("Result should only be specified at the end.");

                Result result;

                auto isValid = TryParseResult(line, result);
                if (!isValid)
                    continue;

                return { actions, result };
            }

        }

        throw std::logic_error("No result specified.");
    }
};


class OrderbookTestsFixture : public googletest::TestWithParam<const char*> 
{
private:
    const static inline std::filesystem::path Root{ std::filesystem::current_path() };
    const static inline std::filesystem::path TestFolder{ "TestFiles" };
public:
    const static inline std::filesystem::path TestFolderPath{ Root / TestFolder };
};

TEST_P(OrderbookTestsFixture, OrderbookTestSuite)
{
    // Arrange
    const auto file = OrderbookTestsFixture::TestFolderPath / GetParam();

    InputHandler handler;
    const auto [actions, result] = handler.GetInformations(file);

    auto GetOrder = [](const Information& action)
    {
        return std::make_shared<Order>(
            action.orderType_,
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_);
    };

    auto GetOrderModify = [](const Information& action)
    {
        return OrderModify
        {
            action.orderId_,
            action.side_,
            action.price_,
            action.quantity_,
        };
    };

    // Act
    Orderbook orderbook;
    for (const auto& action : actions)
    {
        switch (action.type_)
        {
        case ActionType::Add:
        {
            const Trades& trades = orderbook.AddOrder(GetOrder(action));
        }
        break;
        case ActionType::Modify:
        {
            const Trades& trades = orderbook.ModifyOrder(GetOrderModify(action));
        }
        break;
        case ActionType::Cancel:
        {
            orderbook.CancelOrder(action.orderId_);
        }
        break;
        default:
            throw std::logic_error("Unsupported Action.");
        }
    }

    // Assert
    const auto& orderbookInfos = orderbook.GetOrderInfos();
    ASSERT_EQ(orderbook.Size(), result.allCount_);
    ASSERT_EQ(orderbookInfos.GetBids().size(), result.bidCount_);
    ASSERT_EQ(orderbookInfos.GetAsks().size(), result.askCount_);
}

INSTANTIATE_TEST_CASE_P(Tests, OrderbookTestsFixture, googletest::ValuesIn({
    "Match_GoodTillCancel.txt",
    "Match_FillAndKill.txt",
    "Match_FillOrKill_Hit.txt",
    "Match_FillOrKill_Miss.txt",
    "Cancel_Success.txt",
    "Modify_Side.txt",
    "Match_Market.txt"
}));
