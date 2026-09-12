// Copyright (c) 2026 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
//
// Regression tests for issue #18: every O transaction envelope written by
// ToScript() must parse back through its own FromScript() and be detected by
// GetOTxType(). Before the fix, ToScript() emitted the version/type bytes as
// small-integer OPCODES (OP_1..OP_16) while every parser required 1-byte data
// pushes, so 100% of O transactions were silently skipped.

#include <primitives/o_transactions.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <test/util/setup_common.h>

#include <boost/test/unit_test.hpp>

using namespace OTransactions;

BOOST_FIXTURE_TEST_SUITE(o_transactions_envelope_tests, BasicTestingSetup)

namespace {

CTransaction WrapInTx(const CScript& script)
{
    CMutableTransaction m;
    m.vin.emplace_back();
    m.vout.emplace_back(0, script);
    return CTransaction(m);
}

// FromScript for the signed payload types returns data.IsValid(), which demands
// non-empty signatures and a well-formed pubkey — fixtures must carry them.
CPubKey DummyPubKey()
{
    std::vector<unsigned char> v(33, 0x00);
    v[0] = 0x02; // compressed-key header => size()==33 => IsValid()
    v[32] = 0x01;
    return CPubKey(v.begin(), v.end());
}

std::vector<unsigned char> DummySig() { return {0xde, 0xad, 0xbe, 0xef}; }

} // namespace

BOOST_AUTO_TEST_CASE(user_verification_roundtrip)
{
    CUserVerificationData d;
    d.user_id = "user-1";
    d.identity_provider = "kyc_usa";
    d.country_code = "USA";
    d.birth_currency = "OUSD";
    d.verification_data = "{}";
    d.timestamp = 1757600000;
    d.o_pubkey = DummyPubKey();
    d.provider_sig = DummySig();
    d.user_sig = DummySig();

    const CScript s = d.ToScript();

    CUserVerificationData p;
    BOOST_CHECK(CUserVerificationData::FromScript(s, p));
    BOOST_CHECK_EQUAL(p.user_id, d.user_id);
    BOOST_CHECK_EQUAL(p.identity_provider, d.identity_provider);
    BOOST_CHECK_EQUAL(p.country_code, d.country_code);
    BOOST_CHECK_EQUAL(p.birth_currency, d.birth_currency);
    BOOST_CHECK_EQUAL(p.timestamp, d.timestamp);

    const CTransaction tx = WrapInTx(s);
    BOOST_CHECK(IsOTransaction(tx));
    const auto type = GetOTxType(tx);
    BOOST_REQUIRE(type.has_value());
    BOOST_CHECK(*type == OTxType::USER_VERIFY);
    BOOST_CHECK(ExtractUserVerification(tx).has_value());
}

BOOST_AUTO_TEST_CASE(water_price_roundtrip)
{
    CWaterPriceMeasurementData d;
    d.currency_code = "USD";
    d.price = 1250000; // 1.25
    d.measurer = DummyPubKey();
    d.timestamp = 1757600000;
    d.invite_id = uint256::ONE;
    d.proof_type = "url";
    d.proof_data = "https://example.org/receipt";
    d.signature = DummySig();

    const CScript s = d.ToScript();

    CWaterPriceMeasurementData p;
    BOOST_CHECK(CWaterPriceMeasurementData::FromScript(s, p));
    BOOST_CHECK_EQUAL(p.currency_code, d.currency_code);
    BOOST_CHECK_EQUAL(p.price, d.price);
    BOOST_CHECK(p.invite_id == d.invite_id);
    BOOST_CHECK_EQUAL(p.proof_data, d.proof_data);

    const auto type = GetOTxType(WrapInTx(s));
    BOOST_REQUIRE(type.has_value());
    BOOST_CHECK(*type == OTxType::WATER_PRICE);
}

BOOST_AUTO_TEST_CASE(exchange_rate_roundtrip)
{
    CExchangeRateMeasurementData d;
    d.from_currency = "OUSD";
    d.to_currency = "USD";
    d.exchange_rate = 1000000; // 1.0
    d.measurer = DummyPubKey();
    d.timestamp = 1757600000;
    d.invite_id = uint256::ONE;
    d.proof_data = "https://example.org/rate";
    d.signature = DummySig();

    const CScript s = d.ToScript();

    CExchangeRateMeasurementData p;
    BOOST_CHECK(CExchangeRateMeasurementData::FromScript(s, p));
    BOOST_CHECK_EQUAL(p.from_currency, d.from_currency);
    BOOST_CHECK_EQUAL(p.to_currency, d.to_currency);
    BOOST_CHECK_EQUAL(p.exchange_rate, d.exchange_rate);

    const auto type = GetOTxType(WrapInTx(s));
    BOOST_REQUIRE(type.has_value());
    BOOST_CHECK(*type == OTxType::EXCHANGE_RATE);
}

BOOST_AUTO_TEST_CASE(measurement_validation_roundtrip)
{
    CMeasurementValidationData d;
    d.measurement_id = uint256::ONE;
    d.measurement_type = OTxType::WATER_PRICE;
    d.validator = DummyPubKey();
    d.validation_result = true;
    d.timestamp = 1757600000;
    d.validation_notes = "looks right";
    d.signature = DummySig();

    const CScript s = d.ToScript();

    CMeasurementValidationData p;
    BOOST_CHECK(CMeasurementValidationData::FromScript(s, p));
    BOOST_CHECK(p.measurement_id == d.measurement_id);
    BOOST_CHECK(p.measurement_type == d.measurement_type);
    BOOST_CHECK_EQUAL(p.validation_result, d.validation_result);
    BOOST_CHECK_EQUAL(p.validation_notes, d.validation_notes);

    const auto type = GetOTxType(WrapInTx(s));
    BOOST_REQUIRE(type.has_value());
    BOOST_CHECK(*type == OTxType::MEASUREMENT_VALIDATION);
}

BOOST_AUTO_TEST_CASE(measurement_invite_roundtrip)
{
    CMeasurementInviteData d;
    d.invite_id = uint256::ONE;
    d.invited_user = DummyPubKey();
    d.measurement_type = 0x02;
    d.currency_code = "USD";
    d.created_at = 1757600000;
    d.expires_at = 1757700000;
    d.block_height = 42;

    const CScript s = d.ToScript();

    CMeasurementInviteData p;
    BOOST_CHECK(CMeasurementInviteData::FromScript(s, p));
    BOOST_CHECK(p.invite_id == d.invite_id);
    BOOST_CHECK_EQUAL(p.measurement_type, d.measurement_type);
    BOOST_CHECK_EQUAL(p.currency_code, d.currency_code);
    BOOST_CHECK_EQUAL(p.created_at, d.created_at);
    BOOST_CHECK_EQUAL(p.expires_at, d.expires_at);
    BOOST_CHECK_EQUAL(p.block_height, d.block_height);

    const auto type = GetOTxType(WrapInTx(s));
    BOOST_REQUIRE(type.has_value());
    BOOST_CHECK(*type == OTxType::MEASUREMENT_INVITE);
}

BOOST_AUTO_TEST_CASE(cross_type_parse_rejected)
{
    CMeasurementInviteData d;
    d.invite_id = uint256::ONE;
    d.invited_user = DummyPubKey();
    d.measurement_type = 0x02;
    d.created_at = 1757600000;
    d.expires_at = 1757700000;
    d.block_height = 1;

    CMeasurementValidationData wrong;
    BOOST_CHECK(!CMeasurementValidationData::FromScript(d.ToScript(), wrong));
}

BOOST_AUTO_TEST_CASE(truncated_envelope_rejected)
{
    CScript s;
    s << OP_RETURN;
    s << std::vector<unsigned char>(O_TX_PREFIX.begin(), O_TX_PREFIX.end());

    CUserVerificationData p;
    BOOST_CHECK(!CUserVerificationData::FromScript(s, p));
    BOOST_CHECK(!GetOTxType(WrapInTx(s)).has_value());
}

BOOST_AUTO_TEST_CASE(non_o_opreturn_ignored)
{
    CScript s;
    s << OP_RETURN;
    s << std::vector<unsigned char>{'X', 'Y', 'Z', 'W'};

    BOOST_CHECK(!IsOTransaction(WrapInTx(s)));
    BOOST_CHECK(!GetOTxType(WrapInTx(s)).has_value());
}

BOOST_AUTO_TEST_CASE(legacy_opcode_form_envelope_rejected)
{
    // Reproduces the exact pre-fix writer bug (#18): version/type emitted via
    // `script << int`, which for 1..16 yields OP_1..OP_16 OPCODES instead of
    // 1-byte data pushes. Such envelopes must stay unparseable — they never
    // parsed on any deployed node, so there is no compatibility surface.
    CScript s;
    s << OP_RETURN;
    s << std::vector<unsigned char>(O_TX_PREFIX.begin(), O_TX_PREFIX.end());
    s << O_TX_VERSION;                                        // emits OP_1 (opcode!)
    s << static_cast<uint8_t>(OTxType::USER_VERIFY);          // emits OP_1 (opcode!)
    s << std::vector<unsigned char>{0x00, 0x01, 0x02};        // dummy payload

    CUserVerificationData p;
    BOOST_CHECK(!CUserVerificationData::FromScript(s, p));
    BOOST_CHECK(IsOTransaction(WrapInTx(s)));                 // prefix still matches...
    BOOST_CHECK(!GetOTxType(WrapInTx(s)).has_value());        // ...but type never parses
}

BOOST_AUTO_TEST_SUITE_END()
