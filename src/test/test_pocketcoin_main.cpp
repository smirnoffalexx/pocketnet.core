// Copyright (c) 2011-2018 The Pocketcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#define BOOST_TEST_MODULE Pocketcoin Test Suite

#include <net.h>

#include <memory>

#include <boost/test/unit_test.hpp>

std::unique_ptr<CConnman> g_connman_main;

[[noreturn]] void Shutdown(void* parg)
{
  std::exit(EXIT_SUCCESS);
}

[[noreturn]] void StartShutdown_main()
{
  std::exit(EXIT_SUCCESS);
}

bool ShutdownRequested_main()
{
  return false;
}
