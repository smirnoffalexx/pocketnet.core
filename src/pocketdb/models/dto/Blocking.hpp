// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETTX_BLOCKING_HPP
#define POCKETTX_BLOCKING_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class Blocking : public Transaction
    {
    public:

        Blocking() : Transaction()
        {
            SetTxType(PocketTxType::BLOCKING_ACTION);
        }

        void Deserialize(const UniValue &src) override {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "address_to"); ok) SetAddressTo(val);
        }

        shared_ptr<string> GetAddressTo() const { return m_string1; }
        void SetAddressTo(std::string value) { m_string1 = make_shared<string>(value); }

    private:

        void BuildPayload(const UniValue &src) override { }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            if (auto[ok, val] = TryGetStr(src, "address_to"); ok) data += val;

            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif // POCKETTX_BLOCKING_HPP