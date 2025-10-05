// Copyright (c) 2024 The Bitcoin Core developers
// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <measurement/volume_conversion.h>
#include <logging.h>
#include <util/strencodings.h>
#include <cmath>
#include <algorithm>

// ===== VolumeMeasurement Implementation =====

bool VolumeMeasurement::IsValid() const {
    return VolumeConverter::IsVolumeAcceptable(volume, unit);
}

double VolumeMeasurement::ToLiters() const {
    return VolumeConverter::ToLiters(volume, unit);
}

std::string VolumeMeasurement::GetUnitString() const {
    return VolumeConverter::UnitToString(unit);
}

// ===== VolumeConverter Implementation =====

double VolumeConverter::ToLiters(double volume, VolumeUnit unit) {
    switch (unit) {
        case VolumeUnit::LITERS:
            return volume;
        
        case VolumeUnit::MILLILITERS:
            return volume / ML_PER_LITER;
        
        case VolumeUnit::FLUID_OUNCES:
            return volume / FL_OZ_PER_LITER;
        
        case VolumeUnit::GALLONS_US:
            return volume * LITERS_PER_US_GALLON;
        
        case VolumeUnit::GALLONS_UK:
            return volume * LITERS_PER_UK_GALLON;
        
        case VolumeUnit::PINTS:
            return volume * LITERS_PER_PINT;
        
        case VolumeUnit::QUARTS:
            return volume * LITERS_PER_QUART;
        
        default:
            LogPrintf("O Volume: Unknown volume unit: %d\n", static_cast<int>(unit));
            return 0.0;
    }
}

double VolumeConverter::FromLiters(double liters, VolumeUnit unit) {
    switch (unit) {
        case VolumeUnit::LITERS:
            return liters;
        
        case VolumeUnit::MILLILITERS:
            return liters * ML_PER_LITER;
        
        case VolumeUnit::FLUID_OUNCES:
            return liters * FL_OZ_PER_LITER;
        
        case VolumeUnit::GALLONS_US:
            return liters / LITERS_PER_US_GALLON;
        
        case VolumeUnit::GALLONS_UK:
            return liters / LITERS_PER_UK_GALLON;
        
        case VolumeUnit::PINTS:
            return liters / LITERS_PER_PINT;
        
        case VolumeUnit::QUARTS:
            return liters / LITERS_PER_QUART;
        
        default:
            LogPrintf("O Volume: Unknown volume unit: %d\n", static_cast<int>(unit));
            return 0.0;
    }
}

VolumeValidationResult VolumeConverter::ValidateAndConvert(
    double volume,
    VolumeUnit unit,
    int64_t price_for_volume,
    const std::string& currency_code
) {
    VolumeValidationResult result;
    
    // Validate volume is positive
    if (volume <= 0.0) {
        result.is_valid = false;
        result.error_message = "Volume must be positive";
        LogPrintf("O Volume: Invalid volume (non-positive): %.4f %s\n", 
                  volume, UnitToString(unit).c_str());
        return result;
    }
    
    // Validate price is positive
    if (price_for_volume <= 0) {
        result.is_valid = false;
        result.error_message = "Price must be positive";
        LogPrintf("O Volume: Invalid price (non-positive): %lld\n", price_for_volume);
        return result;
    }
    
    // Check if volume is within acceptable range
    if (!IsVolumeAcceptable(volume, unit)) {
        auto [min_vol, max_vol] = GetAcceptableRange(unit);
        result.is_valid = false;
        result.error_message = "Volume out of acceptable range (" + 
                              std::to_string(min_vol) + " - " + 
                              std::to_string(max_vol) + " " + UnitToString(unit) + ")";
        LogPrintf("O Volume: Volume out of range: %.4f %s (acceptable: %.4f - %.4f)\n",
                  volume, UnitToString(unit).c_str(), min_vol, max_vol);
        return result;
    }
    
    // Convert volume to liters
    double volume_liters = ToLiters(volume, unit);
    result.volume_liters = volume_liters;
    
    // Calculate price per liter
    int64_t price_per_liter = CalculatePricePerLiter(price_for_volume, volume, unit);
    result.price_per_liter = static_cast<double>(price_per_liter);
    
    // Validate price per liter is reasonable
    if (!IsPricePerLiterReasonable(price_per_liter, currency_code)) {
        result.is_valid = false;
        result.error_message = "Price per liter is unreasonable for currency " + currency_code;
        LogPrintf("O Volume: Unreasonable price per liter: %lld for currency %s\n",
                  price_per_liter, currency_code.c_str());
        return result;
    }
    
    result.is_valid = true;
    
    LogPrintf("O Volume: Validated measurement - Volume: %.4f %s (%.4fL), Price: %lld, Price/L: %lld\n",
              volume, UnitToString(unit).c_str(), volume_liters, 
              price_for_volume, price_per_liter);
    
    return result;
}

bool VolumeConverter::IsVolumeAcceptable(double volume, VolumeUnit unit) {
    // Convert to liters for validation
    double volume_liters = ToLiters(volume, unit);
    
    // Check if within acceptable range (0.9L - 1.1L)
    if (volume_liters < MIN_LITERS || volume_liters > MAX_LITERS) {
        return false;
    }
    
    return true;
}

std::pair<double, double> VolumeConverter::GetAcceptableRange(VolumeUnit unit) {
    // Get range in liters, then convert to requested unit
    double min_in_unit = FromLiters(MIN_LITERS, unit);
    double max_in_unit = FromLiters(MAX_LITERS, unit);
    
    return {min_in_unit, max_in_unit};
}

int64_t VolumeConverter::CalculatePricePerLiter(
    int64_t price_for_volume,
    double volume,
    VolumeUnit unit
) {
    // Convert volume to liters
    double volume_liters = ToLiters(volume, unit);
    
    if (volume_liters <= 0.0) {
        LogPrintf("O Volume: Cannot calculate price per liter - invalid volume\n");
        return 0;
    }
    
    // Pro-rate to price per liter
    // price_per_liter = price_for_volume / volume_liters
    double price_per_liter_double = static_cast<double>(price_for_volume) / volume_liters;
    
    // Round to nearest integer
    int64_t price_per_liter = static_cast<int64_t>(std::round(price_per_liter_double));
    
    LogPrintf("O Volume: Price calculation - Container: %lld for %.4f %s (%.4fL), Per Liter: %lld\n",
              price_for_volume, volume, UnitToString(unit).c_str(), 
              volume_liters, price_per_liter);
    
    return price_per_liter;
}

bool VolumeConverter::IsPricePerLiterReasonable(
    int64_t price_per_liter,
    const std::string& currency_code
) {
    // Define reasonable price ranges per currency (in smallest units: cents, yen, etc.)
    // These are rough estimates - water should cost between $0.10 and $10 per liter in most currencies
    
    // For now, accept any positive price
    // TODO: Implement currency-specific ranges when we have historical data
    if (price_per_liter <= 0) {
        return false;
    }
    
    // Rough sanity check: price per liter should be between 0.01 and 10000 units
    // This catches obvious errors like inputting wrong decimal place
    if (price_per_liter < 1 || price_per_liter > 10000000) {
        LogPrintf("O Volume: Price per liter %lld is outside reasonable range (1 - 10000000)\n", 
                  price_per_liter);
        return false;
    }
    
    return true;
}

std::optional<VolumeUnit> VolumeConverter::ParseUnit(const std::string& unit_str) {
    std::string lower = ToLower(unit_str);
    
    // Metric
    if (lower == "l" || lower == "liter" || lower == "liters" || lower == "litre" || lower == "litres") {
        return VolumeUnit::LITERS;
    }
    if (lower == "ml" || lower == "milliliter" || lower == "milliliters" || lower == "millilitre" || lower == "millilitres") {
        return VolumeUnit::MILLILITERS;
    }
    
    // Imperial
    if (lower == "oz" || lower == "fl oz" || lower == "floz" || lower == "fluid ounce" || lower == "fluid ounces") {
        return VolumeUnit::FLUID_OUNCES;
    }
    if (lower == "gal" || lower == "gallon" || lower == "gallons") {
        return VolumeUnit::GALLONS_US; // Default to US gallons
    }
    if (lower == "us gal" || lower == "us gallon" || lower == "us gallons") {
        return VolumeUnit::GALLONS_US;
    }
    if (lower == "uk gal" || lower == "uk gallon" || lower == "uk gallons" || lower == "imp gal" || lower == "imperial gallon") {
        return VolumeUnit::GALLONS_UK;
    }
    if (lower == "pt" || lower == "pint" || lower == "pints") {
        return VolumeUnit::PINTS;
    }
    if (lower == "qt" || lower == "quart" || lower == "quarts") {
        return VolumeUnit::QUARTS;
    }
    
    LogPrintf("O Volume: Unknown unit string: %s\n", unit_str.c_str());
    return std::nullopt;
}

std::string VolumeConverter::UnitToString(VolumeUnit unit) {
    switch (unit) {
        case VolumeUnit::LITERS:
            return "L";
        case VolumeUnit::MILLILITERS:
            return "mL";
        case VolumeUnit::FLUID_OUNCES:
            return "fl oz";
        case VolumeUnit::GALLONS_US:
            return "US gal";
        case VolumeUnit::GALLONS_UK:
            return "UK gal";
        case VolumeUnit::PINTS:
            return "pints";
        case VolumeUnit::QUARTS:
            return "quarts";
        default:
            return "unknown";
    }
}

VolumeUnit VolumeConverter::GetRecommendedUnit(const std::string& country_code) {
    // Countries using Imperial/US customary units
    if (country_code == "US" || country_code == "USA") {
        return VolumeUnit::FLUID_OUNCES;
    }
    if (country_code == "GB" || country_code == "UK") {
        return VolumeUnit::FLUID_OUNCES;
    }
    if (country_code == "LR" || country_code == "MM") {
        // Liberia, Myanmar also use some imperial units
        return VolumeUnit::FLUID_OUNCES;
    }
    
    // Default to metric (liters) for all other countries
    return VolumeUnit::LITERS;
}

// ===== Helper Function =====

VolumeValidationResult ValidateWaterPriceVolume(
    double volume,
    const std::string& unit_str,
    int64_t price,
    const std::string& currency_code
) {
    // Parse unit string
    auto unit_opt = VolumeConverter::ParseUnit(unit_str);
    if (!unit_opt.has_value()) {
        VolumeValidationResult result;
        result.is_valid = false;
        result.error_message = "Invalid or unsupported volume unit: " + unit_str;
        LogPrintf("O Volume: Failed to parse unit string: %s\n", unit_str.c_str());
        return result;
    }
    
    VolumeUnit unit = unit_opt.value();
    
    // Validate and convert
    return VolumeConverter::ValidateAndConvert(volume, unit, price, currency_code);
}

