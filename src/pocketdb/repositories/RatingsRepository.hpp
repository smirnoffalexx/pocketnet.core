// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0

#ifndef SRC_RATINGSREPOSITORY_HPP
#define SRC_RATINGSREPOSITORY_HPP

#include <util.h>

#include "pocketdb/repositories/BaseRepository.hpp"
#include "pocketdb/models/base/Rating.hpp"
#include "pocketdb/models/dto/ReturnDtoModels.hpp"

namespace PocketDb
{
    using std::runtime_error;

    using namespace PocketTx;

    class RatingsRepository : public BaseRepository
    {
    public:
        explicit RatingsRepository(SQLiteDatabase& db) : BaseRepository(db) {}

        void Init() override {}
        void Destroy() override {}

        // Accumulate new rating records
        bool InsertRatings(shared_ptr<vector<Rating>> ratings)
        {
            for (const auto& rating : *ratings)
            {
                auto result = TryTransactionStep([&]()
                {
                    if (*rating.GetType() != RatingType::RATING_ACCOUNT_LIKERS)
                    {
                        int64_t nTime1 = GetTimeMicros();

                        InsertRating(rating);

                        int64_t nTime2 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - InsertRating: %.2fms\n", 0.001 * (nTime2 - nTime1));
                    }
                    else
                    {
                        int64_t nTime1 = GetTimeMicros();

                        InsertLiker(rating);

                        int64_t nTime2 = GetTimeMicros();
                        LogPrint(BCLog::BENCH, "      - InsertLiker: %.2fms\n", 0.001 * (nTime2 - nTime1));
                    }
                });

                if (!result)
                    return false;
            }

            return true;
        }

        tuple<bool, int> GetUserReputation(int addressId, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select r.Value
                    from Ratings r
                    where r.Type = ?
                        and r.Id = ?
                        and r.Height <= ?
                    order by r.Height desc
                    limit 1
                )sql");

                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT);
                auto heightPtr = make_shared<int>(height);
                auto addressIdPtr = make_shared<int>(addressId);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, addressIdPtr);
                bindResult &= TryBindStatementInt(stmt, 3, heightPtr);

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't get user reputation (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        tuple<bool, int> GetUserLikersCount(int addressId, int height)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
                auto stmt = SetupSqlStatement(R"sql(
                    select count(1)
                    from Ratings r
                    where r.Type = ?
                        and r.Height <= ?
                        and r.Id = ?
                )sql");

                auto typePtr = make_shared<int>(RatingType::RATING_ACCOUNT_LIKERS);
                auto heightPtr = make_shared<int>(height);
                auto addressIdPtr = make_shared<int>(addressId);

                auto bindResult = TryBindStatementInt(stmt, 1, typePtr);
                bindResult &= TryBindStatementInt(stmt, 2, heightPtr);
                bindResult &= TryBindStatementInt(stmt, 3, addressIdPtr);
                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't select user likers count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

        tuple<bool, int> GetScoreContentCount(PocketTxType scoreType,
            const string& scoreAddress, const string& contentAddress,
            int height, const CTransactionRef& tx,
            const std::vector<int>& values,
            int64_t scoresOneToOneDepth)
        {
            int result = 0;
            auto func = __func__;

            bool tryResult = TryTransactionStep([&]()
            {
                string sql = R"sql(
                    select count(1)
                    from vScores s
                    join vContents c on c.Hash = s.ContentTxHash
                    where   s.AddressHash = ?
                        and c.AddressHash = ?
                        and s.Height is not null
                        and s.Height <= ?
                        and s.Time < ?
                        and s.Time >= ?
                        and s.Hash != ?
                        and s.Type = ?
                        and s.Value in
                )sql";

                sql += "(";
                sql += std::to_string(values[0]);
                for (size_t i = 1; i < values.size(); i++)
                {
                    sql += ',';
                    sql += std::to_string(values[i]);
                }
                sql += ")";

                auto stmt = SetupSqlStatement(sql);

                auto scoreAddressPtr = make_shared<string>(scoreAddress);
                auto postAddressPtr = make_shared<string>(contentAddress);
                auto heightPtr = make_shared<int>(height);

                // TODO: нужна будет перегрузка для изменения отбора - время заменить на блоки
                auto maxTimePtr = make_shared<int64_t>(tx->nTime);
                auto minTimePtr = make_shared<int64_t>((int64_t) tx->nTime - scoresOneToOneDepth);

                auto scoreTxHashPtr = make_shared<string>(tx->GetHash().GetHex());
                auto scoreTypePtr = make_shared<int>(scoreType);

                auto bindResult = TryBindStatementText(stmt, 1, scoreAddressPtr);
                bindResult &= TryBindStatementText(stmt, 2, postAddressPtr);
                bindResult &= TryBindStatementInt(stmt, 3, heightPtr);
                bindResult &= TryBindStatementInt64(stmt, 4, maxTimePtr);
                bindResult &= TryBindStatementInt64(stmt, 5, minTimePtr);
                bindResult &= TryBindStatementText(stmt, 6, scoreTxHashPtr);
                bindResult &= TryBindStatementInt(stmt, 7, scoreTypePtr);

                if (!bindResult)
                {
                    FinalizeSqlStatement(*stmt);
                    throw runtime_error(strprintf("%s: can't get score count (bind)\n", func));
                }

                if (sqlite3_step(*stmt) == SQLITE_ROW)
                {
                    result = GetColumnInt(*stmt, 0);
                }

                FinalizeSqlStatement(*stmt);
                return true;
            });

            return make_tuple(tryResult, result);
        }

    private:

        void InsertRating(const Rating& rating)
        {
            // Build sql statement with auto select IDs
            // TODO (brangr): limit 1 after debugging
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,
                    ifnull((
                        select r.Value
                        from Ratings r
                        where r.Type = ?
                            and r.Id = ?
                            and r.Height < ?
                        order by r.Height desc
                        limit 1
                    ), 0) + ?
            )sql");

            // Bind arguments
            auto result = TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            result &= TryBindStatementInt(stmt, 2, rating.GetHeight());
            result &= TryBindStatementInt64(stmt, 3, rating.GetId());
            result &= TryBindStatementInt(stmt, 4, rating.GetTypeInt());
            result &= TryBindStatementInt64(stmt, 5, rating.GetId());
            result &= TryBindStatementInt(stmt, 6, rating.GetHeight());
            result &= TryBindStatementInt64(stmt, 7, rating.GetValue());
            if (!result)
                throw runtime_error(strprintf("%s: can't insert in ratings (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in ratings (step) Type:%d Height:%d Hash:%s\n",
                    __func__, *rating.GetTypeInt(), *rating.GetHeight(), *rating.GetId()));
        }

        void InsertLiker(const Rating& rating)
        {
            // Build sql statement with auto select IDs
            auto stmt = SetupSqlStatement(R"sql(
                INSERT OR FAIL INTO Ratings (
                    Type,
                    Height,
                    Id,
                    Value
                ) SELECT ?,?,?,?
                WHERE NOT EXISTS (select 1 from Ratings r where r.Type=? and r.Id=? and r.Value=?)
            )sql");

            // Bind arguments
            auto result = TryBindStatementInt(stmt, 1, rating.GetTypeInt());
            result &= TryBindStatementInt(stmt, 2, rating.GetHeight());
            result &= TryBindStatementInt64(stmt, 3, rating.GetId());
            result &= TryBindStatementInt64(stmt, 4, rating.GetValue());
            result &= TryBindStatementInt(stmt, 5, rating.GetTypeInt());
            result &= TryBindStatementInt64(stmt, 6, rating.GetId());
            result &= TryBindStatementInt64(stmt, 7, rating.GetValue());
            if (!result)
                throw runtime_error(strprintf("%s: can't insert in likers (bind)\n", __func__));

            // Try execute
            if (!TryStepStatement(stmt))
                throw runtime_error(strprintf("%s: can't insert in likers (step) Type:%d Height:%d Hash:%s Value:%s\n",
                    __func__, *rating.GetTypeInt(), *rating.GetHeight(), *rating.GetId(), *rating.GetValue()));
        }

    }; // namespace PocketDb
}
#endif //SRC_RATINGSREPOSITORY_HPP