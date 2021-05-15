// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef SRC_SCOREPOST_HPP
#define SRC_SCOREPOST_HPP

#include "pocketdb/models/base/Transaction.hpp"

namespace PocketTx
{

    class ScorePost : public Transaction
    {
    public:

        ScorePost(string hash, int64_t time) : Transaction(hash, time)
        {
            SetType(PocketTxType::ACTION_SCORE_POST);
        }

        void Deserialize(const UniValue &src) override
        {
            Transaction::Deserialize(src);
            if (auto[ok, val] = TryGetStr(src, "posttxid"); ok) SetPostTxHash(val);
            if (auto[ok, val] = TryGetInt64(src, "value"); ok) SetValue(val);
        }

        shared_ptr<int64_t> GetPostTxId() const { return m_int1; }
        string GetPostTxHashStr() const { return m_post_tx_hash ? *m_post_tx_hash : ""; }
        void SetPostTxId(int64_t value) { m_int1 = make_shared<int64_t>(value); }
        void SetPostTxHash(string value) { m_post_tx_hash = make_shared<string>(value); }

        shared_ptr<int64_t> GetValue() const { return m_int2; }
        string GetValueStr() const { return m_int2 == nullptr ? "" : std::to_string(*m_int2); }
        void SetValue(int64_t value) { m_int2 = make_shared<int64_t>(value); }

    protected:
        shared_ptr<string> m_post_tx_hash = nullptr;

    private:

        void BuildPayload(const UniValue &src) override
        {
        }

        void BuildHash(const UniValue &src) override
        {
            std::string data;
            data += GetPostTxHashStr();
            data += GetValueStr();
            Transaction::GenerateHash(data);
        }
    };

} // namespace PocketTx

#endif //SRC_SCOREPOST_HPP