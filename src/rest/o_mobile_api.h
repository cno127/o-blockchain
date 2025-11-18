// Copyright (c) 2025 The O Blockchain Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_REST_O_MOBILE_API_H
#define BITCOIN_REST_O_MOBILE_API_H

#include <httpserver.h>
#include <string>

/**
 * Mobile App REST API Endpoints
 * 
 * Provides RESTful endpoints for:
 * - User registration with country-based legal restrictions
 * - Exchange rates (O coin to fiat, measured rates)
 * - Map data (countries with water prices and stability)
 * - Wallet operations
 * - Measurement notifications and submissions
 */

// User Registration Endpoints
bool rest_user_register(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_user_status(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_user_legal_restrictions(const std::any& context, HTTPRequest* req, const std::string& strReq);

// Exchange Rate Endpoints
bool rest_exchange_rate_current(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_exchange_rate_measured(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_exchange_rate_historical(const std::any& context, HTTPRequest* req, const std::string& strReq);

// Map Data Endpoints
bool rest_map_countries(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_map_country(const std::any& context, HTTPRequest* req, const std::string& strReq);

// Wallet Endpoints
bool rest_wallet_balance(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_wallet_transactions(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_wallet_send(const std::any& context, HTTPRequest* req, const std::string& strReq);

// Measurement & Notification Endpoints
bool rest_notifications_invites(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_measurements_submit_water(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_measurements_submit_exchange(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_notifications_measurements(const std::any& context, HTTPRequest* req, const std::string& strReq);

// General Information Endpoints
bool rest_info_currencies(const std::any& context, HTTPRequest* req, const std::string& strReq);
bool rest_info_stability_status(const std::any& context, HTTPRequest* req, const std::string& strReq);

// Helper functions
bool ParseJSONRequest(HTTPRequest* req, UniValue& json);
bool WriteJSONResponse(HTTPRequest* req, const UniValue& json, enum HTTPStatusCode status = HTTP_OK);
bool WriteErrorResponse(HTTPRequest* req, const std::string& error_code, const std::string& message, enum HTTPStatusCode status = HTTP_BAD_REQUEST);
std::string ExtractPublicKeyFromPath(const std::string& path);
std::string ExtractCountryCodeFromPath(const std::string& path);
std::string ExtractOCurrencyFromPath(const std::string& path);

#endif // BITCOIN_REST_O_MOBILE_API_H

