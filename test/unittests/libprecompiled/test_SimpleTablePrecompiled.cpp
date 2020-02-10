/*
 * @CopyRight:
 * FISCO-BCOS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * FISCO-BCOS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with FISCO-BCOS.  If not, see <http://www.gnu.org/licenses/>
 * (c) 2016-2018 fisco-dev contributors.
 */

#include "../libstorage/MemoryStorage2.h"
#include <libdevcrypto/Common.h>
#include <libethcore/ABI.h>
#include <libprecompiled/ConditionPrecompiled.h>
#include <libprecompiled/EntriesPrecompiled.h>
#include <libprecompiled/EntryPrecompiled.h>
#include <libprecompiled/SimpleTablePrecompiled.h>
#include <libstorage/MemoryTable2.h>
#include <libstorage/Table.h>
#include <boost/test/unit_test.hpp>

using namespace dev;
using namespace std;
using namespace dev::blockverifier;
using namespace dev::storage;
using namespace dev::precompiled;

namespace test_SimpleTablePrecompiled
{
class MockPrecompiledEngine : public dev::blockverifier::ExecutiveContext
{
public:
    virtual ~MockPrecompiledEngine() {}
};

class MockMemoryDB : public dev::storage::MemoryTable2
{
public:
    virtual ~MockMemoryDB() {}
};

struct TablePrecompiledFixture2
{
    TablePrecompiledFixture2()
    {
        context = std::make_shared<MockPrecompiledEngine>();
        tablePrecompiled = std::make_shared<precompiled::SimpleTablePrecompiled>();
        auto memStorage = std::make_shared<MemoryStorage2>();
        auto table = std::make_shared<MockMemoryDB>();
        table->setStateStorage(memStorage);
        TableInfo::Ptr info = std::make_shared<TableInfo>();
        info->fields.emplace_back(ID_FIELD);
        info->fields.emplace_back("name");
        info->fields.emplace_back(STATUS);
        okAddress = Address("0x420f853b49838bd3e9466c85a4cc3428c960dde2");
        info->authorizedAddress.emplace_back(okAddress);

        table->setTableInfo(info);
        table->setRecorder(
            [&](Table::Ptr, Change::Kind, string const&, vector<Change::Record>&) {});
        tablePrecompiled->setTable(table);
    }

    ~TablePrecompiledFixture2() {}

    precompiled::SimpleTablePrecompiled::Ptr tablePrecompiled;
    ExecutiveContext::Ptr context;
    BlockInfo blockInfo;
    Address okAddress;
    int addressCount = 0x10000;
};

BOOST_FIXTURE_TEST_SUITE(SimpleTablePrecompiled, TablePrecompiledFixture2)

BOOST_AUTO_TEST_CASE(getDB)
{
    tablePrecompiled->getTable();
}

BOOST_AUTO_TEST_CASE(hash)
{
    tablePrecompiled->hash();
}

BOOST_AUTO_TEST_CASE(clear)
{
    auto table = tablePrecompiled->getTable();
    table->clear();
}

BOOST_AUTO_TEST_CASE(toString)
{
    BOOST_CHECK_EQUAL(tablePrecompiled->toString(), "SimpleTable");
}

BOOST_AUTO_TEST_CASE(call_get_set)
{
    eth::ContractABI abi;
    bytes in = abi.abiIn("get(string)", std::string("name"));
    bytes out = tablePrecompiled->call(context, bytesConstRef(&in));
    bool status = true;
    Address entryAddress;
    abi.abiOut(bytesConstRef(&out), status, entryAddress);
    BOOST_TEST(status == false);
    auto entry = std::make_shared<storage::Entry>();
    entry->setField("name", "WangWu");
    auto entryPrecompiled = std::make_shared<EntryPrecompiled>();
    entryPrecompiled->setEntry(entry);
    auto entryAddress1 = context->registerPrecompiled(entryPrecompiled);
    bytes in1 = abi.abiIn("set(string,address)", std::string("name"), entryAddress1);
    bytes out1 = tablePrecompiled->call(context, bytesConstRef(&in1), okAddress);
    u256 num;
    abi.abiOut(bytesConstRef(&out1), num);
    BOOST_TEST(num == 1);
    bytes in2 = abi.abiIn("get(string)", std::string("name"));
    bytes out2 = tablePrecompiled->call(context, bytesConstRef(&in2));
    bool status2 = true;
    Address entryAddress2;
    abi.abiOut(bytesConstRef(&out2), status2, entryAddress2);
    BOOST_TEST(status2 == true);
    auto entryPrecompiled2 =
        std::dynamic_pointer_cast<EntryPrecompiled>(context->getPrecompiled(entryAddress2));
    entry = entryPrecompiled2->getEntry();
    BOOST_TEST(entry->getField(std::string("name")) == "WangWu");
}

BOOST_AUTO_TEST_CASE(call_set_permission_denied)
{
    eth::ContractABI abi;
    auto entry = std::make_shared<storage::Entry>();
    entry->setField("name", "WangWu");
    auto entryPrecompiled = std::make_shared<EntryPrecompiled>();
    entryPrecompiled->setEntry(entry);
    auto entryAddress1 = context->registerPrecompiled(entryPrecompiled);
    bytes in1 = abi.abiIn("set(string,address)", std::string("name"), entryAddress1);
    BOOST_CHECK_THROW(tablePrecompiled->call(context, bytesConstRef(&in1)), PrecompiledException);
    auto tooLongKey256 = std::string(
        "555555012345678901234567890123456789012345678901234567890123456789012345678901234567890123"
        "456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123"
        "4567890123456789012345678901234567890123456789012345678901234567890123456789");
    bytes in = abi.abiIn("set(string,address)", tooLongKey256, entryAddress1);
    BOOST_CHECK_THROW(tablePrecompiled->call(context, bytesConstRef(&in)), PrecompiledException);
}

BOOST_AUTO_TEST_CASE(call_newEntry)
{
    eth::ContractABI abi;
    bytes in = abi.abiIn("newEntry()");
    bytes out1 = tablePrecompiled->call(context, bytesConstRef(&in));
    Address address(++addressCount);
    bytes out2 = abi.abiIn("", address);
    BOOST_CHECK(out1 == out2);
}

BOOST_AUTO_TEST_SUITE_END()

}  // namespace test_SimpleTablePrecompiled
