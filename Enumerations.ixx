export module Enumerations;

export
{
	enum class OrderType
	{
		GoodTillCancel,
		FillAndKill,
		FillOrKill,
		GoodForDay,
	};

	enum class Side
	{
		Buy,
		Sell
	};
}