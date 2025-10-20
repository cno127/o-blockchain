// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/o_measurement_db.h>
#include <measurement/measurement_system.h>
#include <test/util/setup_common.h>
#include <boost/test/unit_test.hpp>
#include <util/strencodings.h>
#include <uint256.h>

using namespace OMeasurement;

// Helper to create unique uint256 for testing
static uint256 MakeTestUint256(int id) {
    uint256 result;
    result.SetNull();
    *(reinterpret_cast<int*>(result.begin())) = id;
    return result;
}

BOOST_FIXTURE_TEST_SUITE(o_measurement_db_tests, BasicTestingSetup)

BOOST_AUTO_TEST_CASE(measurement_db_water_price_write_read)
{
    // Create temporary database
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false); // 2MB cache, memory-only
    
    // Create test water price measurement
    WaterPriceMeasurement measurement;
    measurement.measurement_id = uint256{"1234567890abcdef1234567890abcdef1234567890abcdef1234567890abcdef"};
    measurement.currency_code = "USD";
    measurement.price = 150; // $1.50 in cents
    measurement.volume = 1.0; // 1.0 liters
    measurement.volume_unit = "L";
    measurement.price_per_liter = 150;
    measurement.location = "New York, USA";
    measurement.timestamp = 1234567890;
    measurement.block_height = 100;
    measurement.is_validated = true;
    measurement.confidence_score = 0.95;
    measurement.source = MeasurementSource::USER_ONLINE;
    
    // Write measurement
    BOOST_CHECK(db->WriteWaterPrice(measurement.measurement_id, measurement));
    
    // Read back
    auto read = db->ReadWaterPrice(measurement.measurement_id);
    BOOST_CHECK(read.has_value());
    BOOST_CHECK_EQUAL(read->currency_code, "USD");
    BOOST_CHECK_EQUAL(read->price, 150);
    BOOST_CHECK_CLOSE(read->volume, 1.0, 0.0001);
    BOOST_CHECK_EQUAL(read->volume_unit, "L");
    BOOST_CHECK_CLOSE(read->confidence_score, 0.95, 0.0001);
    BOOST_CHECK(read->source == MeasurementSource::USER_ONLINE);
}

BOOST_AUTO_TEST_CASE(measurement_db_exchange_rate_write_read)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Create test exchange rate measurement
    ExchangeRateMeasurement measurement;
    measurement.measurement_id = uint256{"abcdef1234567890abcdef1234567890abcdef1234567890abcdef1234567890"};
    measurement.from_currency = "OUSD";
    measurement.to_currency = "USD";
    measurement.exchange_rate = 1.25;
    measurement.location = "Paris, France";
    measurement.timestamp = 1234567890;
    measurement.block_height = 150;
    measurement.is_validated = true;
    measurement.source = MeasurementSource::BOT_ONLINE;
    
    // Write measurement
    BOOST_CHECK(db->WriteExchangeRate(measurement.measurement_id, measurement));
    
    // Read back
    auto read = db->ReadExchangeRate(measurement.measurement_id);
    BOOST_CHECK(read.has_value());
    BOOST_CHECK_EQUAL(read->from_currency, "OUSD");
    BOOST_CHECK_EQUAL(read->to_currency, "USD");
    BOOST_CHECK_CLOSE(read->exchange_rate, 1.25, 0.0001);
    BOOST_CHECK(read->source == MeasurementSource::BOT_ONLINE);
}

BOOST_AUTO_TEST_CASE(measurement_db_double_serialization)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Test various double values in water price measurements
    std::vector<double> test_volumes = {0.5, 0.95, 1.0, 1.5, 33.8};
    std::vector<double> test_confidence = {0.0, 0.5, 0.95, 0.999999, 1.0};
    
    for (size_t i = 0; i < test_volumes.size(); i++) {
        WaterPriceMeasurement m;
        m.measurement_id = MakeTestUint256(1000 + i);
        m.volume = test_volumes[i];
        m.confidence_score = test_confidence[i % test_confidence.size()];
        
        BOOST_CHECK(db->WriteWaterPrice(m.measurement_id, m));
        
        auto read = db->ReadWaterPrice(m.measurement_id);
        BOOST_CHECK(read.has_value());
        BOOST_CHECK_CLOSE(read->volume, test_volumes[i], 0.0001);
        BOOST_CHECK_CLOSE(read->confidence_score, test_confidence[i % test_confidence.size()], 0.0001);
    }
    
    // Test exchange rate double serialization
    std::vector<double> test_rates = {0.5, 1.0, 1.25, 1.5, 100.123456};
    
    for (size_t i = 0; i < test_rates.size(); i++) {
        ExchangeRateMeasurement m;
        m.measurement_id = MakeTestUint256(2000 + i);
        m.exchange_rate = test_rates[i];
        
        BOOST_CHECK(db->WriteExchangeRate(m.measurement_id, m));
        
        auto read = db->ReadExchangeRate(m.measurement_id);
        BOOST_CHECK(read.has_value());
        BOOST_CHECK_CLOSE(read->exchange_rate, test_rates[i], 0.0001);
    }
}

BOOST_AUTO_TEST_CASE(measurement_db_daily_average)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Create daily average
    DailyAverage avg;
    avg.currency_code = "USD";
    avg.date = "2025-10-19";
    avg.avg_water_price = 1.5;
    avg.avg_exchange_rate = 1.25;
    avg.measurement_count = 25;
    avg.std_deviation = 0.15;
    avg.is_stable = true;
    avg.block_height = 1000;
    avg.confidence_level = ConfidenceLevel::HIGH_CONFIDENCE;
    avg.is_statistically_significant = true;
    
    // Write daily average
    BOOST_CHECK(db->WriteDailyAverage("USD", "2025-10-19", avg));
    
    // Read back
    auto read = db->ReadDailyAverage("USD", "2025-10-19");
    BOOST_CHECK(read.has_value());
    BOOST_CHECK_EQUAL(read->currency_code, "USD");
    BOOST_CHECK_EQUAL(read->date, "2025-10-19");
    BOOST_CHECK_CLOSE(read->avg_water_price, 1.5, 0.0001);
    BOOST_CHECK_CLOSE(read->avg_exchange_rate, 1.25, 0.0001);
    BOOST_CHECK_CLOSE(read->std_deviation, 0.15, 0.0001);
    BOOST_CHECK(read->confidence_level == ConfidenceLevel::HIGH_CONFIDENCE);
}

BOOST_AUTO_TEST_CASE(measurement_db_validated_url)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Create validated URL
    ValidatedURL url;
    url.url_id = uint256{"1111111111111111111111111111111111111111111111111111111111111111"};
    url.url = "https://example.com/water-price";
    url.type = MeasurementType::WATER_PRICE;
    url.currency_code = "USD";
    url.last_checked = 1234567890;
    url.is_active = true;
    url.reliability_score = 0.98;
    url.validation_count = 50;
    url.block_height = 200;
    
    // Write URL
    BOOST_CHECK(db->WriteValidatedURL(url.url_id, url));
    
    // Read back
    auto read = db->ReadValidatedURL(url.url_id);
    BOOST_CHECK(read.has_value());
    BOOST_CHECK_EQUAL(read->url, "https://example.com/water-price");
    BOOST_CHECK(read->type == MeasurementType::WATER_PRICE);
    BOOST_CHECK_CLOSE(read->reliability_score, 0.98, 0.0001);
    BOOST_CHECK_EQUAL(read->validation_count, 50);
}

BOOST_AUTO_TEST_CASE(measurement_db_measurement_invite)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Create invite
    MeasurementInvite invite;
    invite.invite_id = uint256{"2222222222222222222222222222222222222222222222222222222222222222"};
    invite.type = MeasurementType::WATER_PRICE;
    invite.currency_code = "EUR";
    invite.created_at = 1234567890;
    invite.expires_at = 1234567890 + 86400 * 7; // 7 days
    invite.is_used = false;
    invite.is_expired = false;
    invite.block_height = 300;
    
    // Write invite
    BOOST_CHECK(db->WriteInvite(invite.invite_id, invite));
    
    // Read back
    auto read = db->ReadInvite(invite.invite_id);
    BOOST_CHECK(read.has_value());
    BOOST_CHECK(read->type == MeasurementType::WATER_PRICE);
    BOOST_CHECK_EQUAL(read->currency_code, "EUR");
    BOOST_CHECK_EQUAL(read->is_used, false);
    BOOST_CHECK_EQUAL(read->is_expired, false);
}

BOOST_AUTO_TEST_CASE(measurement_db_confidence_level_serialization)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Test all confidence levels
    std::vector<ConfidenceLevel> levels = {
        ConfidenceLevel::INSUFFICIENT_DATA,
        ConfidenceLevel::LOW_CONFIDENCE,
        ConfidenceLevel::HIGH_CONFIDENCE,
        ConfidenceLevel::VERY_HIGH_CONFIDENCE
    };
    
    for (size_t i = 0; i < levels.size(); i++) {
        DailyAverage avg;
        avg.currency_code = "TEST";
        avg.date = "2025-10-" + std::to_string(10 + i);
        avg.confidence_level = levels[i];
        
        BOOST_CHECK(db->WriteDailyAverage("TEST", "2025-10-" + std::to_string(10 + i), avg));
        
        auto read = db->ReadDailyAverage("TEST", "2025-10-" + std::to_string(10 + i));
        BOOST_CHECK(read.has_value());
        BOOST_CHECK(read->confidence_level == levels[i]);
    }
}

BOOST_AUTO_TEST_CASE(measurement_db_get_recent_averages)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Write multiple daily averages for USD
    for (int i = 0; i < 30; i++) {
        DailyAverage avg;
        avg.currency_code = "USD";
        avg.date = "2025-10-" + std::to_string(1 + i);
        avg.avg_water_price = 1.5 + (i * 0.01);
        
        db->WriteDailyAverage("USD", "2025-10-" + std::to_string(1 + i), avg);
    }
    
    // Get recent averages
    auto averages = db->GetRecentDailyAverages("USD", 30);
    BOOST_CHECK(averages.size() > 0);
    
    // Verify they're all USD
    for (const auto& avg : averages) {
        BOOST_CHECK_EQUAL(avg.currency_code, "USD");
    }
}

BOOST_AUTO_TEST_CASE(measurement_db_erase_operations)
{
    auto db = std::make_unique<CMeasurementDB>(2 << 20, true, false);
    
    // Create and write measurement
    WaterPriceMeasurement m;
    m.measurement_id = uint256{"3333333333333333333333333333333333333333333333333333333333333333"};
    BOOST_CHECK(db->WriteWaterPrice(m.measurement_id, m));
    BOOST_CHECK(db->ReadWaterPrice(m.measurement_id).has_value());
    
    // Erase
    BOOST_CHECK(db->EraseWaterPrice(m.measurement_id));
    
    // Verify removed
    BOOST_CHECK(!db->ReadWaterPrice(m.measurement_id).has_value());
}

BOOST_AUTO_TEST_SUITE_END()

