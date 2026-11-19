// Copyright (c) 2026 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <o_layer2/payment_state.h>

#include <crypto/sha256.h>

#include <cstddef>
#include <limits>
#include <utility>

namespace o_layer2 {

namespace {

constexpr uint8_t BATCH_MAGIC[4] = {'O', 'L', '2', 'B'};
constexpr uint8_t STATE_MAGIC[4] = {'O', 'L', '2', 'S'};
constexpr uint8_t FORMAT_VERSION = 1;
constexpr size_t OP_SIZE = 1 + 32 + 32 + 32 + 4 + 8 + 4 + 8 + 8 + 1; // 130
constexpr size_t RATE_SIZE = 4 + 4 + 4;
constexpr size_t BATCH_HEADER_SIZE = 4 + 1 + 8 + 32 + 4; // magic, version, number, prev_root, rate count
constexpr Amount MAX_AMOUNT = std::numeric_limits<Amount>::max();

// Little-endian, fixed width: the byte form never depends on the machine.
void Put8(std::vector<uint8_t>& v, uint8_t x) { v.push_back(x); }
void Put32(std::vector<uint8_t>& v, uint32_t x)
{
    for (int i = 0; i < 4; ++i) v.push_back(static_cast<uint8_t>(x >> (8 * i)));
}
void Put64(std::vector<uint8_t>& v, uint64_t x)
{
    for (int i = 0; i < 8; ++i) v.push_back(static_cast<uint8_t>(x >> (8 * i)));
}
void PutBytes(std::vector<uint8_t>& v, const std::array<uint8_t, 32>& a) { v.insert(v.end(), a.begin(), a.end()); }

class Reader
{
public:
    explicit Reader(const std::vector<uint8_t>& v) : m_v(v) {}
    size_t Left() const { return m_v.size() - m_pos; }
    uint8_t Get8() { return m_v[m_pos++]; }
    uint32_t Get32()
    {
        uint32_t x = 0;
        for (int i = 0; i < 4; ++i) x |= static_cast<uint32_t>(m_v[m_pos++]) << (8 * i);
        return x;
    }
    uint64_t Get64()
    {
        uint64_t x = 0;
        for (int i = 0; i < 8; ++i) x |= static_cast<uint64_t>(m_v[m_pos++]) << (8 * i);
        return x;
    }
    std::array<uint8_t, 32> GetBytes()
    {
        std::array<uint8_t, 32> a;
        for (size_t i = 0; i < 32; ++i) a[i] = m_v[m_pos++];
        return a;
    }

private:
    const std::vector<uint8_t>& m_v;
    size_t m_pos{0};
};

bool IsZero(const std::array<uint8_t, 32>& a)
{
    for (uint8_t b : a) {
        if (b != 0) return false;
    }
    return true;
}

// The fields a type does not use are zero, and the type and the flag are known values.
bool IsCanonical(const Op& op)
{
    const bool no_to = IsZero(op.to);
    const bool no_converter = IsZero(op.converter);
    const bool no_recv = op.recv_currency == 0 && op.recv_amount == 0;
    switch (op.type) {
    case OpType::DEPOSIT:
        return no_to && no_converter && no_recv && op.nonce == 0 && op.flag == 0;
    case OpType::WITHDRAW:
        return no_to && no_converter && no_recv && op.flag == 0;
    case OpType::PAY:
        return no_converter && no_recv && op.flag == 0;
    case OpType::PAY_CONVERT:
        return op.flag == 0;
    case OpType::SET_CONVERTER:
        return no_to && no_converter && no_recv && op.currency == 0 && op.amount == 0 && op.flag <= 1;
    }
    return false; // unknown type
}

Hash256 Sha256(const std::vector<uint8_t>& bytes)
{
    Hash256 out;
    CSHA256().Write(bytes.data(), bytes.size()).Finalize(out.data());
    return out;
}

} // namespace

std::vector<uint8_t> SerializeBatch(const Batch& batch)
{
    std::vector<uint8_t> v;
    v.reserve(BATCH_HEADER_SIZE + batch.rates.size() * RATE_SIZE + 4 + batch.ops.size() * OP_SIZE);
    v.insert(v.end(), BATCH_MAGIC, BATCH_MAGIC + 4);
    Put8(v, FORMAT_VERSION);
    Put64(v, batch.number);
    PutBytes(v, batch.prev_root);
    Put32(v, static_cast<uint32_t>(batch.rates.size()));
    for (const Rate& r : batch.rates) {
        Put32(v, r.currency);
        Put32(v, r.num);
        Put32(v, r.den);
    }
    Put32(v, static_cast<uint32_t>(batch.ops.size()));
    for (const Op& op : batch.ops) {
        Put8(v, static_cast<uint8_t>(op.type));
        PutBytes(v, op.from);
        PutBytes(v, op.to);
        PutBytes(v, op.converter);
        Put32(v, op.currency);
        Put64(v, static_cast<uint64_t>(op.amount));
        Put32(v, op.recv_currency);
        Put64(v, static_cast<uint64_t>(op.recv_amount));
        Put64(v, op.nonce);
        Put8(v, op.flag);
    }
    return v;
}

bool ParseBatch(const std::vector<uint8_t>& bytes, Batch& out)
{
    if (bytes.size() < BATCH_HEADER_SIZE + 4) return false;
    Reader r(bytes);
    for (uint8_t m : BATCH_MAGIC) {
        if (r.Get8() != m) return false;
    }
    if (r.Get8() != FORMAT_VERSION) return false;

    Batch b;
    b.number = r.Get64();
    b.prev_root = r.GetBytes();

    const uint64_t n_rates = r.Get32();
    if (r.Left() < n_rates * RATE_SIZE + 4) return false;
    b.rates.reserve(n_rates);
    for (uint64_t i = 0; i < n_rates; ++i) {
        Rate rate;
        rate.currency = r.Get32();
        rate.num = r.Get32();
        rate.den = r.Get32();
        if (rate.num == 0 || rate.den == 0) return false;
        if (!b.rates.empty() && b.rates.back().currency >= rate.currency) return false; // strictly ascending
        b.rates.push_back(rate);
    }

    const uint64_t n_ops = r.Get32();
    if (r.Left() != n_ops * OP_SIZE) return false; // exact length: nothing missing, nothing after
    b.ops.reserve(n_ops);
    for (uint64_t i = 0; i < n_ops; ++i) {
        Op op;
        op.type = static_cast<OpType>(r.Get8());
        op.from = r.GetBytes();
        op.to = r.GetBytes();
        op.converter = r.GetBytes();
        op.currency = r.Get32();
        op.amount = static_cast<Amount>(r.Get64());
        op.recv_currency = r.Get32();
        op.recv_amount = static_cast<Amount>(r.Get64());
        op.nonce = r.Get64();
        op.flag = r.Get8();
        if (!IsCanonical(op)) return false;
        b.ops.push_back(op);
    }

    out = std::move(b);
    return true;
}

bool MulDivFloor(uint64_t a, uint64_t b, uint64_t d, uint64_t& out)
{
    if (d == 0) return false;

    // a * b as (hi, lo), from 32-bit halves
    const uint64_t a_lo = a & 0xffffffffULL, a_hi = a >> 32;
    const uint64_t b_lo = b & 0xffffffffULL, b_hi = b >> 32;
    const uint64_t p0 = a_lo * b_lo, p1 = a_lo * b_hi, p2 = a_hi * b_lo, p3 = a_hi * b_hi;
    const uint64_t mid = (p0 >> 32) + (p1 & 0xffffffffULL) + (p2 & 0xffffffffULL);
    const uint64_t lo = (p0 & 0xffffffffULL) | (mid << 32);
    const uint64_t hi = p3 + (p1 >> 32) + (p2 >> 32) + (mid >> 32);

    if (hi >= d) return false; // the quotient would need more than 64 bits

    // long division of (hi, lo) by d, one bit at a time; the remainder stays below d
    uint64_t rem = hi;
    uint64_t q = 0;
    for (int i = 63; i >= 0; --i) {
        const bool carry = (rem >> 63) != 0;
        rem = (rem << 1) | ((lo >> i) & 1);
        q <<= 1;
        if (carry || rem >= d) {
            rem -= d;
            q |= 1;
        }
    }
    out = q;
    return true;
}

bool ConvertFloor(Amount amount, const Rate& from, const Rate& to, Amount& out)
{
    if (amount <= 0 || from.num == 0 || from.den == 0 || to.num == 0 || to.den == 0) return false;
    // 32-bit terms: both products fit in 64 bits
    const uint64_t mul = static_cast<uint64_t>(from.num) * to.den;
    const uint64_t div = static_cast<uint64_t>(from.den) * to.num;
    uint64_t q = 0;
    if (!MulDivFloor(static_cast<uint64_t>(amount), mul, div, q)) return false;
    if (q > static_cast<uint64_t>(MAX_AMOUNT)) return false;
    out = static_cast<Amount>(q);
    return true;
}

const PaymentState::Account* PaymentState::Find(const AccountId& id) const
{
    const auto it = m_accounts.find(id);
    return it == m_accounts.end() ? nullptr : &it->second;
}

Amount PaymentState::Held(const Account* account, CurrencyId currency)
{
    if (account == nullptr) return 0;
    const auto it = account->balances.find(currency);
    return it == account->balances.end() ? 0 : it->second;
}

bool PaymentState::CanCredit(const Account* account, CurrencyId currency, Amount amount)
{
    return Held(account, currency) <= MAX_AMOUNT - amount;
}

void PaymentState::Credit(Account& account, CurrencyId currency, Amount amount) { account.balances[currency] += amount; }

void PaymentState::Debit(Account& account, CurrencyId currency, Amount amount)
{
    const auto it = account.balances.find(currency);
    it->second -= amount;
    if (it->second == 0) account.balances.erase(it);
}

Amount PaymentState::Balance(const AccountId& account, CurrencyId currency) const { return Held(Find(account), currency); }

uint64_t PaymentState::Nonce(const AccountId& account) const
{
    const Account* a = Find(account);
    return a == nullptr ? 0 : a->nonce;
}

bool PaymentState::IsConverter(const AccountId& account) const
{
    const Account* a = Find(account);
    return a != nullptr && a->converter;
}

Amount PaymentState::Supply(CurrencyId currency) const
{
    const auto it = m_supply.find(currency);
    return it == m_supply.end() ? 0 : it->second;
}

bool PaymentState::CheckConservation() const
{
    std::map<CurrencyId, Amount> sums;
    for (const auto& [id, account] : m_accounts) {
        for (const auto& [currency, amount] : account.balances) {
            if (amount <= 0) return false;
            Amount& sum = sums[currency];
            if (sum > MAX_AMOUNT - amount) return false;
            sum += amount;
        }
    }
    return sums == m_supply;
}

// The order of the tests below is part of the rule: it decides which status a wrong operation gets.
OpStatus PaymentState::ApplyOp(const Op& op, const std::map<CurrencyId, Rate>& rates)
{
    const Account* from = Find(op.from);

    if (op.type == OpType::DEPOSIT) {
        if (op.amount <= 0) return OpStatus::BAD_AMOUNT;
        if (!CanCredit(from, op.currency, op.amount) || Supply(op.currency) > MAX_AMOUNT - op.amount) return OpStatus::AMOUNT_OVERFLOW;
        Credit(m_accounts[op.from], op.currency, op.amount);
        m_supply[op.currency] += op.amount;
        return OpStatus::OK;
    }

    if (op.nonce != (from == nullptr ? 0 : from->nonce)) return OpStatus::BAD_NONCE;

    switch (op.type) {
    case OpType::SET_CONVERTER: {
        Account& a = m_accounts[op.from];
        a.converter = op.flag == 1;
        ++a.nonce;
        return OpStatus::OK;
    }
    case OpType::WITHDRAW: {
        if (op.amount <= 0) return OpStatus::BAD_AMOUNT;
        if (Held(from, op.currency) < op.amount) return OpStatus::INSUFFICIENT_FUNDS;
        Account& a = m_accounts[op.from];
        Debit(a, op.currency, op.amount);
        ++a.nonce;
        const auto it = m_supply.find(op.currency);
        it->second -= op.amount;
        if (it->second == 0) m_supply.erase(it);
        return OpStatus::OK;
    }
    case OpType::PAY: {
        if (op.amount <= 0) return OpStatus::BAD_AMOUNT;
        if (op.from == op.to) return OpStatus::BAD_PARTIES;
        if (Held(from, op.currency) < op.amount) return OpStatus::INSUFFICIENT_FUNDS;
        if (!CanCredit(Find(op.to), op.currency, op.amount)) return OpStatus::AMOUNT_OVERFLOW;
        Account& payer = m_accounts[op.from];
        Debit(payer, op.currency, op.amount);
        ++payer.nonce;
        Credit(m_accounts[op.to], op.currency, op.amount);
        return OpStatus::OK;
    }
    case OpType::PAY_CONVERT: {
        if (op.amount <= 0 || op.recv_amount <= 0) return OpStatus::BAD_AMOUNT;
        if (op.currency == op.recv_currency || op.converter == op.from || op.converter == op.to) return OpStatus::BAD_PARTIES;
        const auto rate_paid = rates.find(op.currency);
        const auto rate_recv = rates.find(op.recv_currency);
        if (rate_paid == rates.end() || rate_recv == rates.end()) return OpStatus::NO_RATE;
        Amount expected = 0;
        if (!ConvertFloor(op.amount, rate_paid->second, rate_recv->second, expected) || expected != op.recv_amount) return OpStatus::RATE_MISMATCH;
        const Account* converter = Find(op.converter);
        if (converter == nullptr || !converter->converter) return OpStatus::NOT_CONVERTER;
        if (Held(from, op.currency) < op.amount) return OpStatus::INSUFFICIENT_FUNDS;
        if (Held(converter, op.recv_currency) < op.recv_amount) return OpStatus::CONVERTER_INSUFFICIENT;
        if (!CanCredit(converter, op.currency, op.amount) || !CanCredit(Find(op.to), op.recv_currency, op.recv_amount)) return OpStatus::AMOUNT_OVERFLOW;

        // Every test passed: the four legs, together. Nothing above changed the state.
        Account& payer = m_accounts[op.from];
        Debit(payer, op.currency, op.amount);
        ++payer.nonce;
        Account& conv = m_accounts[op.converter];
        Credit(conv, op.currency, op.amount);
        Debit(conv, op.recv_currency, op.recv_amount);
        Credit(m_accounts[op.to], op.recv_currency, op.recv_amount);
        return OpStatus::OK;
    }
    case OpType::DEPOSIT:
        break; // handled above
    }
    return OpStatus::BAD_PARTIES; // not reached: ParseBatch refuses an unknown type
}

BatchResult PaymentState::ApplyBatch(const std::vector<uint8_t>& batch_bytes)
{
    BatchResult result;
    result.root = Root();

    Batch batch;
    if (!ParseBatch(batch_bytes, batch)) {
        result.error = BatchError::MALFORMED;
        return result;
    }
    if (batch.number != m_batch_count + 1) {
        result.error = BatchError::WRONG_NUMBER;
        return result;
    }
    if (batch.prev_root != result.root) {
        result.error = BatchError::WRONG_PREV_ROOT;
        return result;
    }

    std::map<CurrencyId, Rate> rates;
    for (const Rate& r : batch.rates) rates[r.currency] = r;

    result.statuses.reserve(batch.ops.size());
    for (const Op& op : batch.ops) {
        result.statuses.push_back(ApplyOp(op, rates));
    }

    m_batch_count = batch.number;
    m_last_batch_hash = Sha256(batch_bytes);
    result.batch_hash = m_last_batch_hash;
    result.root = Root();
    return result;
}

Hash256 PaymentState::Root() const
{
    // One flat hash of the ordered state. A tree (to prove one account without the others) is a later step.
    std::vector<uint8_t> v;
    v.insert(v.end(), STATE_MAGIC, STATE_MAGIC + 4);
    Put8(v, FORMAT_VERSION);
    Put64(v, m_batch_count);
    PutBytes(v, m_last_batch_hash);
    Put64(v, m_supply.size());
    for (const auto& [currency, amount] : m_supply) {
        Put32(v, currency);
        Put64(v, static_cast<uint64_t>(amount));
    }
    Put64(v, m_accounts.size());
    for (const auto& [id, account] : m_accounts) {
        PutBytes(v, id);
        Put64(v, account.nonce);
        Put8(v, account.converter ? 1 : 0);
        Put64(v, account.balances.size());
        for (const auto& [currency, amount] : account.balances) {
            Put32(v, currency);
            Put64(v, static_cast<uint64_t>(amount));
        }
    }
    return Sha256(v);
}

} // namespace o_layer2
