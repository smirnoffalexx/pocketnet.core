// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 Bitcoin developers
// Copyright (c) 2018-2021 Pocketnet developers
// Distributed under the Apache 2.0 software license, see the accompanying
// https://www.apache.org/licenses/LICENSE-2.0


#ifndef POCKETDB_SQLITEDATABASE_H
#define POCKETDB_SQLITEDATABASE_H

#include "logging.h"
#include "sqlite3.h"
#include "sync.h"
#include "tinyformat.h"
#include "fs.h"

#include <iostream>

namespace PocketDb
{
    class SQLiteDatabase
    {
    private:
        std::string m_dir_path;
        std::string m_file_path;

        void Cleanup() noexcept
        {
            Close();

            //LOCK(g_sqlite_mutex);
            if (--g_sqlite_count == 0)
            {
                int ret = sqlite3_shutdown();
                if (ret != SQLITE_OK)
                {
                    LogPrintf("%s: %d; Failed to shutdown SQLite: %s\n", __func__, ret, sqlite3_errstr(ret));
                }
            }
        }

        Mutex g_sqlite_mutex;
        int g_sqlite_count = 0; //GUARDED_BY(g_sqlite_mutex)

        static void ErrorLogCallback(void* arg, int code, const char* msg)
        {
            // From sqlite3_config() documentation for the SQLITE_CONFIG_LOG option:
            // "The void pointer that is the second argument to SQLITE_CONFIG_LOG is passed through as
            // the first parameter to the application-defined logger function whenever that function is
            // invoked."
            // Assert that this is the case:
            assert(arg == nullptr);
            LogPrintf("%s: %d; Message: %s\n", __func__, code, msg);
        }

        bool TryCreateDbIfNotExists()
        {
            try
            {
                return fs::create_directories(m_dir_path);
            }
            catch (const fs::filesystem_error&)
            {
                if (!fs::exists(m_dir_path) || !fs::is_directory(m_dir_path))
                    throw;

                return false;
            }
        }

        bool CreateStructure() const
        {
            std::string generate_sql = R"sql(
                create table if not exists Transactions
                (
                    Type      int    not null,
                    Hash      string not null primary key,
                    Time      int    not null,

                    BlockHash string null,
                    Height    int    null,

                    -- User.Id
                    -- Post.Id
                    -- Comment.Id
                    Id        int    null,

                    -- User.AddressHash
                    -- Post.AddressHash
                    -- Comment.AddressHash
                    -- ScorePost.AddressHash
                    -- ScoreComment.AddressHash
                    -- Subscribe.AddressHash
                    -- Blocking.AddressHash
                    -- Complain.AddressHash
                    String1   string null,

                    -- User.ReferrerAddressHash
                    -- Post.RootTxHash
                    -- Comment.RootTxHash
                    -- ScorePost.PostTxHash
                    -- ScoreComment.CommentTxHash
                    -- Subscribe.AddressToHash
                    -- Blocking.AddressToHash
                    -- Complain.PostTxHash
                    String2   string null,

                    -- Post.RelayTxHash
                    -- Comment.PostTxHash
                    String3   string null,

                    -- Comment.ParentTxHash
                    String4   string null,

                    -- Comment.AnswerTxHash
                    String5   string null,

                    -- ScorePost.Value
                    -- ScoreComment.Value
                    -- Complain.Reason
                    Int1      int    null
                );

                create index if not exists Transactions_Type on Transactions (Type);
                create index if not exists Transactions_Hash on Transactions (Hash);
                create index if not exists Transactions_Time on Transactions (Time);
                create index if not exists Transactions_BlockHash on Transactions (BlockHash);
                create index if not exists Transactions_Height on Transactions (Height);
                create index if not exists Transactions_String1 on Transactions (String1);
                create index if not exists Transactions_String2 on Transactions (String2);
                create index if not exists Transactions_String3 on Transactions (String3);
                create index if not exists Transactions_String4 on Transactions (String4);
                create index if not exists Transactions_String5 on Transactions (String5);
                create index if not exists Transactions_Int1 on Transactions (Int1);

                create index if not exists Transactions_Type_String1_index on Transactions (Type, String1);
                create index if not exists Transactions_Type_String2_index on Transactions (Type, String2);

                --------------------------------------------
                --               EXT TABLES               --
                --------------------------------------------
                create table if not exists Payload
                (
                    TxHash  string primary key, -- Transactions.Hash

                    -- User.Lang
                    -- Post.Lang
                    -- Comment.Lang
                    String1 string null,

                    -- User.Name
                    -- Post.Caption
                    -- Comment.Message
                    String2 string null,

                    -- User.Avatar
                    -- Post.Message
                    String3 string null,

                    -- User.About
                    -- Post.Tags JSON
                    String4 string null,

                    -- User.Url
                    -- Post.Images JSON
                    String5 string null,

                    -- User.Pubkey
                    -- Post.Settings JSON
                    String6 string null,

                    -- User.Donations JSON
                    -- Post.Url
                    String7 string null
                );


                --------------------------------------------
                create table if not exists TxOutputs
                (
                    TxHash      string not null, -- Transactions.Hash
                    Number      int    not null, -- Number in tx.vout
                    AddressHash string not null, -- Address
                    Value       int    not null, -- Amount
                    SpentHeight int    null,     -- Where spent
                    SpentTxHash string null,     -- Who spent
                    primary key (TxHash, Number, AddressHash)
                );
                create index if not exists TxOutput_SpentHeight on TxOutputs (SpentHeight);
                create index if not exists TxOutputs_AddressHash_SpentHeight_Value on TxOutputs (AddressHash, SpentHeight, Value);

                --------------------------------------------
                create table if not exists Ratings
                (
                    Type   int not null,
                    Height int not null,
                    Id     int not null,
                    Value  int not null,
                    primary key (Type, Height, Id, Value)
                );
                create index if not exists Ratings_Height on Ratings (Height);
                create index if not exists Ratings_Type_Id_Value on Ratings (Type, Id, Value);
                create index if not exists Ratings_Type_Height_Id on Ratings (Type, Height, Id);

                --------------------------------------------
                --                 VIEWS                  --
                --------------------------------------------
                drop view if exists vAccounts;
                create view vAccounts as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2 as ReferrerAddressHash
                from Transactions t
                where t.Height is not null and t.Type in (100,101,102);

                drop view if exists vUsers;
                create view vUsers as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2 as ReferrerAddressHash
                from Transactions t
                where t.Height is not null and t.Type = 100;

                --------------------------------------------
                drop view if exists vContents;
                create view vContents as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Id,
                       t.String1 as AddressHash
                from Transactions t
                where t.Height is not null and t.Type in (200,201,202,203,204,205);

                drop view if exists vPosts;
                create view vPosts as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2 as RootTxHash,
                       t.String3 as RelayTxHash
                from Transactions t
                where t.Height is not null and t.Type = 200;


                drop view if exists vComments;
                create view vComments as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.Id,
                       t.String1 as AddressHash,
                       t.String2 as RootTxHash,
                       t.String3 as PostTxHash,
                       t.String4 as ParentTxHash,
                       t.String5 as AnswerTxHash
                from Transactions t
                where t.Height is not null and t.Type = 204;

                --------------------------------------------
                drop view if exists vScores;
                create view vScores as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.String1 as AddressHash,
                       t.String2 as ContentTxHash,
                       t.Int1    as Value
                from Transactions t
                where t.Height is not null and t.Type in (300, 301);

                drop view if exists vScorePosts;
                create view vScorePosts as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.String1 as AddressHash,
                       t.String2 as PostTxHash,
                       t.Int1    as Value
                from Transactions t
                where t.Height is not null and t.Type in (300);

                drop view if exists vScoreComments;
                create view vScoreComments as
                select t.Type,
                       t.Hash,
                       t.Time,
                       t.BlockHash,
                       t.Height,
                       t.String1 as AddressHash,
                       t.String2 as CommentTxHash,
                       t.Int1    as Value
                from Transactions t
                where t.Height is not null and t.Type in (301);


            )sql";

            BeginTransaction();

            try
            {
                char* errMsg = nullptr;
                size_t pos;
                std::string token;

                while ((pos = generate_sql.find(';')) != std::string::npos)
                {
                    token = generate_sql.substr(0, pos);

                    if (sqlite3_exec(m_db, token.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK)
                    {
                        throw std::runtime_error("Failed to create init database");
                    }

                    generate_sql.erase(0, pos + 1);
                }

                CommitTransaction();
            }
            catch (const std::exception&)
            {
                AbortTransaction();
                LogPrintf("%s: Failed to create init database structure", __func__);
                return false;
            }

            return true;
        }

    public:
        sqlite3* m_db{nullptr};

        SQLiteDatabase() = default;

        void Init(const std::string& dir_path, const std::string& file_path)
        {
            m_dir_path = dir_path;
            m_file_path = file_path;

            if (++g_sqlite_count == 1)
            {
                // Setup logging
                int ret = sqlite3_config(SQLITE_CONFIG_LOG, ErrorLogCallback, nullptr);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %sd Failed to setup error log: %s\n", __func__, ret, sqlite3_errstr(ret)));
                }
                // Force serialized threading mode
                ret = sqlite3_config(SQLITE_CONFIG_SERIALIZED);
                if (ret != SQLITE_OK)
                {
                    throw std::runtime_error(
                        strprintf("%s: %d; Failed to configure serialized threading mode: %s\n",
                            __func__, ret, sqlite3_errstr(ret)));
                }

                TryCreateDbIfNotExists();
            }
            int ret = sqlite3_initialize(); // This is a no-op if sqlite3 is already initialized
            if (ret != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to initialize SQLite: %s\n", __func__, ret, sqlite3_errstr(ret)));
            }

            try
            {
                Open();
            } catch (const std::runtime_error&)
            {
                // If open fails, cleanup this object and rethrow the exception
                Cleanup();
                throw;
            }
        }

        void Open()
        {
            int flags = SQLITE_OPEN_READWRITE |
                        SQLITE_OPEN_CREATE; //SQLITE_OPEN_FULLMUTEX | SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE;

            if (m_db == nullptr)
            {
                int ret = sqlite3_open_v2(m_file_path.c_str(), &m_db, flags, nullptr);
                if (ret != SQLITE_OK)
                    throw std::runtime_error(strprintf("%s: %d; Failed to open database: %s\n",
                        __func__, ret, sqlite3_errstr(ret)));

                if (!CreateStructure())
                {
                    throw std::runtime_error(strprintf("%s: Failed to create database\n",
                        __func__));
                }
            }

            if (sqlite3_db_readonly(m_db, "main") != 0)
                throw std::runtime_error("Database opened in readonly");

            if (sqlite3_exec(m_db, "PRAGMA journal_mode = wal;", nullptr, nullptr, nullptr) != 0)
                throw std::runtime_error("Failed apply journal_mode = wal");

            //    // Acquire an exclusive lock on the database
            //    // First change the locking mode to exclusive
            //    int ret = sqlite3_exec(m_db, "PRAGMA locking_mode = exclusive", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to change database locking mode to exclusive: %s\n", sqlite3_errstr(ret)));
            //    }
            //    // Now begin a transaction to acquire the exclusive lock. This lock won't be released until we close because of the exclusive locking mode.
            //    ret = sqlite3_exec(m_db, "BEGIN EXCLUSIVE TRANSACTION", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error("SQLiteDatabase: Unable to obtain an exclusive lock on the database, is it being used by another bitcoind?\n");
            //    }
            //    ret = sqlite3_exec(m_db, "COMMIT", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Unable to end exclusive lock transaction: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Enable fullfsync for the platforms that use it
            //    ret = sqlite3_exec(m_db, "PRAGMA fullfsync = true", nullptr, nullptr, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to enable fullfsync: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Make the table for our key-value pairs
            //    // First check that the main table exists
            //    sqlite3_stmt* check_main_stmt{nullptr};
            //    ret = sqlite3_prepare_v2(m_db, "SELECT name FROM sqlite_master WHERE type='table' AND name='main'", -1, &check_main_stmt, nullptr);
            //    if (ret != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to prepare statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    ret = sqlite3_step(check_main_stmt);
            //    if (sqlite3_finalize(check_main_stmt) != SQLITE_OK) {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to finalize statement checking table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //    bool table_exists;
            //    if (ret == SQLITE_DONE) {
            //        table_exists = false;
            //    } else if (ret == SQLITE_ROW) {
            //        table_exists = true;
            //    } else {
            //        throw std::runtime_error(strprintf("SQLiteDatabase: Failed to execute statement to check table existence: %s\n", sqlite3_errstr(ret)));
            //    }
            //
            //    // Do the db setup things because the table doesn't exist only when we are creating a new wallet
            //    if (!table_exists) {
            //        ret = sqlite3_exec(m_db, "CREATE TABLE main(key BLOB PRIMARY KEY NOT NULL, value BLOB NOT NULL)", nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to create new database: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the application id
            //        uint32_t app_id = ReadBE32(Params().MessageStart());
            //        std::string set_app_id = strprintf("PRAGMA application_id = %d", static_cast<int32_t>(app_id));
            //        ret = sqlite3_exec(m_db, set_app_id.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the application id: %s\n", sqlite3_errstr(ret)));
            //        }

            // Set the user version
            //        std::string set_user_ver = strprintf("PRAGMA user_version = %d", WALLET_SCHEMA_VERSION);
            //        ret = sqlite3_exec(m_db, set_user_ver.c_str(), nullptr, nullptr, nullptr);
            //        if (ret != SQLITE_OK) {
            //            throw std::runtime_error(strprintf("SQLiteDatabase: Failed to set the wallet schema version: %s\n", sqlite3_errstr(ret)));
            //        }
            //    }
        }

        void Close()
        {
            int res = sqlite3_close(m_db);
            if (res != SQLITE_OK)
            {
                throw std::runtime_error(
                    strprintf("%s: %d; Failed to close database: %s\n", __func__, res, sqlite3_errstr(res)));
            }
            m_db = nullptr;
        }

        bool BeginTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) == 0) return false;
            int res = sqlite3_exec(m_db, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to begin the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool CommitTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "COMMIT TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to commit the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

        bool AbortTransaction() const
        {
            if (!m_db || sqlite3_get_autocommit(m_db) != 0) return false;
            int res = sqlite3_exec(m_db, "ROLLBACK TRANSACTION", nullptr, nullptr, nullptr);
            if (res != SQLITE_OK)
            {
                LogPrintf("%s: %d; Failed to abort the transaction: %s\n", __func__, res, sqlite3_errstr(res));
            }
            return res == SQLITE_OK;
        }

    };
} // namespace PocketDb

#endif // POCKETDB_SQLITEDATABASE_H
