// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_MEASUREMENT_VOLUME_CONVERSION_H
#define BITCOIN_MEASUREMENT_VOLUME_CONVERSION_H

#include <cstdint>
#include <string>
#include <optional>

/**
 * Volume Unit System for Water Price Measurements
 * 
 * The O Blockchain stablecoin is calibrated to water prices at 1 liter.
 * However, water is sold in various container sizes and unit systems globally.
 * This module handles volume validation and conversion to standardized 1-liter pricing.
 */

/** Volume Unit Types */
enum class VolumeUnit {
    LITERS,          // Metric: Liters (L)
    MILLILITERS,     // Metric: Milliliters (mL)
    FLUID_OUNCES,    // Imperial: Fluid Ounces (fl oz)
    GALLONS_US,      // US: Gallons
    GALLONS_UK,      // UK: Imperial Gallons
    PINTS,           // Pints
    QUARTS           // Quarts
};

/** Volume Measurement with Unit */
struct VolumeMeasurement {
    double volume;              // Volume amount
    VolumeUnit unit;            // Unit of measurement
    
    VolumeMeasurement() : volume(0.0), unit(VolumeUnit::LITERS) {}
    VolumeMeasurement(double v, VolumeUnit u) : volume(v), unit(u) {}
    
    /** Check if volume is within acceptable range */
    bool IsValid() const;
    
    /** Convert to liters */
    double ToLiters() const;
    
    /** Get human-readable unit string */
    std::string GetUnitString() const;
};

/** Volume Validation Result */
struct VolumeValidationResult {
    bool is_valid;                  // Whether volume is acceptable
    double volume_liters;           // Volume converted to liters
    double price_per_liter;         // Price pro-rated to per-liter
    std::string error_message;      // Error message if invalid
    
    VolumeValidationResult() 
        : is_valid(false), volume_liters(0.0), price_per_liter(0.0), error_message() {}
};

/** Volume Conversion and Validation System */
class VolumeConverter {
public:
    // ===== Conversion Factors =====
    
    /** Convert volume to liters */
    static double ToLiters(double volume, VolumeUnit unit);
    
    /** Convert liters to specific unit */
    static double FromLiters(double liters, VolumeUnit unit);
    
    // ===== Validation =====
    
    /** Validate and convert volume measurement */
    static VolumeValidationResult ValidateAndConvert(
        double volume,
        VolumeUnit unit,
        int64_t price_for_volume,
        const std::string& currency_code
    );
    
    /** Check if volume is within acceptable range (0.9L - 1.1L for metric) */
    static bool IsVolumeAcceptable(double volume, VolumeUnit unit);
    
    /** Get acceptable range for a unit */
    static std::pair<double, double> GetAcceptableRange(VolumeUnit unit);
    
    // ===== Pro-Rating =====
    
    /** Calculate price per liter from container price */
    static int64_t CalculatePricePerLiter(
        int64_t price_for_volume,
        double volume,
        VolumeUnit unit
    );
    
    /** Validate price per liter is reasonable */
    static bool IsPricePerLiterReasonable(
        int64_t price_per_liter,
        const std::string& currency_code
    );
    
    // ===== Unit Parsing =====
    
    /** Parse unit string to VolumeUnit enum */
    static std::optional<VolumeUnit> ParseUnit(const std::string& unit_str);
    
    /** Get unit string from enum */
    static std::string UnitToString(VolumeUnit unit);
    
    /** Get recommended unit for a country */
    static VolumeUnit GetRecommendedUnit(const std::string& country_code);
    
    // ===== Constants =====
    
    // Metric System (Liters)
    static constexpr double MIN_LITERS = 0.9;        // Minimum: 0.9L
    static constexpr double MAX_LITERS = 1.1;        // Maximum: 1.1L
    static constexpr double TARGET_LITERS = 1.0;     // Target: 1.0L
    
    // Imperial System (Fluid Ounces) - 1 liter ≈ 33.814 fl oz
    static constexpr double MIN_FL_OZ = 30.0;        // Minimum: 30 fl oz (~0.887L)
    static constexpr double MAX_FL_OZ = 37.0;        // Maximum: 37 fl oz (~1.094L)
    static constexpr double TARGET_FL_OZ = 33.814;   // Target: ~1L
    
    // Conversion Factors
    static constexpr double ML_PER_LITER = 1000.0;
    static constexpr double FL_OZ_PER_LITER = 33.814;
    static constexpr double LITERS_PER_US_GALLON = 3.78541;
    static constexpr double LITERS_PER_UK_GALLON = 4.54609;
    static constexpr double LITERS_PER_PINT = 0.473176;
    static constexpr double LITERS_PER_QUART = 0.946353;
};

/** Helper function to validate volume for water price measurement */
VolumeValidationResult ValidateWaterPriceVolume(
    double volume,
    const std::string& unit_str,
    int64_t price,
    const std::string& currency_code
);

#endif // BITCOIN_MEASUREMENT_VOLUME_CONVERSION_H

