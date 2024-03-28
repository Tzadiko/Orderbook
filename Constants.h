#pragma once

#include <limits>

#include "Usings.h"

struct Constants
{
    static const Price InvalidPrice = std::numeric_limits<Price>::quiet_NaN();
};
