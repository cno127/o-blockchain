// Copyright (c) 2026 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// O Layer 2 prototype, first part: the payment state (o_layer2/payment_state.h).
// No node, no chain, no fixture: the executor is plain logic over bytes.

#include <o_layer2/payment_state.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

using namespace o_layer2;

namespace {

// Test numbers only, not the ids of the currency registry.
constexpr CurrencyId USD = 1, EUR = 2, JPY = 3;

// Value of one smallest unit in the common reference: USD 100, EUR 110, JPY 2/3.
std::vector<Rate> TestRates() { return {{USD, 100, 1}, {EUR, 110, 1}, {JPY, 2, 3}}; }

AccountId Acct(uint8_t n)
{
    AccountId a{};
    a[0] = n;
    a[31] = 0xA0;
    return a;
}

std::string Hex(const Hash256& h)
{
    static const char* digits = "0123456789abcdef";
    std::string s;
    for (uint8_t b : h) {
        s.push_back(digits[b >> 4]);
        s.push_back(digits[b & 15]);
    }
    return s;
}

Op Deposit(const AccountId& a, CurrencyId c, Amount amount)
{
    Op op;
    op.type = OpType::DEPOSIT;
    op.from = a;
    op.currency = c;
    op.amount = amount;
    return op;
}

Op Withdraw(const AccountId& a, CurrencyId c, Amount amount, uint64_t nonce)
{
    Op op = Deposit(a, c, amount);
    op.type = OpType::WITHDRAW;
    op.nonce = nonce;
    return op;
}

Op Pay(const AccountId& from, const AccountId& to, CurrencyId c, Amount amount, uint64_t nonce)
{
    Op op = Withdraw(from, c, amount, nonce);
    op.type = OpType::PAY;
    op.to = to;
    return op;
}

Op PayConvert(const AccountId& from, const AccountId& to, const AccountId& converter, CurrencyId c, Amount amount, CurrencyId recv_c, Amount recv_amount, uint64_t nonce)
{
    Op op = Pay(from, to, c, amount, nonce);
    op.type = OpType::PAY_CONVERT;
    op.converter = converter;
    op.recv_currency = recv_c;
    op.recv_amount = recv_amount;
    return op;
}

Op SetConverter(const AccountId& a, uint8_t flag, uint64_t nonce)
{
    Op op;
    op.type = OpType::SET_CONVERTER;
    op.from = a;
    op.flag = flag;
    op.nonce = nonce;
    return op;
}

// The next batch for this state.
std::vector<uint8_t> NextBatch(const PaymentState& s, const std::vector<Op>& ops)
{
    Batch b;
    b.number = s.BatchCount() + 1;
    b.prev_root = s.Root();
    b.rates = TestRates();
    b.ops = ops;
    return SerializeBatch(b);
}

struct Lcg {
    uint64_t s;
    uint32_t Next()
    {
        s = s * 6364136223846793005ULL + 1442695040888963407ULL;
        return static_cast<uint32_t>(s >> 33);
    }
    uint32_t Below(uint32_t n) { return Next() % n; }
};

struct RunTotals {
    std::map<OpType, int> ok;
    std::map<OpStatus, int> statuses;
    std::map<CurrencyId, Amount> supply; //!< deposits minus withdrawals of the operations that were applied
};

// A run of mixed batches, many operations wrong on purpose. Applies each batch to `s` and returns the bytes.
std::vector<std::vector<uint8_t>> BuildRun(PaymentState& s, uint64_t seed, int n_batches, int n_ops, RunTotals& totals, bool& all_conserved)
{
    Lcg rng{seed};
    const CurrencyId currencies[3] = {USD, EUR, JPY};
    const std::vector<Rate> rates = TestRates();
    std::vector<std::vector<uint8_t>> out;
    all_conserved = true;

    for (int b = 0; b < n_batches; ++b) {
        std::vector<Op> ops;
        std::map<AccountId, uint64_t> used; // operations of the account earlier in this batch
        auto nonce = [&](const AccountId& a) { return s.Nonce(a) + used[a]++; };
        if (b == 0) {
            // accounts 1 and 2 convert, everybody starts with some of each currency
            ops.push_back(SetConverter(Acct(1), 1, nonce(Acct(1))));
            ops.push_back(SetConverter(Acct(2), 1, nonce(Acct(2))));
            for (uint8_t a = 1; a <= 6; ++a) {
                for (CurrencyId c : currencies) ops.push_back(Deposit(Acct(a), c, 1000000));
            }
        }
        for (int i = 0; i < n_ops; ++i) {
            const AccountId from = Acct(static_cast<uint8_t>(1 + rng.Below(6)));
            const AccountId to = Acct(static_cast<uint8_t>(1 + rng.Below(7))); // 7 = an account that starts empty
            const CurrencyId c = currencies[rng.Below(3)];
            const Amount amount = static_cast<Amount>(rng.Below(40) == 0 ? 0 : rng.Below(30) == 0 ? 50000000 : 1 + rng.Below(5000));
            const uint32_t kind = rng.Below(20);
            if (kind == 0) {
                ops.push_back(Deposit(from, c, amount));
            } else if (kind == 1) {
                ops.push_back(Withdraw(from, c, amount, nonce(from)));
            } else if (kind == 2) {
                ops.push_back(SetConverter(from, static_cast<uint8_t>(rng.Below(4) != 0), nonce(from)));
            } else if (kind < 11) {
                ops.push_back(Pay(from, to, c, amount, nonce(from)));
            } else {
                const CurrencyId recv_c = currencies[rng.Below(3)];
                const AccountId converter = Acct(static_cast<uint8_t>(1 + rng.Below(3)));
                Amount recv = 0;
                if (c != recv_c) ConvertFloor(amount, rates[c - 1], rates[recv_c - 1], recv);
                if (rng.Below(15) == 0) ++recv; // a payee asked for one unit too many
                ops.push_back(PayConvert(from, to, converter, c, amount, recv_c, recv, nonce(from)));
            }
            if (ops.back().type != OpType::DEPOSIT && rng.Below(25) == 0) ops.back().nonce += 1; // a wrong nonce
        }

        const std::vector<uint8_t> bytes = NextBatch(s, ops);
        const BatchResult r = s.ApplyBatch(bytes);
        if (r.error != BatchError::NONE || r.statuses.size() != ops.size()) all_conserved = false;
        for (size_t i = 0; i < r.statuses.size(); ++i) {
            ++totals.statuses[r.statuses[i]];
            if (r.statuses[i] != OpStatus::OK) continue;
            ++totals.ok[ops[i].type];
            if (ops[i].type == OpType::DEPOSIT) totals.supply[ops[i].currency] += ops[i].amount;
            if (ops[i].type == OpType::WITHDRAW) totals.supply[ops[i].currency] -= ops[i].amount;
        }
        if (!s.CheckConservation()) all_conserved = false;
        for (CurrencyId c : currencies) {
            if (s.Supply(c) != totals.supply[c]) all_conserved = false;
        }
        out.push_back(bytes);
    }
    return out;
}

} // namespace

BOOST_AUTO_TEST_SUITE(o_layer2_tests)

BOOST_AUTO_TEST_CASE(muldiv_floor)
{
    uint64_t q = 99;
    BOOST_CHECK(MulDivFloor(0, 5, 3, q) && q == 0);
    BOOST_CHECK(MulDivFloor(10, 10, 3, q) && q == 33);
    BOOST_CHECK(MulDivFloor(UINT64_MAX, UINT64_MAX, UINT64_MAX, q) && q == UINT64_MAX);
    BOOST_CHECK(MulDivFloor(1ULL << 63, 4, 8, q) && q == (1ULL << 62));
    BOOST_CHECK(MulDivFloor(UINT64_MAX, 3, 4, q) && q == 13835058055282163711ULL);
    BOOST_CHECK(!MulDivFloor(UINT64_MAX, 2, 1, q)); // does not fit
    BOOST_CHECK(!MulDivFloor(1, 1, 0, q));

#ifdef __SIZEOF_INT128__
    // against the compiler's own 128-bit integers, where there are some
    Lcg rng{20261119};
    int compared = 0, wrong = 0;
    for (int i = 0; i < 20000; ++i) {
        const uint64_t a = (static_cast<uint64_t>(rng.Next()) << 32 | rng.Next()) >> rng.Below(64);
        const uint64_t b = (static_cast<uint64_t>(rng.Next()) << 32 | rng.Next()) >> rng.Below(64);
        const uint64_t d = (static_cast<uint64_t>(rng.Next()) << 32 | rng.Next()) >> rng.Below(64);
        if (d == 0) continue;
        const unsigned __int128 exact = static_cast<unsigned __int128>(a) * b / d;
        const bool fits = exact <= UINT64_MAX;
        uint64_t got = 0;
        const bool ok = MulDivFloor(a, b, d, got);
        ++compared;
        if (ok != fits || (ok && got != static_cast<uint64_t>(exact))) ++wrong;
    }
    BOOST_CHECK(compared > 19000);
    BOOST_CHECK_EQUAL(wrong, 0);
#endif

    const std::vector<Rate> rates = TestRates();
    Amount out = 0;
    BOOST_CHECK(ConvertFloor(1000, rates[0], rates[1], out) && out == 909);    // USD -> EUR, 909.09 down
    BOOST_CHECK(ConvertFloor(1000, rates[0], rates[2], out) && out == 150000); // USD -> JPY
    BOOST_CHECK(ConvertFloor(1000, rates[2], rates[0], out) && out == 6);      // JPY -> USD, 6.67 down
    BOOST_CHECK(ConvertFloor(1, rates[2], rates[0], out) && out == 0);         // less than one unit
    BOOST_CHECK(!ConvertFloor(0, rates[0], rates[1], out));
    BOOST_CHECK(!ConvertFloor(INT64_MAX, rates[0], rates[2], out)); // would pass the largest amount
}

BOOST_AUTO_TEST_CASE(batch_has_one_byte_form)
{
    Batch b;
    b.number = 7;
    b.prev_root[5] = 0x44;
    b.rates = TestRates();
    b.ops = {Deposit(Acct(1), USD, 500), Pay(Acct(1), Acct(2), USD, 120, 0), PayConvert(Acct(1), Acct(2), Acct(3), USD, 100, EUR, 90, 1),
             Withdraw(Acct(2), USD, 20, 0), SetConverter(Acct(3), 1, 0)};
    const std::vector<uint8_t> bytes = SerializeBatch(b);
    BOOST_CHECK_EQUAL(bytes.size(), 49U + 3 * 12 + 4 + 5 * 130);

    Batch parsed;
    BOOST_REQUIRE(ParseBatch(bytes, parsed));
    BOOST_CHECK(SerializeBatch(parsed) == bytes);
    BOOST_CHECK(parsed.number == 7 && parsed.ops.size() == 5 && parsed.ops[2].recv_amount == 90 && parsed.rates[2].den == 3);

    auto refused = [](std::vector<uint8_t> v) {
        Batch x;
        return !ParseBatch(v, x);
    };
    std::vector<uint8_t> v = bytes;
    v.pop_back();
    BOOST_CHECK(refused(v)); // one byte short
    v = bytes;
    v.push_back(0);
    BOOST_CHECK(refused(v)); // one byte after the end
    v = bytes;
    v[0] = 'X';
    BOOST_CHECK(refused(v)); // not a batch
    v = bytes;
    v[4] = 2;
    BOOST_CHECK(refused(v)); // another version
    BOOST_CHECK(refused({}));

    Batch c = b;
    std::swap(c.rates[0], c.rates[1]);
    BOOST_CHECK(refused(SerializeBatch(c))); // rates not in order
    c = b;
    c.rates[1].currency = USD;
    BOOST_CHECK(refused(SerializeBatch(c))); // a currency twice
    c = b;
    c.rates[1].den = 0;
    BOOST_CHECK(refused(SerializeBatch(c)));
    c = b;
    c.ops[1].flag = 1;
    BOOST_CHECK(refused(SerializeBatch(c))); // a field PAY does not use
    c = b;
    c.ops[0].nonce = 1;
    BOOST_CHECK(refused(SerializeBatch(c))); // a field DEPOSIT does not use
    c = b;
    c.ops[4].flag = 2;
    BOOST_CHECK(refused(SerializeBatch(c)));
    c = b;
    c.ops[3].type = static_cast<OpType>(9);
    BOOST_CHECK(refused(SerializeBatch(c))); // unknown operation
}

BOOST_AUTO_TEST_CASE(pay_and_nonce)
{
    PaymentState s;
    const AccountId alice = Acct(1), bob = Acct(2);
    BatchResult r = s.ApplyBatch(NextBatch(s, {
        Deposit(alice, USD, 1000),
        Pay(alice, bob, USD, 300, 0),
        Pay(alice, bob, USD, 300, 0),  // the same operation again
        Pay(alice, bob, USD, 800, 1),  // more than she holds
        Pay(alice, alice, USD, 10, 1), // to herself
        Pay(alice, bob, USD, 0, 1),
        Pay(alice, bob, EUR, 5, 1), // a currency she does not hold
        Pay(bob, alice, USD, 100, 0),
        Withdraw(bob, USD, 150, 1),
        Withdraw(bob, USD, 150, 2), // 50 left
    }));
    BOOST_REQUIRE(r.error == BatchError::NONE);
    const std::vector<OpStatus> expected = {OpStatus::OK, OpStatus::OK, OpStatus::BAD_NONCE, OpStatus::INSUFFICIENT_FUNDS, OpStatus::BAD_PARTIES,
                                            OpStatus::BAD_AMOUNT, OpStatus::INSUFFICIENT_FUNDS, OpStatus::OK, OpStatus::OK, OpStatus::INSUFFICIENT_FUNDS};
    BOOST_CHECK(r.statuses == expected);
    BOOST_CHECK_EQUAL(s.Balance(alice, USD), 800);
    BOOST_CHECK_EQUAL(s.Balance(bob, USD), 50);
    BOOST_CHECK_EQUAL(s.Nonce(alice), 1U);
    BOOST_CHECK_EQUAL(s.Nonce(bob), 2U);
    BOOST_CHECK_EQUAL(s.Supply(USD), 850);
    BOOST_CHECK(s.CheckConservation());
    BOOST_CHECK(r.root == s.Root());

    // the largest amount: a balance, and the supply of a currency, never pass it
    const AccountId carol = Acct(3);
    r = s.ApplyBatch(NextBatch(s, {
        Deposit(carol, JPY, INT64_MAX),
        Deposit(carol, JPY, 1),
        Deposit(alice, JPY, 1),
    }));
    const std::vector<OpStatus> expected_big = {OpStatus::OK, OpStatus::AMOUNT_OVERFLOW, OpStatus::AMOUNT_OVERFLOW};
    BOOST_CHECK(r.statuses == expected_big);
    BOOST_CHECK(s.Balance(carol, JPY) == INT64_MAX && s.Balance(alice, JPY) == 0 && s.Supply(JPY) == INT64_MAX);
    BOOST_CHECK(s.CheckConservation());
}

BOOST_AUTO_TEST_CASE(pay_with_conversion_is_all_or_nothing)
{
    PaymentState s;
    const AccountId payer = Acct(1), merchant = Acct(2), converter = Acct(3), other = Acct(4);
    BatchResult r = s.ApplyBatch(NextBatch(s, {
        Deposit(payer, USD, 5000),
        Deposit(converter, EUR, 2000),
        Deposit(other, EUR, 2000),
        SetConverter(converter, 1, 0),
        PayConvert(payer, merchant, converter, USD, 1000, EUR, 909, 0),
    }));
    BOOST_REQUIRE(r.error == BatchError::NONE);
    BOOST_CHECK(r.statuses.back() == OpStatus::OK);
    // the four legs
    BOOST_CHECK_EQUAL(s.Balance(payer, USD), 4000);
    BOOST_CHECK_EQUAL(s.Balance(converter, USD), 1000);
    BOOST_CHECK_EQUAL(s.Balance(converter, EUR), 1091);
    BOOST_CHECK_EQUAL(s.Balance(merchant, EUR), 909);
    BOOST_CHECK_EQUAL(s.Balance(merchant, USD), 0);
    // no unit of either currency created or destroyed
    BOOST_CHECK_EQUAL(s.Supply(USD), 5000);
    BOOST_CHECK_EQUAL(s.Supply(EUR), 4000);
    BOOST_CHECK(s.CheckConservation());

    struct Case {
        Op op;
        OpStatus expected;
    };
    const std::vector<Case> cases = {
        {PayConvert(payer, merchant, converter, USD, 1000, EUR, 910, 1), OpStatus::RATE_MISMATCH},
        {PayConvert(payer, merchant, converter, USD, 1000, EUR, 908, 1), OpStatus::RATE_MISMATCH}, // less is refused too: one result only
        {PayConvert(payer, merchant, converter, USD, 1000, 9, 909, 1), OpStatus::NO_RATE},
        {PayConvert(payer, merchant, other, USD, 1000, EUR, 909, 1), OpStatus::NOT_CONVERTER}, // holds EUR, never agreed
        {PayConvert(payer, merchant, Acct(9), USD, 1000, EUR, 909, 1), OpStatus::NOT_CONVERTER},
        {PayConvert(payer, merchant, converter, USD, 4001, EUR, 3637, 1), OpStatus::INSUFFICIENT_FUNDS},
        {PayConvert(payer, merchant, converter, USD, 4000, EUR, 3636, 1), OpStatus::CONVERTER_INSUFFICIENT},
        {PayConvert(payer, converter, converter, USD, 1000, EUR, 909, 1), OpStatus::BAD_PARTIES},
        {PayConvert(payer, merchant, converter, USD, 1000, USD, 1000, 1), OpStatus::BAD_PARTIES},
        {PayConvert(payer, merchant, converter, USD, 1000, EUR, 909, 0), OpStatus::BAD_NONCE},
        {PayConvert(payer, merchant, converter, USD, 0, EUR, 909, 1), OpStatus::BAD_AMOUNT},
        {PayConvert(payer, merchant, converter, JPY, 1, USD, 0, 1), OpStatus::BAD_AMOUNT}, // converts to less than one unit
    };
    for (const Case& c : cases) {
        const BatchResult rc = s.ApplyBatch(NextBatch(s, {c.op}));
        BOOST_REQUIRE(rc.error == BatchError::NONE);
        BOOST_CHECK(rc.statuses.size() == 1 && rc.statuses[0] == c.expected);
        // nothing moved, on any leg
        BOOST_CHECK(s.Balance(payer, USD) == 4000 && s.Balance(converter, USD) == 1000 && s.Balance(converter, EUR) == 1091);
        BOOST_CHECK(s.Balance(merchant, EUR) == 909 && s.Balance(merchant, USD) == 0 && s.Balance(other, EUR) == 2000);
        BOOST_CHECK(s.Nonce(payer) == 1 && s.Nonce(converter) == 1 && s.Nonce(merchant) == 0);
        BOOST_CHECK(s.CheckConservation());
    }

    // a wallet converts its own money (payer = payee), then the converter stops converting
    r = s.ApplyBatch(NextBatch(s, {
        PayConvert(payer, payer, converter, USD, 110, EUR, 100, 1),
        SetConverter(converter, 0, 1),
        PayConvert(payer, merchant, converter, USD, 110, EUR, 100, 2),
    }));
    const std::vector<OpStatus> expected = {OpStatus::OK, OpStatus::OK, OpStatus::NOT_CONVERTER};
    BOOST_CHECK(r.statuses == expected);
    BOOST_CHECK(s.Balance(payer, USD) == 3890 && s.Balance(payer, EUR) == 100 && s.Balance(converter, EUR) == 991);
    BOOST_CHECK(s.CheckConservation());
}

BOOST_AUTO_TEST_CASE(conservation_over_a_mixed_run)
{
    PaymentState s;
    RunTotals totals;
    bool conserved = false;
    BuildRun(s, 7, 150, 12, totals, conserved);
    BOOST_CHECK(conserved); // after every batch: balances add up to the supply, supply = deposits - withdrawals
    // the run really went through each path
    BOOST_CHECK(totals.ok[OpType::DEPOSIT] > 18 && totals.ok[OpType::WITHDRAW] > 0 && totals.ok[OpType::SET_CONVERTER] > 2);
    BOOST_CHECK(totals.ok[OpType::PAY] > 100 && totals.ok[OpType::PAY_CONVERT] > 50);
    for (OpStatus st : {OpStatus::BAD_AMOUNT, OpStatus::BAD_NONCE, OpStatus::BAD_PARTIES, OpStatus::INSUFFICIENT_FUNDS, OpStatus::RATE_MISMATCH, OpStatus::NOT_CONVERTER,
                        OpStatus::CONVERTER_INSUFFICIENT}) {
        BOOST_CHECK(totals.statuses[st] > 0);
    }
}

BOOST_AUTO_TEST_CASE(replay_gives_the_same_root)
{
    // one instance builds the run, three others only get the bytes
    PaymentState builder;
    RunTotals totals;
    bool conserved = false;
    const std::vector<std::vector<uint8_t>> run = BuildRun(builder, 11, 60, 12, totals, conserved);
    BOOST_REQUIRE(conserved && run.size() == 60);

    PaymentState node[3];
    PaymentState check;
    for (const std::vector<uint8_t>& bytes : run) {
        const BatchResult expected = check.ApplyBatch(bytes);
        BOOST_REQUIRE(expected.error == BatchError::NONE);
        for (PaymentState& n : node) {
            const BatchResult r = n.ApplyBatch(bytes);
            BOOST_CHECK(r.error == BatchError::NONE && r.root == expected.root && r.statuses == expected.statuses && r.batch_hash == expected.batch_hash);
        }
    }
    for (PaymentState& n : node) BOOST_CHECK(n.Root() == builder.Root() && n.BatchCount() == 60);

    const Hash256 empty_root = PaymentState().Root();

    // a batch out of turn, or twice, is refused and changes nothing
    PaymentState late;
    BOOST_CHECK(late.ApplyBatch(run[1]).error == BatchError::WRONG_NUMBER);
    BOOST_CHECK(late.Root() == empty_root);
    BOOST_CHECK(late.ApplyBatch(run[0]).error == BatchError::NONE);
    const Hash256 after_first = late.Root();
    BOOST_CHECK(late.ApplyBatch(run[0]).error == BatchError::WRONG_NUMBER);
    BOOST_CHECK(late.Root() == after_first);

    // one byte changed in the first batch (the amount of the first deposit): another state, and the honest second batch
    // does not apply to it
    std::vector<uint8_t> forged = run[0];
    const size_t first_deposit_amount = 49 + 3 * 12 + 4 + 2 * 130 + 1 + 96 + 4;
    forged[first_deposit_amount] ^= 1;
    PaymentState fooled;
    const BatchResult rf = fooled.ApplyBatch(forged);
    BOOST_CHECK(rf.error == BatchError::NONE);
    BOOST_CHECK(rf.root != after_first);
    BOOST_CHECK(fooled.ApplyBatch(run[1]).error == BatchError::WRONG_PREV_ROOT);
    BOOST_CHECK(fooled.Root() == rf.root);

    // garbage is refused and changes nothing
    BOOST_CHECK(fooled.ApplyBatch({1, 2, 3}).error == BatchError::MALFORMED);
    BOOST_CHECK(fooled.Root() == rf.root);
}

// Fixed results: the same values must come out of every compiler, standard library and machine.
BOOST_AUTO_TEST_CASE(fixed_roots)
{
    BOOST_CHECK_EQUAL(Hex(PaymentState().Root()), "fa71019a61905967c39c5f50b2d9342d0001123d0c7e173edf53201af8899e9c");

    PaymentState s;
    RunTotals totals;
    bool conserved = false;
    BuildRun(s, 11, 60, 12, totals, conserved);
    BOOST_CHECK(conserved);
    BOOST_CHECK_EQUAL(Hex(s.Root()), "1f9d5c8bfac83bb7c9264387fe6087e0a1c50d28a2c10ac0930db021357aff51");
    BOOST_CHECK_EQUAL(totals.statuses[OpStatus::OK], 317);
}

BOOST_AUTO_TEST_SUITE_END()
