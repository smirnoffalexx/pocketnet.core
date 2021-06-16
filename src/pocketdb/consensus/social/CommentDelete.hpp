// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef POCKETCONSENSUS_COMMENT_DELETE_HPP
#define POCKETCONSENSUS_COMMENT_DELETE_HPP

#include "pocketdb/consensus/social/Base.hpp"
#include "pocketdb/models/dto/CommentDelete.hpp"

namespace PocketConsensus
{
    /*******************************************************************************************************************
    *
    *  CommentDelete consensus base class
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus : public SocialBaseConsensus
    {
    public:
        explicit CommentDeleteConsensus(int height) : SocialBaseConsensus(height) {}
        CommentDeleteConsensus() : SocialBaseConsensus() {}

        tuple<bool, SocialConsensusResult> Validate(shared_ptr<Transaction> tx, PocketBlock& block) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Validate(tx, block); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Validate(static_pointer_cast<CommentDelete>(tx), block); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

        tuple<bool, SocialConsensusResult> Check(shared_ptr<Transaction> tx) override
        {
            if (auto[ok, result] = SocialBaseConsensus::Check(tx); !ok)
                return make_tuple(false, result);

            if (auto[ok, result] = Check(static_pointer_cast<CommentDelete>(tx)); !ok)
                return make_tuple(false, result);
                
            return make_tuple(true, SocialConsensusResult_Success);
        }

    protected:
    
        virtual tuple<bool, SocialConsensusResult> Validate(shared_ptr<CommentDelete> tx, PocketBlock& block)
        {
            return make_tuple(true, SocialConsensusResult_Success);
            // TODO (brangr): implement
            // std::string _address = oitm["address"].get_str();
            // int64_t _time = oitm["time"].get_int64();

            // std::string _txid = oitm["txid"].get_str();
            // std::string _otxid = oitm["otxid"].get_str();
            // std::string _parentid = oitm["parentid"].get_str();
            // std::string _answerid = oitm["answerid"].get_str();

            // // User registered?
            // if (!CheckRegistration(oitm, _address, checkMempool, checkWithTime, height, blockVtx, result)) {
            //     return false;
            // }

            // // Original comment exists
            // reindexer::Item _original_comment_itm;
            // if (!g_pocketdb->SelectOne(Query("Comment").Where("otxid", CondEq, _otxid).Where("txid", CondEq, _otxid).Where("address", CondEq, _address).Where("block", CondLt, height), _original_comment_itm).ok()) {
            //     result = ANTIBOTRESULT::NotFound;
            //     return false;
            // }

            // // Last comment not deleted
            // if (!g_pocketdb->Exists(Query("Comment").Where("otxid", CondEq, _otxid).Where("last", CondEq, true).Where("address", CondEq, _address).Not().Where("msg", CondEq, "").Where("block", CondLt, height))) {
            //     result = ANTIBOTRESULT::DoubleCommentDelete;
            //     return false;
            // }

            // // Parent comment
            // if (_parentid != _original_comment_itm["parentid"].As<string>()) {
            //     result = ANTIBOTRESULT::InvalidParentComment;
            //     return false;
            // }

            // // Answer comment
            // if (_answerid != _original_comment_itm["answerid"].As<string>()) {
            //     result = ANTIBOTRESULT::InvalidAnswerComment;
            //     return false;
            // }

            // // Double delete in block denied
            // if (blockVtx.Exists("Comment")) {
            //     for (auto& mtx : blockVtx.Data["Comment"]) {
            //         if (mtx["txid"].get_str() != _txid && mtx["otxid"].get_str() == _otxid) {
            //             result = ANTIBOTRESULT::DoubleCommentDelete;
            //             return false;
            //         }
            //     }
            // }

            // // Double delete in mempool denied
            // if (checkMempool) {
            //     reindexer::QueryResults res;
            //     if (g_pocketdb->Select(reindexer::Query("Mempool").Where("table", CondEq, "Comment").Not().Where("txid", CondEq, _txid), res).ok()) {
            //         for (auto& m : res) {
            //             reindexer::Item mItm = m.GetItem();
            //             std::string t_src = DecodeBase64(mItm["data"].As<string>());

            //             reindexer::Item t_itm = g_pocketdb->DB()->NewItem("Comment");
            //             if (t_itm.FromJSON(t_src).ok()) {
            //                 if (t_itm["otxid"].As<string>() == _otxid) {
            //                     result = ANTIBOTRESULT::DoubleCommentDelete;
            //                     return false;
            //                 }
            //             }
            //         }
            //     }
            // }

            // return true;
        }

    private:

        tuple<bool, SocialConsensusResult> Check(shared_ptr<CommentDelete> tx)
        {
            return make_tuple(true, SocialConsensusResult_Success);
        }

    };

    /*******************************************************************************************************************
    *
    *  Consensus checkpoint at 1 block
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensus_checkpoint_1 : public CommentDeleteConsensus
    {
    protected:
        int CheckpointHeight() override { return 1; }
    public:
        CommentDeleteConsensus_checkpoint_1(int height) : CommentDeleteConsensus(height) {}
    };


    /*******************************************************************************************************************
    *
    *  Factory for select actual rules version
    *  Каждая новая перегрузка добавляет новый функционал, поддерживающийся с некоторым условием - например высота
    *
    *******************************************************************************************************************/
    class CommentDeleteConsensusFactory
    {
    private:
        static inline const std::map<int, std::function<CommentDeleteConsensus*(int height)>> m_rules =
        {
            {0, [](int height) { return new CommentDeleteConsensus(height); }},
        };
    public:
        shared_ptr <CommentDeleteConsensus> Instance(int height)
        {
            return shared_ptr<CommentDeleteConsensus>(
                (--m_rules.upper_bound(height))->second(height)
            );
        }
    };
}

#endif // POCKETCONSENSUS_COMMENT_DELETE_HPP