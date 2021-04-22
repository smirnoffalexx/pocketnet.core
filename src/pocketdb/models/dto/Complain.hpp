// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_COMPLAIN_HPP
#define POCKETTX_COMPLAIN_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Complain : public Transaction
    {
    public:
        ~Complain() = default;

        Complain(const UniValue &src) : Transaction(src)
        {
            SetTxType(PocketTxType::COMPLAIN_ACTION);

            assert(src.exists("reason") && src["reason"].isNum());
            SetReason(src["reason"].get_int());

            assert(src.exists("posttxid") && src["posttxid"].isStr());
            SetPostTxId(src["posttxid"].get_str());
        }
        

        [[nodiscard]] int64_t* GetReason() const { return m_int1; }
        void SetReason(int64_t value) { m_int1 = new int64_t(value); }

        [[nodiscard]] std::string* GetPostTxId() const { return m_string1; }
        void SetPostTxId(std::string value) { m_string1 = new std::string(std::move(value)); }
    };

} // namespace PocketTx

#endif // POCKETTX_COMPLAIN_HPP
