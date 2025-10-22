# Measurement Invitation System

## Overview

The O Blockchain measurement system uses **invitations** to request water price and exchange rate measurements from randomly selected verified users. This ensures decentralized, accurate price data for the water-calibrated currency.

---

## How It Works (End-to-End)

### **Step 1: System Identifies Measurement Gap**

The blockchain automatically detects when more measurements are needed:

```cpp
// Example: Need 100 water prices for OMXN
if (current_measurements < target_measurements) {
    CreateInvitesForTargetMeasurements(100, WATER_PRICE, "OMXN");
}
```

---

### **Step 2: Invitations Created**

System creates invitations for randomly selected users:

- **Selection**: Random verified BrightID users in the target currency zone
- **Targeting**: Currency-specific (e.g., Mexican users for OMXN)
- **Conversion Rate**: System tracks how many invites needed to get target measurements
- **Expiration**: Invites expire after configured days (default: 7 days)

**Database Storage**:
```
invite_id: abc123...
invited_user: [user's public key]
type: WATER_PRICE
currency_code: OMXN
created_at: 1234567890
expires_at: 1234654290
is_used: false
```

---

### **Step 3: Users Check for Invites** ✨ NEW

Users can now retrieve their pending invitations:

**Command**:
```bash
bitcoin-cli getmyinvites
```

**Response**:
```json
[
  {
    "invite_id": "abc123def456...",
    "type": "water",
    "currency": "OMXN",
    "created_at": 1234567890,
    "expires_at": 1234654290,
    "time_remaining": 86400,
    "status": "active"
  },
  {
    "invite_id": "xyz789...",
    "type": "exchange",
    "currency": "OMXN",
    "created_at": 1234567900,
    "expires_at": 1234654300,
    "time_remaining": 86410,
    "status": "active"
  }
]
```

---

### **Step 4: User Submits Measurement**

User fulfills the invitation by submitting a measurement:

**Example: Water Price**
```bash
bitcoin-cli submitwaterprice \
    "OMXN" \
    150 \
    "Mexico City, Mexico" \
    "https://walmart.com.mx/agua" \
    "abc123def456..."
```

**Example: Exchange Rate**
```bash
bitcoin-cli submitexchangerate \
    "OMXN" \
    "USD" \
    20.5 \
    "https://exchangerate.com" \
    "xyz789..."
```

---

### **Step 5: Validation**

System validates the submission:

1. **Invite Validity**:
   - Invite exists
   - Not expired
   - Not already used

2. **User Authorization**:
   - **Critical Security Check**: Submitter's public key matches `invite.invited_user`
   - Prevents invite sharing or reuse

3. **Measurement Quality**:
   - URL validation
   - Price reasonability
   - Photo verification (if required)

If valid:
- Measurement accepted
- Invite marked as used
- User earns measurement fees

If invalid:
- Measurement rejected
- Invite remains active (if not expired)
- User can retry

---

## RPC Commands

### **getmyinvites**

Get all pending measurement invitations for your wallet.

**Syntax**:
```bash
bitcoin-cli getmyinvites [address]
```

**Parameters**:
- `address` (optional): Specific address to check. Uses wallet default if omitted.

**Returns**:
- Array of active invitations with details

**Example**:
```bash
# Check with wallet's default address
bitcoin-cli getmyinvites

# Check specific address
bitcoin-cli getmyinvites "1YourAddressHere..."
```

---

### **createinvites** (Admin/Testing)

Manually create measurement invitations.

**Syntax**:
```bash
bitcoin-cli createinvites <count> <type> [currency]
```

**Parameters**:
- `count`: Number of invites to create
- `type`: "water", "exchange", or "validation"
- `currency`: Optional currency code (e.g., "OMXN")

**Example**:
```bash
# Create 10 water price invites for OMXN
bitcoin-cli createinvites 10 water OMXN

# Create 5 exchange rate invites
bitcoin-cli createinvites 5 exchange
```

---

## Wallet UI Integration

### **Recommended Implementation**

**1. Periodic Polling**:
```javascript
// Check for invites every 10 minutes
setInterval(async () => {
    const invites = await rpc.call('getmyinvites');
    if (invites.length > 0) {
        showNotification(`You have ${invites.length} pending measurement requests`);
    }
}, 600000);
```

**2. Notification Display**:
```
╔════════════════════════════════════════╗
║  💧 Measurement Requests (3 pending)  ║
╠════════════════════════════════════════╣
║  Water Price - OMXN (expires in 6h)   ║
║  Exchange Rate - OMXN/USD (expires 1d) ║
║  Water Price - OMXN (expires in 23h)   ║
╚════════════════════════════════════════╝
```

**3. Click to Fulfill**:
- User clicks notification
- Opens measurement submission form
- Pre-fills invite_id automatically
- User enters price/URL
- Submits measurement

---

## Security Features

### **1. User Authorization**

Only the invited user can submit measurements:

```cpp
if (invite.invited_user != submitter) {
    LogPrintf("SECURITY VIOLATION - Submitter mismatch\n");
    return false;
}
```

### **2. One-Time Use**

Invites can only be used once:

```cpp
if (invite.is_used) {
    LogPrintf("SECURITY: Attempt to reuse invite\n");
    return false;
}
```

### **3. Expiration**

Invites expire after configured period:

```cpp
if (current_time > invite.expires_at) {
    return false; // Expired
}
```

---

## Statistics & Monitoring

### **Conversion Rate Tracking**

System tracks invitation effectiveness:

```cpp
double conversion_rate = 
    measurements_received / invites_sent;

// Example: 0.5 = 50% conversion rate
// If need 100 measurements, send 200 invites
```

### **Dynamic Invite Count**

Based on historical conversion rates:

```cpp
int invite_count = 
    ceil(target_measurements / conversion_rate);

// Better conversion = fewer invites needed
// Worse conversion = more invites sent
```

---

## Future Enhancements

### **Phase 1: Current (Pull-Based)** ✅
- Users check `getmyinvites` periodically
- Wallet UI polls for updates
- Simple, works today

### **Phase 2: Push Notifications** (Future)
- P2P network `INVITE` message type
- Real-time notification to connected peers
- Lower latency

### **Phase 3: Incentive Optimization** (Future)
- Dynamic fees based on urgency
- Priority invites for critical measurements
- Reputation-based selection

---

## Testing

### **Create Test Invites**:
```bash
# Create 5 water price invites for testing
bitcoin-cli createinvites 5 water OMXN
```

### **Check Your Invites**:
```bash
# See pending invites
bitcoin-cli getmyinvites
```

### **Submit Test Measurement**:
```bash
# Submit water price with invite
bitcoin-cli submitwaterprice \
    "OMXN" \
    150 \
    "Test Location" \
    "https://test.com" \
    "YOUR_INVITE_ID_HERE"
```

### **Verify Used**:
```bash
# Should show one fewer invite
bitcoin-cli getmyinvites
```

---

## Summary

The measurement invitation system ensures:

✅ **Decentralized measurement** - No central authority decides who measures  
✅ **Targeted collection** - Currency-specific user selection  
✅ **Security** - Only invited user can submit  
✅ **Efficiency** - Conversion rate tracking optimizes invite count  
✅ **User-friendly** - Simple pull-based delivery via RPC  

**Next**: Integrate `getmyinvites` into wallet UI for seamless user experience!

