// Copyright (c) 2026 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_O_LAYER2_PAYMENT_STATE_H
#define BITCOIN_O_LAYER2_PAYMENT_STATE_H

#include <array>
#include <cstdint>
#include <map>
#include <vector>

/**
 * O Layer 2 PROTOTYPE, first part: the shared multi-currency payment state.
 *
 * Spec base: the article "Why Proof of Work Was Built for Security, Not Speed" (section "Layer 2: shared
 * multi-currency payment execution"): retail payments happen in one account-based state, transactions are ordered in
 * batches, independent nodes replay the same bytes and calculate the same resulting state, and a payment with a
 * conversion is one operation: either every leg succeeds, or none does.
 *
 * What this file is: a deterministic executor. It takes the bytes of a batch, applies its operations in order and
 * gives the commitment (root) of the resulting state. Integers only, ordered containers only, one byte form per batch.
 *
 * What this file is NOT (test value only, nothing here is linked into the node):
 *  - no signature: an operation found in a batch is taken as authorized by its account;
 *  - no sequencer, no network, no storage: a caller hands the batches over, the state lives in memory;
 *  - no Layer 1 link: a DEPOSIT / WITHDRAW is not checked against the chain and the root is not posted to it;
 *  - no proof of a state transition, no data-availability rule;
 *  - no fee, no reward, no spam limit (economic settings, not defined here).
 *
 * Not decided here, on purpose:
 *  - the scale of an amount (issue #22): an Amount is a count of the currency's smallest unit, whatever that unit is;
 *  - who converts: a conversion is a swap with a named account that agreed to convert (SET_CONVERTER) and holds the
 *    other currency, so no unit of any currency is created or destroyed by a payment. Whether the protocol itself
 *    should be that party (mint / burn at the reference rate) is an economic choice left open;
 *  - where the rates come from: each batch carries its rate table as data (in the design, the reference rates Layer 1
 *    publishes); the executor only checks that a conversion uses that table;
 *  - rounding: the payee receives floor(paid x rate); the remainder (less than one smallest unit) stays with the
 *    converter. One rule had to be picked for the result to be reproducible; it is one line (ConvertFloor).
 */
namespace o_layer2 {

using AccountId = std::array<uint8_t, 32>;
using Hash256 = std::array<uint8_t, 32>;
using CurrencyId = uint32_t; //!< same width as CurrencyId of consensus/multicurrency.h
using Amount = int64_t;      //!< count of the currency's smallest unit

enum class OpType : uint8_t {
    DEPOSIT = 1,       //!< value enters the payment state (in the design: after a Layer 1 deposit)
    WITHDRAW = 2,      //!< value leaves the payment state (in the design: towards a Layer 1 withdrawal)
    PAY = 3,           //!< from -> to, one currency
    PAY_CONVERT = 4,   //!< from pays `amount` of `currency`, to receives `recv_amount` of `recv_currency`, through `converter`
    SET_CONVERTER = 5, //!< the account agrees (flag 1) or stops agreeing (flag 0) to convert at the batch's rates
};

enum class OpStatus : uint8_t {
    OK = 0,
    BAD_AMOUNT = 1,             //!< an amount is zero or negative
    BAD_NONCE = 2,              //!< the nonce is not the account's next one
    BAD_PARTIES = 3,            //!< PAY to oneself; PAY_CONVERT with the converter as payer or payee, or with one currency
    INSUFFICIENT_FUNDS = 4,     //!< the paying / withdrawing account does not hold the amount
    NO_RATE = 5,                //!< a currency of the conversion is not in the batch's rate table
    RATE_MISMATCH = 6,          //!< recv_amount is not floor(amount x rate) (this includes a result of zero)
    NOT_CONVERTER = 7,          //!< the named converter has not agreed to convert
    CONVERTER_INSUFFICIENT = 8, //!< the converter does not hold recv_amount of recv_currency
    AMOUNT_OVERFLOW = 9,        //!< a balance or a supply would pass the largest Amount
};

/** One operation. A field a type does not use must be zero (one byte form per operation). */
struct Op {
    OpType type{OpType::PAY};
    AccountId from{};      //!< DEPOSIT: the account credited. Else the account that acts (and whose nonce is used).
    AccountId to{};        //!< PAY, PAY_CONVERT: the payee
    AccountId converter{}; //!< PAY_CONVERT
    CurrencyId currency{0};
    Amount amount{0};
    CurrencyId recv_currency{0}; //!< PAY_CONVERT
    Amount recv_amount{0};       //!< PAY_CONVERT
    uint64_t nonce{0};           //!< number of operations of `from` already applied (not used by DEPOSIT)
    uint8_t flag{0};             //!< SET_CONVERTER: 1 = on, 0 = off
};

/** Value of one smallest unit of a currency in the batch's common reference: num / den. */
struct Rate {
    CurrencyId currency{0};
    uint32_t num{0};
    uint32_t den{0};
};

struct Batch {
    uint64_t number{0};      //!< 1 for the first batch, then +1
    Hash256 prev_root{};     //!< root of the state the batch applies to
    std::vector<Rate> rates; //!< strictly ascending by currency, num and den not zero
    std::vector<Op> ops;
};

/** The one byte form of a batch. */
std::vector<uint8_t> SerializeBatch(const Batch& batch);

/** Strict: any other byte form of the same content (or a malformed one) is refused. */
bool ParseBatch(const std::vector<uint8_t>& bytes, Batch& out);

/** floor(a * b / d). False when d is zero or the result does not fit in 64 bits. No 128-bit type needed. */
bool MulDivFloor(uint64_t a, uint64_t b, uint64_t d, uint64_t& out);

/** What the payee receives for `amount` of the `from` currency: floor(amount * from.num * to.den / (from.den * to.num)). */
bool ConvertFloor(Amount amount, const Rate& from, const Rate& to, Amount& out);

enum class BatchError : uint8_t {
    NONE = 0,
    MALFORMED = 1,       //!< the bytes are not the byte form of a batch
    WRONG_NUMBER = 2,    //!< not the next batch
    WRONG_PREV_ROOT = 3, //!< built on another state
};

struct BatchResult {
    BatchError error{BatchError::NONE}; //!< not NONE = the batch was refused and the state is unchanged
    std::vector<OpStatus> statuses;     //!< one per operation, in order; an operation that is not OK changed nothing
    Hash256 batch_hash{};               //!< SHA256 of the batch bytes
    Hash256 root{};                     //!< root of the state after the batch
};

class PaymentState
{
public:
    /** Apply the next batch. Same state + same bytes = same result, on every node. */
    BatchResult ApplyBatch(const std::vector<uint8_t>& batch_bytes);

    /** Commitment of the whole state (accounts, nonces, balances, supplies, batch count, last batch hash). */
    Hash256 Root() const;

    Amount Balance(const AccountId& account, CurrencyId currency) const;
    uint64_t Nonce(const AccountId& account) const;
    bool IsConverter(const AccountId& account) const;
    /** Deposits minus withdrawals of the currency. */
    Amount Supply(CurrencyId currency) const;
    uint64_t BatchCount() const { return m_batch_count; }

    /** True when, for every currency, the balances add up to the supply (payments create and destroy nothing). */
    bool CheckConservation() const;

private:
    struct Account {
        uint64_t nonce{0};
        bool converter{false};
        std::map<CurrencyId, Amount> balances; //!< no zero entry
    };

    OpStatus ApplyOp(const Op& op, const std::map<CurrencyId, Rate>& rates);
    static bool CanCredit(const Account* account, CurrencyId currency, Amount amount);
    static void Credit(Account& account, CurrencyId currency, Amount amount);
    static void Debit(Account& account, CurrencyId currency, Amount amount);
    static Amount Held(const Account* account, CurrencyId currency);
    const Account* Find(const AccountId& id) const;

    std::map<AccountId, Account> m_accounts; //!< ordered: the root never depends on insertion order
    std::map<CurrencyId, Amount> m_supply;   //!< no zero entry
    uint64_t m_batch_count{0};
    Hash256 m_last_batch_hash{};
};

} // namespace o_layer2

#endif // BITCOIN_O_LAYER2_PAYMENT_STATE_H
