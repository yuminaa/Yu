#include "../include/tokens.h"

namespace yu::lang
{
    void TokenList::push_back(const token_t &token)
    {
        starts.emplace_back(token.start);
        lengths.emplace_back(token.length);
        types.emplace_back(token.type);
        flags.emplace_back(token.flags);
    }

    void TokenList::reserve(const uint32_t &n)
    {
        starts.reserve(n);
        lengths.reserve(n);
        types.reserve(n);
        flags.reserve(n);
    }

    size_t TokenList::size() const
    {
        return starts.size();
    }
}