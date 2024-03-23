export module LevelInfo;

import Globals;

export
{
    struct LevelInfo
    {
        Price price_;
        Quantity quantity_;
    };

    using LevelInfos = std::vector<LevelInfo>;
}
