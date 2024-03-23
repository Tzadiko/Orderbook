export module Globals;

import <vector>;

export
{
	using Price = std::int32_t;
	using Quantity = std::uint32_t;
	using OrderId = std::uint64_t;
	using OrderIds = std::vector<OrderId>;
}