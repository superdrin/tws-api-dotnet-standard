﻿/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved. This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */

#pragma once
#ifndef eclientsocketbaseimpl_h__INCLUDED
#define eclientsocketbaseimpl_h__INCLUDED

#include "EClientSocketBase.h"

#include "EWrapper.h"
#include "TwsSocketClientErrors.h"
#include "Contract.h"
#include "Order.h"
#include "OrderState.h"
#include "Execution.h"
#include "ScannerSubscription.h"
#include "CommissionReport.h"
#include "EDecoder.h"

#include <sstream>
#include <iomanip>
#include <algorithm>

#include <stdio.h>
#include <string.h>
#include <assert.h>

/////////////////////////////////////////////////////////////////////////////////
// SOCKET CLIENT VERSION CHANGE LOG : Incremented when the format of incomming
//                                    server responses change
/////////////////////////////////////////////////////////////////////////////////
// constants
// 6 = Added parentId to orderStatus
// 7 = The new execDetails event returned for an order filled status and reqExecDetails
//     Also added market depth support.
// 8 = Added 'lastFillPrice' to orderStatus and 'permId' to execDetails
// 9 = Added 'avgCost', 'unrealizedPNL', and 'unrealizedPNL' to updatePortfolio event
// 10 = Added 'serverId' to the 'open order' & 'order status' events.
//      We send back all the API open orders upon connection.
//      Added new methods reqAllOpenOrders, reqAutoOpenOrders()
//      Added FA support - reqExecution has filter.
//                       - reqAccountUpdates takes acct code.
// 11 = Added permId to openOrder event.
// 12 = Added IgnoreRth, hidden, and discretionaryAmt
// 13 = Added GoodAfterTime
// 14 = always send size on bid/ask/last tick
// 15 = send allocation string with open order
// 16 = can receive account name in account and portfolio updates, and fa params in openOrder
// 17 = can receive liquidation field in exec reports, and notAutoAvailable field in mkt data
// 18 = can receive good till date field in open order messages, and send backfill requests
// 19 = can receive new extended order attributes in OPEN_ORDER
// 20 = expects TWS time string on connection after server version >= 20, and parentId in open order
// 21 = can receive bond contract details.
// 22 = can receive price magnifier in contract details
// 23 = support for scanner
// 24 = can receive volatility order parameters in open order messages
// 25 = can receive HMDS query start and end times
// 26 = can receive option vols in option market data messages
// 27 = can receive delta neutral order type and delta neutral aux price
// 28 = can receive option model computation ticks
// 29 = can receive trail stop limit price in open order and can place them: API 8.91
// 30 = can receive extended bond contract def, new ticks, and trade count in bars
// 31 = can receive EFP extensions to scanner and market data, and combo legs on open orders
//    ; can receive RT bars
// 32 = can receive TickType.LAST_TIMESTAMP
// 33 = can receive ScaleNumComponents and ScaleComponentSize is open order messages
// 34 = can receive whatIf orders / order state
// 35 = can receive contId field for Contract objects
// 36 = can receive outsideRth field for Order objects
// 37 = can receive clearingAccount and clearingIntent for Order objects
// 38 = can receive multipier and primaryExchange in portfolio updates
//    ; can receive cumQty and avgPrice in execution
//    ; can receive fundamental data
//    ; can receive underComp for Contract objects
//    ; can receive reqId and end marker in contractDetails/bondContractDetails
//    ; can receive ScaleInitComponentSize and ScaleSubsComponentSize for Order objects
// 39 = can receive underConId in contractDetails
// 40 = can receive algoStrategy/algoParams in openOrder
// 41 = can receive end marker for openOrder
//    ; can receive end marker for account download
//    ; can receive end marker for executions download
// 42 = can receive deltaNeutralValidation
// 43 = can receive longName(companyName)
//    ; can receive listingExchange
//    ; can receive RTVolume tick
// 44 = can receive end market for ticker snapshot
// 45 = can receive notHeld field in openOrder
// 46 = can receive contractMonth, industry, category, subcategory fields in contractDetails
//    ; can receive timeZoneId, tradingHours, liquidHours fields in contractDetails
// 47 = can receive gamma, vega, theta, undPrice fields in TICK_OPTION_COMPUTATION
// 48 = can receive exemptCode in openOrder
// 49 = can receive hedgeType and hedgeParam in openOrder
// 50 = can receive optOutSmartRouting field in openOrder
// 51 = can receive smartComboRoutingParams in openOrder
// 52 = can receive deltaNeutralConId, deltaNeutralSettlingFirm, deltaNeutralClearingAccount and deltaNeutralClearingIntent in openOrder
// 53 = can receive orderRef in execution
// 54 = can receive scale order fields (PriceAdjustValue, PriceAdjustInterval, ProfitOffset, AutoReset, 
//      InitPosition, InitFillQty and RandomPercent) in openOrder
// 55 = can receive orderComboLegs (price) in openOrder
// 56 = can receive trailingPercent in openOrder
// 57 = can receive commissionReport message
// 58 = can receive CUSIP/ISIN/etc. in contractDescription/bondContractDescription
// 59 = can receive evRule, evMultiplier in contractDescription/bondContractDescription/executionDetails
//      can receive multiplier in executionDetails
// 60 = can receive deltaNeutralOpenClose, deltaNeutralShortSale, deltaNeutralShortSaleSlot 
//      and deltaNeutralDesignatedLocation in openOrder
//      can receive position, positionEnd, accountSummary and accountSummaryEnd
// 61 = can receive multiplier in openOrder
//      can receive tradingClass in openOrder, updatePortfolio, execDetails and position
// 62 = can receive avgCost in position message
// 63 = can receive verifyMessageAPI, verifyCompleted, displayGroupList and displayGroupUpdated messages
// 64 = can receive solicited attrib in openOrder message
// 65 = can receive verifyAndAuthMessageAPI and verifyAndAuthCompleted messages

const int CLIENT_VERSION    = 65;
const int SERVER_VERSION    = 38;

/* 100+ messaging */
// 100 = enhanced handshake, msg length prefixes

const int MIN_CLIENT_VER = 100;
const int MAX_CLIENT_VER = 100;

// outgoing msg id's
const int REQ_MKT_DATA                  = 1;
const int CANCEL_MKT_DATA               = 2;
const int PLACE_ORDER                   = 3;
const int CANCEL_ORDER                  = 4;
const int REQ_OPEN_ORDERS               = 5;
const int REQ_ACCT_DATA                 = 6;
const int REQ_EXECUTIONS                = 7;
const int REQ_IDS                       = 8;
const int REQ_CONTRACT_DATA             = 9;
const int REQ_MKT_DEPTH                 = 10;
const int CANCEL_MKT_DEPTH              = 11;
const int REQ_NEWS_BULLETINS            = 12;
const int CANCEL_NEWS_BULLETINS         = 13;
const int SET_SERVER_LOGLEVEL           = 14;
const int REQ_AUTO_OPEN_ORDERS          = 15;
const int REQ_ALL_OPEN_ORDERS           = 16;
const int REQ_MANAGED_ACCTS             = 17;
const int REQ_FA                        = 18;
const int REPLACE_FA                    = 19;
const int REQ_HISTORICAL_DATA           = 20;
const int EXERCISE_OPTIONS              = 21;
const int REQ_SCANNER_SUBSCRIPTION      = 22;
const int CANCEL_SCANNER_SUBSCRIPTION   = 23;
const int REQ_SCANNER_PARAMETERS        = 24;
const int CANCEL_HISTORICAL_DATA        = 25;
const int REQ_CURRENT_TIME              = 49;
const int REQ_REAL_TIME_BARS            = 50;
const int CANCEL_REAL_TIME_BARS         = 51;
const int REQ_FUNDAMENTAL_DATA          = 52;
const int CANCEL_FUNDAMENTAL_DATA       = 53;
const int REQ_CALC_IMPLIED_VOLAT        = 54;
const int REQ_CALC_OPTION_PRICE         = 55;
const int CANCEL_CALC_IMPLIED_VOLAT     = 56;
const int CANCEL_CALC_OPTION_PRICE      = 57;
const int REQ_GLOBAL_CANCEL             = 58;
const int REQ_MARKET_DATA_TYPE          = 59;
const int REQ_POSITIONS                 = 61;
const int REQ_ACCOUNT_SUMMARY           = 62;
const int CANCEL_ACCOUNT_SUMMARY        = 63;
const int CANCEL_POSITIONS              = 64;
const int VERIFY_REQUEST                = 65;
const int VERIFY_MESSAGE                = 66;
const int QUERY_DISPLAY_GROUPS          = 67;
const int SUBSCRIBE_TO_GROUP_EVENTS     = 68;
const int UPDATE_DISPLAY_GROUP          = 69;
const int UNSUBSCRIBE_FROM_GROUP_EVENTS = 70;
const int START_API                     = 71;
const int VERIFY_AND_AUTH_REQUEST       = 72;
const int VERIFY_AND_AUTH_MESSAGE       = 73;

////const int MIN_SERVER_VER_REAL_TIME_BARS       = 34;
////const int MIN_SERVER_VER_SCALE_ORDERS         = 35;
////const int MIN_SERVER_VER_SNAPSHOT_MKT_DATA    = 35;
////const int MIN_SERVER_VER_SSHORT_COMBO_LEGS    = 35;
////const int MIN_SERVER_VER_WHAT_IF_ORDERS       = 36;
////const int MIN_SERVER_VER_CONTRACT_CONID       = 37;
//const int MIN_SERVER_VER_PTA_ORDERS             = 39;
//const int MIN_SERVER_VER_FUNDAMENTAL_DATA       = 40;
//const int MIN_SERVER_VER_UNDER_COMP             = 40;
//const int MIN_SERVER_VER_CONTRACT_DATA_CHAIN    = 40;
//const int MIN_SERVER_VER_SCALE_ORDERS2          = 40;
//const int MIN_SERVER_VER_ALGO_ORDERS            = 41;
//const int MIN_SERVER_VER_EXECUTION_DATA_CHAIN   = 42;
//const int MIN_SERVER_VER_NOT_HELD               = 44;
//const int MIN_SERVER_VER_SEC_ID_TYPE            = 45;
//const int MIN_SERVER_VER_PLACE_ORDER_CONID      = 46;
//const int MIN_SERVER_VER_REQ_MKT_DATA_CONID     = 47;
//const int MIN_SERVER_VER_REQ_CALC_IMPLIED_VOLAT = 49;
//const int MIN_SERVER_VER_REQ_CALC_OPTION_PRICE  = 50;
//const int MIN_SERVER_VER_CANCEL_CALC_IMPLIED_VOLAT = 50;
//const int MIN_SERVER_VER_CANCEL_CALC_OPTION_PRICE  = 50;
//const int MIN_SERVER_VER_SSHORTX_OLD            = 51;
//const int MIN_SERVER_VER_SSHORTX                = 52;
//const int MIN_SERVER_VER_REQ_GLOBAL_CANCEL      = 53;
//const int MIN_SERVER_VER_HEDGE_ORDERS			= 54;
//const int MIN_SERVER_VER_REQ_MARKET_DATA_TYPE	= 55;
//const int MIN_SERVER_VER_OPT_OUT_SMART_ROUTING  = 56;
//const int MIN_SERVER_VER_SMART_COMBO_ROUTING_PARAMS = 57;
//const int MIN_SERVER_VER_DELTA_NEUTRAL_CONID    = 58;
//const int MIN_SERVER_VER_SCALE_ORDERS3          = 60;
//const int MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE = 61;
//const int MIN_SERVER_VER_TRAILING_PERCENT       = 62;
//const int MIN_SERVER_VER_DELTA_NEUTRAL_OPEN_CLOSE = 66;
//const int MIN_SERVER_VER_POSITIONS              = 67;
//const int MIN_SERVER_VER_ACCOUNT_SUMMARY        = 67;
//const int MIN_SERVER_VER_TRADING_CLASS          = 68;
//const int MIN_SERVER_VER_SCALE_TABLE            = 69;
//const int MIN_SERVER_VER_LINKING                = 70;
//const int MIN_SERVER_VER_ALGO_ID                = 71;
//const int MIN_SERVER_VER_OPTIONAL_CAPABILITIES  = 72;
//const int MIN_SERVER_VER_ORDER_SOLICITED        = 73;
//const int MIN_SERVER_VER_LINKING_AUTH           = 74;
//const int MIN_SERVER_VER_PRIMARYEXCH            = 75;

//// incoming msg id's
//const int TICK_PRICE                = 1;
//const int TICK_SIZE                 = 2;
//const int ORDER_STATUS              = 3;
//const int ERR_MSG                   = 4;
//const int OPEN_ORDER                = 5;
//const int ACCT_VALUE                = 6;
//const int PORTFOLIO_VALUE           = 7;
//const int ACCT_UPDATE_TIME          = 8;
//const int NEXT_VALID_ID             = 9;
//const int CONTRACT_DATA             = 10;
//const int EXECUTION_DATA            = 11;
//const int MARKET_DEPTH              = 12;
//const int MARKET_DEPTH_L2           = 13;
//const int NEWS_BULLETINS            = 14;
//const int MANAGED_ACCTS             = 15;
//const int RECEIVE_FA                = 16;
//const int HISTORICAL_DATA           = 17;
//const int BOND_CONTRACT_DATA        = 18;
//const int SCANNER_PARAMETERS        = 19;
//const int SCANNER_DATA              = 20;
//const int TICK_OPTION_COMPUTATION   = 21;
//const int TICK_GENERIC              = 45;
//const int TICK_STRING               = 46;
//const int TICK_EFP                  = 47;
//const int CURRENT_TIME              = 49;
//const int REAL_TIME_BARS            = 50;
//const int FUNDAMENTAL_DATA          = 51;
//const int CONTRACT_DATA_END         = 52;
//const int OPEN_ORDER_END            = 53;
//const int ACCT_DOWNLOAD_END         = 54;
//const int EXECUTION_DATA_END        = 55;
//const int DELTA_NEUTRAL_VALIDATION  = 56;
//const int TICK_SNAPSHOT_END         = 57;
//const int MARKET_DATA_TYPE          = 58;
//const int COMMISSION_REPORT         = 59;
//const int POSITION_DATA             = 61;
//const int POSITION_END              = 62;
//const int ACCOUNT_SUMMARY           = 63;
//const int ACCOUNT_SUMMARY_END       = 64;
//const int VERIFY_MESSAGE_API        = 65;
//const int VERIFY_COMPLETED          = 66;
//const int DISPLAY_GROUP_LIST        = 67;
//const int DISPLAY_GROUP_UPDATED     = 68;
//const int VERIFY_AND_AUTH_MESSAGE_API = 69;
//const int VERIFY_AND_AUTH_COMPLETED   = 70;

// TWS New Bulletins constants
const int NEWS_MSG              = 1;    // standard IB news bulleting message
const int EXCHANGE_AVAIL_MSG    = 2;    // control message specifing that an exchange is available for trading
const int EXCHANGE_UNAVAIL_MSG  = 3;    // control message specifing that an exchange is unavailable for trading


///////////////////////////////////////////////////////////
// helper macroses
//#define DECODE_FIELD(x) if (!DecodeField(x, ptr, endPtr)) return 0;
//#define DECODE_FIELD_MAX(x) if (!DecodeFieldMax(x, ptr, endPtr)) return 0;

#define ENCODE_FIELD(x) EncodeField(msg, x);
#define ENCODE_FIELD_MAX(x) EncodeFieldMax(msg, x);

///////////////////////////////////////////////////////////
//// helper structures
//namespace {
//
//struct BarData {
//	std::string date;
//	double open;
//	double high;
//	double low;
//	double close;
//	int volume;
//	double average;
//	std::string hasGaps;
//	int barCount;
//};
//
//struct ScanData {
//	ContractDetails contract;
//	int rank;
//	std::string distance;
//	std::string benchmark;
//	std::string projection;
//	std::string legsStr;
//};
//
//} // end of anonymous namespace

///////////////////////////////////////////////////////////
// encoders
template<class T>
void EClientSocketBase::EncodeField(std::ostream& os, T value)
{
	os << value << '\0';
}

template<>
void EClientSocketBase::EncodeField<bool>(std::ostream& os, bool boolValue)
{
	EncodeField<int>(os, boolValue ? 1 : 0);
}

template<>
void EClientSocketBase::EncodeField<double>(std::ostream& os, double doubleValue)
{
	char str[128];

	snprintf(str, sizeof(str), "%.10g", doubleValue);

	EncodeField<const char*>(os, str);
}

///////////////////////////////////////////////////////////
// "max" encoders
void EClientSocketBase::EncodeFieldMax(std::ostream& os, int intValue)
{
	if( intValue == INT_MAX) {
		EncodeField(os, "");
		return;
	}
	EncodeField(os, intValue);
}

void EClientSocketBase::EncodeFieldMax(std::ostream& os, double doubleValue)
{
	if( doubleValue == DBL_MAX) {
		EncodeField(os, "");
		return;
	}
	EncodeField(os, doubleValue);
}

///////////////////////////////////////////////////////////
// decoders
//bool EClientSocketBase::CheckOffset(const char* ptr, const char* endPtr)
//{
//	assert (ptr && ptr <= endPtr);
//	return (ptr && ptr < endPtr);
//}
//
//const char* EClientSocketBase::FindFieldEnd(const char* ptr, const char* endPtr)
//{
//	return (const char*)memchr(ptr, 0, endPtr - ptr);
//}

//bool EClientSocketBase::DecodeField(bool& boolValue, const char*& ptr, const char* endPtr)
//{
//	int intValue;
//	if( !DecodeField(intValue, ptr, endPtr))
//		return false;
//	boolValue = (intValue > 0);
//	return true;
//}
//
//bool EClientSocketBase::DecodeField(int& intValue, const char*& ptr, const char* endPtr)
//{
//	if( !CheckOffset(ptr, endPtr))
//		return false;
//	const char* fieldBeg = ptr;
//	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
//	if( !fieldEnd)
//		return false;
//	intValue = atoi(fieldBeg);
//	ptr = ++fieldEnd;
//	return true;
//}
//
//bool EClientSocketBase::DecodeField(long& longValue, const char*& ptr, const char* endPtr)
//{
//	int intValue;
//	if( !DecodeField(intValue, ptr, endPtr))
//		return false;
//	longValue = intValue;
//	return true;
//}
//
//bool EClientSocketBase::DecodeField(double& doubleValue, const char*& ptr, const char* endPtr)
//{
//	if( !CheckOffset(ptr, endPtr))
//		return false;
//	const char* fieldBeg = ptr;
//	const char* fieldEnd = FindFieldEnd(fieldBeg, endPtr);
//	if( !fieldEnd)
//		return false;
//	doubleValue = atof(fieldBeg);
//	ptr = ++fieldEnd;
//	return true;
//}
//
//bool EClientSocketBase::DecodeField(std::string& stringValue,
//								const char*& ptr, const char* endPtr)
//{
//	if( !CheckOffset(ptr, endPtr))
//		return false;
//	const char* fieldBeg = ptr;
//	const char* fieldEnd = FindFieldEnd(ptr, endPtr);
//	if( !fieldEnd)
//		return false;
//	stringValue = fieldBeg; // better way?
//	ptr = ++fieldEnd;
//	return true;
//}
//
//bool EClientSocketBase::DecodeFieldMax(int& intValue, const char*& ptr, const char* endPtr)
//{
//	std::string stringValue;
//	if( !DecodeField(stringValue, ptr, endPtr))
//		return false;
//	intValue = stringValue.empty() ? UNSET_INTEGER : atoi(stringValue.c_str());
//	return true;
//}
//
//bool EClientSocketBase::DecodeFieldMax(long& longValue, const char*& ptr, const char* endPtr)
//{
//	int intValue;
//	if( !DecodeFieldMax(intValue, ptr, endPtr))
//		return false;
//	longValue = intValue;
//	return true;
//}
//
//bool EClientSocketBase::DecodeFieldMax(double& doubleValue, const char*& ptr, const char* endPtr)
//{
//	std::string stringValue;
//	if( !DecodeField(stringValue, ptr, endPtr))
//		return false;
//	doubleValue = stringValue.empty() ? UNSET_DOUBLE : atof(stringValue.c_str());
//	return true;
//}

///////////////////////////////////////////////////////////
// static helpers

static const size_t BufferSizeHighMark = 1 * 1024 * 1024; // 1Mb

void EClientSocketBase::CleanupBuffer(BytesVec& buffer, int processed)
{
	assert( buffer.empty() || processed <= (int)buffer.size());

	if( buffer.empty())
		return;

	if( processed <= 0)
		return;

	if( (size_t)processed == buffer.size()) {
		if( buffer.capacity() >= BufferSizeHighMark) {
			BytesVec().swap(buffer);
		}
		else {
			buffer.clear();
		}
	}
	else {
		buffer.erase( buffer.begin(), buffer.begin() + processed);
	}
};


///////////////////////////////////////////////////////////
// member funcs
EClientSocketBase::EClientSocketBase( EWrapper *ptr)
	: m_pEWrapper(ptr)
	, m_clientId(-1)
	, m_connState(CS_DISCONNECTED)
	, m_extraAuth(false)
	, m_serverVersion(0)
	, m_useV100Plus(false)
{
}

EClientSocketBase::~EClientSocketBase()
{
}

EClientSocketBase::ConnState EClientSocketBase::connState() const
{
	return m_connState;
}

bool EClientSocketBase::isConnected() const
{
	return m_connState == CS_CONNECTED;
}

bool EClientSocketBase::isConnecting() const
{
	return m_connState == CS_CONNECTING;
}

void EClientSocketBase::eConnectBase()
{
}

void EClientSocketBase::eDisconnectBase()
{
	m_TwsTime.clear();
	m_serverVersion = 0;
	m_connState = CS_DISCONNECTED;
	m_extraAuth = false;
	m_clientId = -1;
	m_outBuffer.clear();
	m_inBuffer.clear();
}

int EClientSocketBase::serverVersion()
{
	return m_serverVersion;
}

std::string EClientSocketBase::TwsConnectionTime()
{
	return m_TwsTime;
}

const std::string& EClientSocketBase::optionalCapabilities() const
{
	return m_optionalCapabilities;
}

void EClientSocketBase::setOptionalCapabilities(const std::string& optCapts)
{
	m_optionalCapabilities = optCapts;
}

void EClientSocketBase::setUseV100Plus(const std::string& connectOptions)
{
	if( isSocketOK()) {
		m_pEWrapper->error( NO_VALID_ID, ALREADY_CONNECTED.code(), ALREADY_CONNECTED.msg());
		return;
	}
	m_useV100Plus = true;
	m_connectOptions = connectOptions;
}

bool EClientSocketBase::usingV100Plus() {
    return m_useV100Plus;
}

void EClientSocketBase::reqMktData(TickerId tickerId, const Contract& contract,
							   const std::string& genericTicks, bool snapshot, const TagValueListSPtr& mktDataOptions)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// not needed anymore validation
	//if( m_serverVersion < MIN_SERVER_VER_SNAPSHOT_MKT_DATA && snapshot) {
	//	m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support snapshot market data requests.");
	//	return;
	//}

	if( m_serverVersion < MIN_SERVER_VER_UNDER_COMP) {
		if( contract.underComp) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support delta-neutral orders.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_REQ_MKT_DATA_CONID) {
		if( contract.conId > 0) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId parameter.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty() ) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support tradingClass parameter in reqMktData.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 11;

	// send req mkt data msg
	ENCODE_FIELD( REQ_MKT_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_REQ_MKT_DATA_CONID) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier); // srv v15 and above

	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange); // srv v14 and above
	ENCODE_FIELD( contract.currency);

	ENCODE_FIELD( contract.localSymbol); // srv v2 and above

	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}

	// Send combo legs for BAG requests (srv v8 and above)
	if( contract.secType == "BAG")
	{
		const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
		const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
		ENCODE_FIELD( comboLegsCount);
		if( comboLegsCount > 0) {
			for( int i = 0; i < comboLegsCount; ++i) {
				const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
				assert( comboLeg);
				ENCODE_FIELD( comboLeg->conId);
				ENCODE_FIELD( comboLeg->ratio);
				ENCODE_FIELD( comboLeg->action);
				ENCODE_FIELD( comboLeg->exchange);
			}
		}
	}

	if( m_serverVersion >= MIN_SERVER_VER_UNDER_COMP) {
		if( contract.underComp) {
			const UnderComp& underComp = *contract.underComp;
			ENCODE_FIELD( true);
			ENCODE_FIELD( underComp.conId);
			ENCODE_FIELD( underComp.delta);
			ENCODE_FIELD( underComp.price);
		}
		else {
			ENCODE_FIELD( false);
		}
	}

	ENCODE_FIELD( genericTicks); // srv v31 and above
	ENCODE_FIELD( snapshot); // srv v35 and above

	// send mktDataOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string mktDataOptionsStr("");
		const int mktDataOptionsCount = mktDataOptions.get() ? mktDataOptions->size() : 0;
		if( mktDataOptionsCount > 0) {
			for( int i = 0; i < mktDataOptionsCount; ++i) {
				const TagValue* tagValue = ((*mktDataOptions)[i]).get();
				mktDataOptionsStr += tagValue->tag;
				mktDataOptionsStr += "=";
				mktDataOptionsStr += tagValue->value;
				mktDataOptionsStr += ";";
			}
		}
		ENCODE_FIELD( mktDataOptionsStr);
	}

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelMktData(TickerId tickerId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	// send cancel mkt data msg
	ENCODE_FIELD( CANCEL_MKT_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqMktDepth( TickerId tickerId, const Contract& contract, int numRows, const TagValueListSPtr& mktDepthOptions)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	// This feature is only available for versions of TWS >=6
	//if( m_serverVersion < 6) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty() || (contract.conId > 0)) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId and tradingClass parameters in reqMktDepth.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 5;

	// send req mkt data msg
	ENCODE_FIELD( REQ_MKT_DEPTH);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier); // srv v15 and above
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}

	ENCODE_FIELD( numRows); // srv v19 and above

	// send mktDepthOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string mktDepthOptionsStr("");
		const int mktDepthOptionsCount = mktDepthOptions.get() ? mktDepthOptions->size() : 0;
		if( mktDepthOptionsCount > 0) {
			for( int i = 0; i < mktDepthOptionsCount; ++i) {
				const TagValue* tagValue = ((*mktDepthOptions)[i]).get();
				mktDepthOptionsStr += tagValue->tag;
				mktDepthOptionsStr += "=";
				mktDepthOptionsStr += tagValue->value;
				mktDepthOptionsStr += ";";
			}
		}
		ENCODE_FIELD( mktDepthOptionsStr);
	}

	closeAndSend( msg.str());
}


void EClientSocketBase::cancelMktDepth( TickerId tickerId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	// This feature is only available for versions of TWS >=6
	//if( m_serverVersion < 6) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send cancel mkt data msg
	ENCODE_FIELD( CANCEL_MKT_DEPTH);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqHistoricalData( TickerId tickerId, const Contract& contract,
									   const std::string& endDateTime, const std::string& durationStr,
									   const std::string&  barSizeSetting, const std::string& whatToShow,
									   int useRTH, int formatDate, const TagValueListSPtr& chartOptions)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 16) {
	//	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty() || (contract.conId > 0)) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId and tradingClass parameters in reqHistoricalData.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 6;

	ENCODE_FIELD( REQ_HISTORICAL_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}
	ENCODE_FIELD( contract.includeExpired); // srv v31 and above

	ENCODE_FIELD( endDateTime); // srv v20 and above
	ENCODE_FIELD( barSizeSetting); // srv v20 and above

	ENCODE_FIELD( durationStr);
	ENCODE_FIELD( useRTH);
	ENCODE_FIELD( whatToShow);
	ENCODE_FIELD( formatDate); // srv v16 and above

	// Send combo legs for BAG requests
	if( contract.secType == "BAG")
	{
		const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
		const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
		ENCODE_FIELD( comboLegsCount);
		if( comboLegsCount > 0) {
			for( int i = 0; i < comboLegsCount; ++i) {
				const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
				assert( comboLeg);
				ENCODE_FIELD( comboLeg->conId);
				ENCODE_FIELD( comboLeg->ratio);
				ENCODE_FIELD( comboLeg->action);
				ENCODE_FIELD( comboLeg->exchange);
			}
		}
	}

	// send chartOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string chartOptionsStr("");
		const int chartOptionsCount = chartOptions.get() ? chartOptions->size() : 0;
		if( chartOptionsCount > 0) {
			for( int i = 0; i < chartOptionsCount; ++i) {
				const TagValue* tagValue = ((*chartOptions)[i]).get();
				chartOptionsStr += tagValue->tag;
				chartOptionsStr += "=";
				chartOptionsStr += tagValue->value;
				chartOptionsStr += ";";
			}
		}
		ENCODE_FIELD( chartOptionsStr);
	}

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelHistoricalData(TickerId tickerId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 24) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support historical data query cancellation.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_HISTORICAL_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqRealTimeBars(TickerId tickerId, const Contract& contract,
									int barSize, const std::string& whatToShow, bool useRTH,
									const TagValueListSPtr& realTimeBarsOptions)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < MIN_SERVER_VER_REAL_TIME_BARS) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support real time bars.");
	//	return;
	//}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty() || (contract.conId > 0)) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId and tradingClass parameters in reqRealTimeBars.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 3;

	ENCODE_FIELD( REQ_REAL_TIME_BARS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}
	ENCODE_FIELD( barSize);
	ENCODE_FIELD( whatToShow);
	ENCODE_FIELD( useRTH);

	// send realTimeBarsOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string realTimeBarsOptionsStr("");
		const int realTimeBarsOptionsCount = realTimeBarsOptions.get() ? realTimeBarsOptions->size() : 0;
		if( realTimeBarsOptionsCount > 0) {
			for( int i = 0; i < realTimeBarsOptionsCount; ++i) {
				const TagValue* tagValue = ((*realTimeBarsOptions)[i]).get();
				realTimeBarsOptionsStr += tagValue->tag;
				realTimeBarsOptionsStr += "=";
				realTimeBarsOptionsStr += tagValue->value;
				realTimeBarsOptionsStr += ";";
			}
		}
		ENCODE_FIELD( realTimeBarsOptionsStr);
	}

	closeAndSend( msg.str());
}


void EClientSocketBase::cancelRealTimeBars(TickerId tickerId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < MIN_SERVER_VER_REAL_TIME_BARS) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support realtime bar data query cancellation.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_REAL_TIME_BARS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	closeAndSend( msg.str());
}


void EClientSocketBase::reqScannerParameters()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 24) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support API scanner subscription.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REQ_SCANNER_PARAMETERS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}


void EClientSocketBase::reqScannerSubscription(int tickerId,
	const ScannerSubscription& subscription, const TagValueListSPtr& scannerSubscriptionOptions)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 24) {
	//	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support API scanner subscription.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 4;

	ENCODE_FIELD( REQ_SCANNER_SUBSCRIPTION);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);
	ENCODE_FIELD_MAX( subscription.numberOfRows);
	ENCODE_FIELD( subscription.instrument);
	ENCODE_FIELD( subscription.locationCode);
	ENCODE_FIELD( subscription.scanCode);
	ENCODE_FIELD_MAX( subscription.abovePrice);
	ENCODE_FIELD_MAX( subscription.belowPrice);
	ENCODE_FIELD_MAX( subscription.aboveVolume);
	ENCODE_FIELD_MAX( subscription.marketCapAbove);
	ENCODE_FIELD_MAX( subscription.marketCapBelow);
	ENCODE_FIELD( subscription.moodyRatingAbove);
	ENCODE_FIELD( subscription.moodyRatingBelow);
	ENCODE_FIELD( subscription.spRatingAbove);
	ENCODE_FIELD( subscription.spRatingBelow);
	ENCODE_FIELD( subscription.maturityDateAbove);
	ENCODE_FIELD( subscription.maturityDateBelow);
	ENCODE_FIELD_MAX( subscription.couponRateAbove);
	ENCODE_FIELD_MAX( subscription.couponRateBelow);
	ENCODE_FIELD_MAX( subscription.excludeConvertible);
	ENCODE_FIELD_MAX( subscription.averageOptionVolumeAbove); // srv v25 and above
	ENCODE_FIELD( subscription.scannerSettingPairs); // srv v25 and above
	ENCODE_FIELD( subscription.stockTypeFilter); // srv v27 and above

	// send scannerSubscriptionOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string scannerSubscriptionOptionsStr("");
		const int scannerSubscriptionOptionsCount = scannerSubscriptionOptions.get() ? scannerSubscriptionOptions->size() : 0;
		if( scannerSubscriptionOptionsCount > 0) {
			for( int i = 0; i < scannerSubscriptionOptionsCount; ++i) {
				const TagValue* tagValue = ((*scannerSubscriptionOptions)[i]).get();
				scannerSubscriptionOptionsStr += tagValue->tag;
				scannerSubscriptionOptionsStr += "=";
				scannerSubscriptionOptionsStr += tagValue->value;
				scannerSubscriptionOptionsStr += ";";
			}
		}
		ENCODE_FIELD( scannerSubscriptionOptionsStr);
	}

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelScannerSubscription(int tickerId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( tickerId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 24) {
	//	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support API scanner subscription.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_SCANNER_SUBSCRIPTION);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqFundamentalData(TickerId reqId, const Contract& contract, 
										   const std::string& reportType)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( reqId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_FUNDAMENTAL_DATA) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support fundamental data requests.");
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( contract.conId > 0) {
			m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId parameter in reqFundamentalData.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	ENCODE_FIELD( REQ_FUNDAMENTAL_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);

	ENCODE_FIELD( reportType);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelFundamentalData( TickerId reqId)
{
		// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( reqId, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_FUNDAMENTAL_DATA) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support fundamental data requests.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_FUNDAMENTAL_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

void EClientSocketBase::calculateImpliedVolatility(TickerId reqId, const Contract& contract, double optionPrice, double underPrice) {

	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_REQ_CALC_IMPLIED_VOLAT) {
		m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support calculate implied volatility requests.");
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty()) {
			m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support tradingClass parameter in calculateImpliedVolatility.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	ENCODE_FIELD( REQ_CALC_IMPLIED_VOLAT);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	// send contract fields
	ENCODE_FIELD( contract.conId);
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}

	ENCODE_FIELD( optionPrice);
	ENCODE_FIELD( underPrice);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelCalculateImpliedVolatility(TickerId reqId) {

	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_CANCEL_CALC_IMPLIED_VOLAT) {
		m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support calculate implied volatility cancellation.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_CALC_IMPLIED_VOLAT);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

void EClientSocketBase::calculateOptionPrice(TickerId reqId, const Contract& contract, double volatility, double underPrice) {

	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_REQ_CALC_OPTION_PRICE) {
		m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support calculate option price requests.");
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty()) {
			m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support tradingClass parameter in calculateOptionPrice.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	ENCODE_FIELD( REQ_CALC_OPTION_PRICE);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	// send contract fields
	ENCODE_FIELD( contract.conId);
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}

	ENCODE_FIELD( volatility);
	ENCODE_FIELD( underPrice);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelCalculateOptionPrice(TickerId reqId) {

	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_CANCEL_CALC_OPTION_PRICE) {
		m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support calculate option price cancellation.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_CALC_OPTION_PRICE);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqContractDetails( int reqId, const Contract& contract)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	// This feature is only available for versions of TWS >=4
	//if( m_serverVersion < 4) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}
	if (m_serverVersion < MIN_SERVER_VER_SEC_ID_TYPE) {
		if( !contract.secIdType.empty() || !contract.secId.empty()) {
			m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
     			"  It does not support secIdType and secId parameters.");
     		return;
     	}
    }
	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty()) {
			m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support tradingClass parameter in reqContractDetails.");
			return;
		}
	}
    if (m_serverVersion < MIN_SERVER_VER_LINKING) {
        if (!contract.primaryExchange.empty()) {
            m_pEWrapper->error( reqId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
                "  It does not support primaryExchange parameter in reqContractDetails.");
            return;
    }
    }

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 8;

	// send req mkt data msg
	ENCODE_FIELD( REQ_CONTRACT_DATA);
	ENCODE_FIELD( VERSION);

	if( m_serverVersion >= MIN_SERVER_VER_CONTRACT_DATA_CHAIN) {
		ENCODE_FIELD( reqId);
	}

	// send contract fields
	ENCODE_FIELD( contract.conId); // srv v37 and above
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier); // srv v15 and above

    if (m_serverVersion >= MIN_SERVER_VER_PRIMARYEXCH)
    {
        ENCODE_FIELD(contract.exchange);
        ENCODE_FIELD(contract.primaryExchange);
    }
    else if (m_serverVersion >= MIN_SERVER_VER_LINKING)
    {
        if (!contract.primaryExchange.empty() && (contract.exchange == "BEST" || contract.exchange == "SMART"))
        {
            ENCODE_FIELD( contract.exchange + ":" + contract.primaryExchange);
        }
        else
        {
            ENCODE_FIELD(contract.exchange);
        }
    }

	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}
	ENCODE_FIELD( contract.includeExpired); // srv v31 and above

	if( m_serverVersion >= MIN_SERVER_VER_SEC_ID_TYPE){
		ENCODE_FIELD( contract.secIdType);
		ENCODE_FIELD( contract.secId);
	}

	closeAndSend( msg.str());
}

void EClientSocketBase::reqCurrentTime()
{
    // not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	// This feature is only available for versions of TWS >= 33
	//if( m_serverVersion < 33) {
	//	m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//		"  It does not support current time requests.");
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send current time req
	ENCODE_FIELD( REQ_CURRENT_TIME);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::placeOrder( OrderId id, const Contract& contract, const Order& order)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( id, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS) {
	//	if( order.scaleNumComponents != UNSET_INTEGER ||
	//		order.scaleComponentSize != UNSET_INTEGER ||
	//		order.scalePriceIncrement != UNSET_DOUBLE) {
	//		m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//			"  It does not support Scale orders.");
	//		return;
	//	}
	//}
	//
	//if( m_serverVersion < MIN_SERVER_VER_SSHORT_COMBO_LEGS) {
	//	if( contract.comboLegs && !contract.comboLegs->empty()) {
	//		typedef Contract::ComboLegList ComboLegList;
	//		const ComboLegList& comboLegs = *contract.comboLegs;
	//		ComboLegList::const_iterator iter = comboLegs.begin();
	//		const ComboLegList::const_iterator iterEnd = comboLegs.end();
	//		for( ; iter != iterEnd; ++iter) {
	//			const ComboLeg* comboLeg = *iter;
	//			assert( comboLeg);
	//			if( comboLeg->shortSaleSlot != 0 ||
	//				!comboLeg->designatedLocation.IsEmpty()) {
	//				m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//					"  It does not support SSHORT flag for combo legs.");
	//				return;
	//			}
	//		}
	//	}
	//}
	//
	//if( m_serverVersion < MIN_SERVER_VER_WHAT_IF_ORDERS) {
	//	if( order.whatIf) {
	//		m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
	//			"  It does not support what-if orders.");
	//		return;
	//	}
	//}

	if( m_serverVersion < MIN_SERVER_VER_UNDER_COMP) {
		if( contract.underComp) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support delta-neutral orders.");
			return;
		}
	}

	if( m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS2) {
		if( order.scaleSubsLevelSize != UNSET_INTEGER) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support Subsequent Level Size for Scale orders.");
			return;
		}
	}

	if( m_serverVersion < MIN_SERVER_VER_ALGO_ORDERS) {

		if( !order.algoStrategy.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support algo orders.");
			return;
		}
	}

	if( m_serverVersion < MIN_SERVER_VER_NOT_HELD) {
		if (order.notHeld) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support notHeld parameter.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_SEC_ID_TYPE) {
		if( !contract.secIdType.empty() || !contract.secId.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
     			"  It does not support secIdType and secId parameters.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_PLACE_ORDER_CONID) {
		if( contract.conId > 0) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
     			"  It does not support conId parameter.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_SSHORTX) {
		if( order.exemptCode != -1) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support exemptCode parameter.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_SSHORTX) {
		const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
		const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
		for( int i = 0; i < comboLegsCount; ++i) {
			const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
			assert( comboLeg);
			if( comboLeg->exemptCode != -1 ){
				m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support exemptCode parameter.");
				return;
			}
		}
	}

	if( m_serverVersion < MIN_SERVER_VER_HEDGE_ORDERS) {
		if( !order.hedgeType.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
     			"  It does not support hedge orders.");
			return;
		}
	}

	if( m_serverVersion < MIN_SERVER_VER_OPT_OUT_SMART_ROUTING) {
		if (order.optOutSmartRouting) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support optOutSmartRouting parameter.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL_CONID) {
		if (order.deltaNeutralConId > 0 
				|| !order.deltaNeutralSettlingFirm.empty()
				|| !order.deltaNeutralClearingAccount.empty()
				|| !order.deltaNeutralClearingIntent.empty()
				) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support deltaNeutral parameters: ConId, SettlingFirm, ClearingAccount, ClearingIntent.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_DELTA_NEUTRAL_OPEN_CLOSE) {
		if (!order.deltaNeutralOpenClose.empty()
				|| order.deltaNeutralShortSale
				|| order.deltaNeutralShortSaleSlot > 0 
				|| !order.deltaNeutralDesignatedLocation.empty()
				) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() + 
				"  It does not support deltaNeutral parameters: OpenClose, ShortSale, ShortSaleSlot, DesignatedLocation.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_SCALE_ORDERS3) {
		if (order.scalePriceIncrement > 0 && order.scalePriceIncrement != UNSET_DOUBLE) {
			if (order.scalePriceAdjustValue != UNSET_DOUBLE 
				|| order.scalePriceAdjustInterval != UNSET_INTEGER 
				|| order.scaleProfitOffset != UNSET_DOUBLE 
				|| order.scaleAutoReset 
				|| order.scaleInitPosition != UNSET_INTEGER 
				|| order.scaleInitFillQty != UNSET_INTEGER 
				|| order.scaleRandomPercent) {
				m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
						"  It does not support Scale order parameters: PriceAdjustValue, PriceAdjustInterval, " +
						"ProfitOffset, AutoReset, InitPosition, InitFillQty and RandomPercent");
				return;
			}
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE && contract.secType == "BAG") {
		const Order::OrderComboLegList* const orderComboLegs = order.orderComboLegs.get();
		const int orderComboLegsCount = orderComboLegs ? orderComboLegs->size() : 0;
		for( int i = 0; i < orderComboLegsCount; ++i) {
			const OrderComboLeg* orderComboLeg = ((*orderComboLegs)[i]).get();
			assert( orderComboLeg);
			if( orderComboLeg->price != UNSET_DOUBLE) {
				m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support per-leg prices for order combo legs.");
				return;
			}
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_TRAILING_PERCENT) {
		if (order.trailingPercent != UNSET_DOUBLE) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support trailing percent parameter");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support tradingClass parameter in placeOrder.");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_SCALE_TABLE) {
		if( !order.scaleTable.empty() || !order.activeStartTime.empty() || !order.activeStopTime.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support scaleTable, activeStartTime and activeStopTime parameters");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_ALGO_ID) {
		if( !order.algoId.empty()) {
			m_pEWrapper->error( id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support algoId parameter");
			return;
		}
	}

	if (m_serverVersion < MIN_SERVER_VER_ORDER_SOLICITED) {
		if (order.solicited) {
			m_pEWrapper->error(id, UPDATE_TWS.code(), UPDATE_TWS.msg() +
					"  It does not support order solicited parameter.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	int VERSION = (m_serverVersion < MIN_SERVER_VER_NOT_HELD) ? 27 : 44;

	// send place order msg
	ENCODE_FIELD( PLACE_ORDER);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( id);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_PLACE_ORDER_CONID) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier); // srv v15 and above
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.primaryExchange); // srv v14 and above
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol); // srv v2 and above
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}

	if( m_serverVersion >= MIN_SERVER_VER_SEC_ID_TYPE){
		ENCODE_FIELD( contract.secIdType);
		ENCODE_FIELD( contract.secId);
	}

	// send main order fields
	ENCODE_FIELD( order.action);
	ENCODE_FIELD( order.totalQuantity);
	ENCODE_FIELD( order.orderType);
	if( m_serverVersion < MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE) {
		ENCODE_FIELD( order.lmtPrice == UNSET_DOUBLE ? 0 : order.lmtPrice);
	}
	else {
		ENCODE_FIELD_MAX( order.lmtPrice);
	}
	if( m_serverVersion < MIN_SERVER_VER_TRAILING_PERCENT) {
		ENCODE_FIELD( order.auxPrice == UNSET_DOUBLE ? 0 : order.auxPrice);
	}
	else {
		ENCODE_FIELD_MAX( order.auxPrice);
	}

	// send extended order fields
	ENCODE_FIELD( order.tif);
	ENCODE_FIELD( order.ocaGroup);
	ENCODE_FIELD( order.account);
	ENCODE_FIELD( order.openClose);
	ENCODE_FIELD( order.origin);
	ENCODE_FIELD( order.orderRef);
	ENCODE_FIELD( order.transmit);
	ENCODE_FIELD( order.parentId); // srv v4 and above

	ENCODE_FIELD( order.blockOrder); // srv v5 and above
	ENCODE_FIELD( order.sweepToFill); // srv v5 and above
	ENCODE_FIELD( order.displaySize); // srv v5 and above
	ENCODE_FIELD( order.triggerMethod); // srv v5 and above

	//if( m_serverVersion < 38) {
	// will never happen
	//	ENCODE_FIELD(/* order.ignoreRth */ false);
	//}
	//else {
		ENCODE_FIELD( order.outsideRth); // srv v5 and above
	//}

	ENCODE_FIELD( order.hidden); // srv v7 and above

	// Send combo legs for BAG requests (srv v8 and above)
	if( contract.secType == "BAG")
	{
		const Contract::ComboLegList* const comboLegs = contract.comboLegs.get();
		const int comboLegsCount = comboLegs ? comboLegs->size() : 0;
		ENCODE_FIELD( comboLegsCount);
		if( comboLegsCount > 0) {
			for( int i = 0; i < comboLegsCount; ++i) {
				const ComboLeg* comboLeg = ((*comboLegs)[i]).get();
				assert( comboLeg);
				ENCODE_FIELD( comboLeg->conId);
				ENCODE_FIELD( comboLeg->ratio);
				ENCODE_FIELD( comboLeg->action);
				ENCODE_FIELD( comboLeg->exchange);
				ENCODE_FIELD( comboLeg->openClose);

				ENCODE_FIELD( comboLeg->shortSaleSlot); // srv v35 and above
				ENCODE_FIELD( comboLeg->designatedLocation); // srv v35 and above
				if (m_serverVersion >= MIN_SERVER_VER_SSHORTX_OLD) { 
					ENCODE_FIELD( comboLeg->exemptCode);
				}
			}
		}
	}

	// Send order combo legs for BAG requests
	if( m_serverVersion >= MIN_SERVER_VER_ORDER_COMBO_LEGS_PRICE && contract.secType == "BAG")
	{
		const Order::OrderComboLegList* const orderComboLegs = order.orderComboLegs.get();
		const int orderComboLegsCount = orderComboLegs ? orderComboLegs->size() : 0;
		ENCODE_FIELD( orderComboLegsCount);
		if( orderComboLegsCount > 0) {
			for( int i = 0; i < orderComboLegsCount; ++i) {
				const OrderComboLeg* orderComboLeg = ((*orderComboLegs)[i]).get();
				assert( orderComboLeg);
				ENCODE_FIELD_MAX( orderComboLeg->price);
			}
		}
	}	

	if( m_serverVersion >= MIN_SERVER_VER_SMART_COMBO_ROUTING_PARAMS && contract.secType == "BAG") {
		const TagValueList* const smartComboRoutingParams = order.smartComboRoutingParams.get();
		const int smartComboRoutingParamsCount = smartComboRoutingParams ? smartComboRoutingParams->size() : 0;
		ENCODE_FIELD( smartComboRoutingParamsCount);
		if( smartComboRoutingParamsCount > 0) {
			for( int i = 0; i < smartComboRoutingParamsCount; ++i) {
				const TagValue* tagValue = ((*smartComboRoutingParams)[i]).get();
				ENCODE_FIELD( tagValue->tag);
				ENCODE_FIELD( tagValue->value);
			}
		}
	}

	/////////////////////////////////////////////////////////////////////////////
	// Send the shares allocation.
	//
	// This specifies the number of order shares allocated to each Financial
	// Advisor managed account. The format of the allocation string is as
	// follows:
	//			<account_code1>/<number_shares1>,<account_code2>/<number_shares2>,...N
	// E.g.
	//		To allocate 20 shares of a 100 share order to account 'U101' and the
	//      residual 80 to account 'U203' enter the following share allocation string:
	//          U101/20,U203/80
	/////////////////////////////////////////////////////////////////////////////
	{
		// send deprecated sharesAllocation field
		ENCODE_FIELD( ""); // srv v9 and above
	}

	ENCODE_FIELD( order.discretionaryAmt); // srv v10 and above
	ENCODE_FIELD( order.goodAfterTime); // srv v11 and above
	ENCODE_FIELD( order.goodTillDate); // srv v12 and above

	ENCODE_FIELD( order.faGroup); // srv v13 and above
	ENCODE_FIELD( order.faMethod); // srv v13 and above
	ENCODE_FIELD( order.faPercentage); // srv v13 and above
	ENCODE_FIELD( order.faProfile); // srv v13 and above

	// institutional short saleslot data (srv v18 and above)
	ENCODE_FIELD( order.shortSaleSlot);      // 0 for retail, 1 or 2 for institutions
	ENCODE_FIELD( order.designatedLocation); // populate only when shortSaleSlot = 2.
	if (m_serverVersion >= MIN_SERVER_VER_SSHORTX_OLD) { 
		ENCODE_FIELD( order.exemptCode);
	}

	// not needed anymore
	//bool isVolOrder = (order.orderType.CompareNoCase("VOL") == 0);

	// srv v19 and above fields
	ENCODE_FIELD( order.ocaType);
	//if( m_serverVersion < 38) {
	// will never happen
	//	send( /* order.rthOnly */ false);
	//}
	ENCODE_FIELD( order.rule80A);
	ENCODE_FIELD( order.settlingFirm);
	ENCODE_FIELD( order.allOrNone);
	ENCODE_FIELD_MAX( order.minQty);
	ENCODE_FIELD_MAX( order.percentOffset);
	ENCODE_FIELD( order.eTradeOnly);
	ENCODE_FIELD( order.firmQuoteOnly);
	ENCODE_FIELD_MAX( order.nbboPriceCap);
	ENCODE_FIELD( order.auctionStrategy); // AUCTION_MATCH, AUCTION_IMPROVEMENT, AUCTION_TRANSPARENT
	ENCODE_FIELD_MAX( order.startingPrice);
	ENCODE_FIELD_MAX( order.stockRefPrice);
	ENCODE_FIELD_MAX( order.delta);
	// Volatility orders had specific watermark price attribs in server version 26
	//double lower = (m_serverVersion == 26 && isVolOrder) ? DBL_MAX : order.stockRangeLower;
	//double upper = (m_serverVersion == 26 && isVolOrder) ? DBL_MAX : order.stockRangeUpper;
	ENCODE_FIELD_MAX( order.stockRangeLower);
	ENCODE_FIELD_MAX( order.stockRangeUpper);

	ENCODE_FIELD( order.overridePercentageConstraints); // srv v22 and above

	// Volatility orders (srv v26 and above)
	ENCODE_FIELD_MAX( order.volatility);
	ENCODE_FIELD_MAX( order.volatilityType);
	// will never happen
	//if( m_serverVersion < 28) {
	//	send( order.deltaNeutralOrderType.CompareNoCase("MKT") == 0);
	//}
	//else {
		ENCODE_FIELD( order.deltaNeutralOrderType); // srv v28 and above
		ENCODE_FIELD_MAX( order.deltaNeutralAuxPrice); // srv v28 and above

		if (m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL_CONID && !order.deltaNeutralOrderType.empty()){
			ENCODE_FIELD( order.deltaNeutralConId);
			ENCODE_FIELD( order.deltaNeutralSettlingFirm);
			ENCODE_FIELD( order.deltaNeutralClearingAccount);
			ENCODE_FIELD( order.deltaNeutralClearingIntent);
		}

		if (m_serverVersion >= MIN_SERVER_VER_DELTA_NEUTRAL_OPEN_CLOSE && !order.deltaNeutralOrderType.empty()){
			ENCODE_FIELD( order.deltaNeutralOpenClose);
			ENCODE_FIELD( order.deltaNeutralShortSale);
			ENCODE_FIELD( order.deltaNeutralShortSaleSlot);
			ENCODE_FIELD( order.deltaNeutralDesignatedLocation);
		}

	//}
	ENCODE_FIELD( order.continuousUpdate);
	//if( m_serverVersion == 26) {
	//	// Volatility orders had specific watermark price attribs in server version 26
	//	double lower = (isVolOrder ? order.stockRangeLower : DBL_MAX);
	//	double upper = (isVolOrder ? order.stockRangeUpper : DBL_MAX);
	//	ENCODE_FIELD_MAX( lower);
	//	ENCODE_FIELD_MAX( upper);
	//}
	ENCODE_FIELD_MAX( order.referencePriceType);

	ENCODE_FIELD_MAX( order.trailStopPrice); // srv v30 and above

	if( m_serverVersion >= MIN_SERVER_VER_TRAILING_PERCENT) {
		ENCODE_FIELD_MAX( order.trailingPercent);
	}

	// SCALE orders
	if( m_serverVersion >= MIN_SERVER_VER_SCALE_ORDERS2) {
		ENCODE_FIELD_MAX( order.scaleInitLevelSize);
		ENCODE_FIELD_MAX( order.scaleSubsLevelSize);
	}
	else {
		// srv v35 and above)
		ENCODE_FIELD( ""); // for not supported scaleNumComponents
		ENCODE_FIELD_MAX( order.scaleInitLevelSize); // for scaleComponentSize
	}

	ENCODE_FIELD_MAX( order.scalePriceIncrement);

	if( m_serverVersion >= MIN_SERVER_VER_SCALE_ORDERS3 
		&& order.scalePriceIncrement > 0.0 && order.scalePriceIncrement != UNSET_DOUBLE) {
		ENCODE_FIELD_MAX( order.scalePriceAdjustValue);
		ENCODE_FIELD_MAX( order.scalePriceAdjustInterval);
		ENCODE_FIELD_MAX( order.scaleProfitOffset);
		ENCODE_FIELD( order.scaleAutoReset);
		ENCODE_FIELD_MAX( order.scaleInitPosition);
		ENCODE_FIELD_MAX( order.scaleInitFillQty);
		ENCODE_FIELD( order.scaleRandomPercent);
	}

	if( m_serverVersion >= MIN_SERVER_VER_SCALE_TABLE) {
		ENCODE_FIELD( order.scaleTable);
		ENCODE_FIELD( order.activeStartTime);
		ENCODE_FIELD( order.activeStopTime);
	}

	// HEDGE orders
	if( m_serverVersion >= MIN_SERVER_VER_HEDGE_ORDERS) {
		ENCODE_FIELD( order.hedgeType);
		if ( !order.hedgeType.empty()) {
			ENCODE_FIELD( order.hedgeParam);
		}
	}

	if( m_serverVersion >= MIN_SERVER_VER_OPT_OUT_SMART_ROUTING){
		ENCODE_FIELD( order.optOutSmartRouting);
	}

	if( m_serverVersion >= MIN_SERVER_VER_PTA_ORDERS) {
		ENCODE_FIELD( order.clearingAccount);
		ENCODE_FIELD( order.clearingIntent);
	}

	if( m_serverVersion >= MIN_SERVER_VER_NOT_HELD){
		ENCODE_FIELD( order.notHeld);
	}

	if( m_serverVersion >= MIN_SERVER_VER_UNDER_COMP) {
		if( contract.underComp) {
			const UnderComp& underComp = *contract.underComp;
			ENCODE_FIELD( true);
			ENCODE_FIELD( underComp.conId);
			ENCODE_FIELD( underComp.delta);
			ENCODE_FIELD( underComp.price);
		}
		else {
			ENCODE_FIELD( false);
		}
	}

	if( m_serverVersion >= MIN_SERVER_VER_ALGO_ORDERS) {
		ENCODE_FIELD( order.algoStrategy);

		if( !order.algoStrategy.empty()) {
			const TagValueList* const algoParams = order.algoParams.get();
			const int algoParamsCount = algoParams ? algoParams->size() : 0;
			ENCODE_FIELD( algoParamsCount);
			if( algoParamsCount > 0) {
				for( int i = 0; i < algoParamsCount; ++i) {
					const TagValue* tagValue = ((*algoParams)[i]).get();
					ENCODE_FIELD( tagValue->tag);
					ENCODE_FIELD( tagValue->value);
				}
			}
		}

	}

	if( m_serverVersion >= MIN_SERVER_VER_ALGO_ID) {
		ENCODE_FIELD( order.algoId);
	}

	ENCODE_FIELD( order.whatIf); // srv v36 and above

	// send miscOptions parameter
	if( m_serverVersion >= MIN_SERVER_VER_LINKING) {
		std::string miscOptionsStr("");
		const TagValueList* const orderMiscOptions = order.orderMiscOptions.get();
		const int orderMiscOptionsCount = orderMiscOptions ? orderMiscOptions->size() : 0;
		if( orderMiscOptionsCount > 0) {
			for( int i = 0; i < orderMiscOptionsCount; ++i) {
				const TagValue* tagValue = ((*orderMiscOptions)[i]).get();
				miscOptionsStr += tagValue->tag;
				miscOptionsStr += "=";
				miscOptionsStr += tagValue->value;
				miscOptionsStr += ";";
			}
		}
		ENCODE_FIELD( miscOptionsStr);
	}

	if (m_serverVersion >= MIN_SERVER_VER_ORDER_SOLICITED) {
		ENCODE_FIELD(order.solicited);
	}

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelOrder( OrderId id)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( id, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	const int VERSION = 1;

	// send cancel order msg
	std::ostringstream msg;
	prepareBuffer( msg);

	ENCODE_FIELD( CANCEL_ORDER);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( id);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqAccountUpdates(bool subscribe, const std::string& acctCode)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	// send req acct msg
	ENCODE_FIELD( REQ_ACCT_DATA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( subscribe);  // TRUE = subscribe, FALSE = unsubscribe.

	// Send the account code. This will only be used for FA clients
	ENCODE_FIELD( acctCode); // srv v9 and above

	closeAndSend( msg.str());
}

void EClientSocketBase::reqOpenOrders()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req open orders msg
	ENCODE_FIELD( REQ_OPEN_ORDERS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqAutoOpenOrders(bool bAutoBind)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req open orders msg
	ENCODE_FIELD( REQ_AUTO_OPEN_ORDERS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( bAutoBind);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqAllOpenOrders()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req open orders msg
	ENCODE_FIELD( REQ_ALL_OPEN_ORDERS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqExecutions(int reqId, const ExecutionFilter& filter)
{
	//NOTE: Time format must be 'yyyymmdd-hh:mm:ss' E.g. '20030702-14:55'

	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 3;

	// send req open orders msg
	ENCODE_FIELD( REQ_EXECUTIONS);
	ENCODE_FIELD( VERSION);

	if( m_serverVersion >= MIN_SERVER_VER_EXECUTION_DATA_CHAIN) {
		ENCODE_FIELD( reqId);
	}

	// Send the execution rpt filter data (srv v9 and above)
	ENCODE_FIELD( filter.m_clientId);
	ENCODE_FIELD( filter.m_acctCode);
	ENCODE_FIELD( filter.m_time);
	ENCODE_FIELD( filter.m_symbol);
	ENCODE_FIELD( filter.m_secType);
	ENCODE_FIELD( filter.m_exchange);
	ENCODE_FIELD( filter.m_side);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqIds( int numIds)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( numIds, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req open orders msg
	ENCODE_FIELD( REQ_IDS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( numIds);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqNewsBulletins(bool allMsgs)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req news bulletins msg
	ENCODE_FIELD( REQ_NEWS_BULLETINS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( allMsgs);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelNewsBulletins()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req news bulletins msg
	ENCODE_FIELD( CANCEL_NEWS_BULLETINS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::setServerLogLevel(int logLevel)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send the set server logging level message
	ENCODE_FIELD( SET_SERVER_LOGLEVEL);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( logLevel);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqManagedAccts()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send req FA managed accounts msg
	ENCODE_FIELD( REQ_MANAGED_ACCTS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}


void EClientSocketBase::requestFA(faDataType pFaDataType)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 13) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REQ_FA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( (int)pFaDataType);

	closeAndSend( msg.str());
}

void EClientSocketBase::replaceFA(faDataType pFaDataType, const std::string& cxml)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 13) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REPLACE_FA);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( (int)pFaDataType);
	ENCODE_FIELD( cxml);

	closeAndSend( msg.str());
}



void EClientSocketBase::exerciseOptions( TickerId tickerId, const Contract& contract,
                                     int exerciseAction, int exerciseQuantity,
                                     const std::string& account, int override)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	// Not needed anymore validation
	//if( m_serverVersion < 21) {
	//	m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
	//	return;
	//}

	if (m_serverVersion < MIN_SERVER_VER_TRADING_CLASS) {
		if( !contract.tradingClass.empty() || (contract.conId > 0)) {
			m_pEWrapper->error( tickerId, UPDATE_TWS.code(), UPDATE_TWS.msg() +
				"  It does not support conId, multiplier and tradingClass parameters in exerciseOptions.");
			return;
		}
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	ENCODE_FIELD( EXERCISE_OPTIONS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( tickerId);

	// send contract fields
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.conId);
	}
	ENCODE_FIELD( contract.symbol);
	ENCODE_FIELD( contract.secType);
	ENCODE_FIELD( contract.expiry);
	ENCODE_FIELD( contract.strike);
	ENCODE_FIELD( contract.right);
	ENCODE_FIELD( contract.multiplier);
	ENCODE_FIELD( contract.exchange);
	ENCODE_FIELD( contract.currency);
	ENCODE_FIELD( contract.localSymbol);
	if( m_serverVersion >= MIN_SERVER_VER_TRADING_CLASS) {
		ENCODE_FIELD( contract.tradingClass);
	}
	ENCODE_FIELD( exerciseAction);
	ENCODE_FIELD( exerciseQuantity);
	ENCODE_FIELD( account);
	ENCODE_FIELD( override);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqGlobalCancel()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if (m_serverVersion < MIN_SERVER_VER_REQ_GLOBAL_CANCEL) {
		m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support globalCancel requests.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	// send current time req
	ENCODE_FIELD( REQ_GLOBAL_CANCEL);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqMarketDataType( int marketDataType)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_REQ_MARKET_DATA_TYPE) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support market data type requests.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REQ_MARKET_DATA_TYPE);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( marketDataType);

	closeAndSend( msg.str());
}

int EClientSocketBase::sendBufferedData()
{
	if( m_outBuffer.empty())
		return 0;

	int nResult = send( &m_outBuffer[0], m_outBuffer.size());
	if( nResult <= 0) {
		return nResult;
	}
	CleanupBuffer( m_outBuffer, nResult);
	return nResult;
}

void EClientSocketBase::prepareBufferImpl(std::ostream& buf) const
{
	assert( m_useV100Plus);
	assert( sizeof(unsigned) == HEADER_LEN);

	char header[HEADER_LEN] = { 0 };
	buf.write( header, sizeof(header));
}

void EClientSocketBase::prepareBuffer(std::ostream& buf) const
{
	if( !m_useV100Plus)
		return;

	prepareBufferImpl( buf);
}

void EClientSocketBase::encodeMsgLen(std::string& msg, unsigned offset) const
{
	assert( !msg.empty());
	assert( m_useV100Plus);

	assert( sizeof(unsigned) == HEADER_LEN);
	assert( msg.size() > offset + HEADER_LEN);
	unsigned len = msg.size() - HEADER_LEN - offset;
	if( len > MAX_MSG_LEN) {
		m_pEWrapper->error( NO_VALID_ID, BAD_LENGTH.code(), BAD_LENGTH.msg());
		return;
	}

	unsigned netlen = htonl( len);
	memcpy( &msg[offset], &netlen, HEADER_LEN);
}

void EClientSocketBase::closeAndSend(std::string msg, unsigned offset)
{
	assert( !msg.empty());
	if( m_useV100Plus) {
		encodeMsgLen( msg, offset);
	}
	bufferedSend( msg);
}

int EClientSocketBase::bufferedSend(const char* buf, size_t sz)
{
	if( sz <= 0)
		return 0;

	if( !m_outBuffer.empty()) {
		m_outBuffer.insert( m_outBuffer.end(), buf, buf + sz);
		return sendBufferedData();
	}

	int nResult = send(buf, sz);

	if( nResult < (int)sz) {
		int sent = (std::max)( nResult, 0);
		m_outBuffer.insert( m_outBuffer.end(), buf + sent, buf + sz);
	}

	return nResult;
}

int EClientSocketBase::bufferedSend(const std::string& msg)
{
	return bufferedSend( msg.data(), msg.size());
}

int EClientSocketBase::bufferedRead()
{
	char buf[8192];
	int nResult = receive( buf, sizeof(buf));

	if( nResult > 0) {
		m_inBuffer.insert( m_inBuffer.end(), &buf[0], &buf[0] + nResult);
	}

	return nResult;
}

void EClientSocketBase::reqPositions()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_POSITIONS) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support positions request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REQ_POSITIONS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelPositions()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_POSITIONS) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support positions cancellation.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_POSITIONS);
	ENCODE_FIELD( VERSION);

	closeAndSend( msg.str());
}

void EClientSocketBase::reqAccountSummary( int reqId, const std::string& groupName, const std::string& tags)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_ACCOUNT_SUMMARY) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support account summary request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( REQ_ACCOUNT_SUMMARY);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);
	ENCODE_FIELD( groupName);
	ENCODE_FIELD( tags);

	closeAndSend( msg.str());
}

void EClientSocketBase::cancelAccountSummary( int reqId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_ACCOUNT_SUMMARY) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support account summary cancellation.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( CANCEL_ACCOUNT_SUMMARY);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

void EClientSocketBase::verifyRequest(const std::string& apiName, const std::string& apiVersion)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support verification request.");
		return;
	}

	if( !m_extraAuth) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  Intent to authenticate needs to be expressed during initial connect request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( VERIFY_REQUEST);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( apiName);
	ENCODE_FIELD( apiVersion);

	closeAndSend( msg.str());
}

void EClientSocketBase::verifyMessage(const std::string& apiData)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support verification message sending.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( VERIFY_MESSAGE);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( apiData);

	closeAndSend( msg.str());
}

void EClientSocketBase::verifyAndAuthRequest(const std::string& apiName, const std::string& apiVersion, const std::string& opaqueIsvKey)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING_AUTH) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support verification request.");
		return;
	}

	if( !m_extraAuth) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  Intent to authenticate needs to be expressed during initial connect request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( VERIFY_AND_AUTH_REQUEST);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( apiName);
	ENCODE_FIELD( apiVersion);
	ENCODE_FIELD( opaqueIsvKey);

	closeAndSend( msg.str());
}

void EClientSocketBase::verifyAndAuthMessage(const std::string& apiData, const std::string& xyzResponse)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING_AUTH) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support verification message sending.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( VERIFY_AND_AUTH_MESSAGE);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( apiData);
	ENCODE_FIELD( xyzResponse);

	closeAndSend( msg.str());
}

void EClientSocketBase::queryDisplayGroups( int reqId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support queryDisplayGroups request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( QUERY_DISPLAY_GROUPS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

void EClientSocketBase::subscribeToGroupEvents( int reqId, int groupId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support subscribeToGroupEvents request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( SUBSCRIBE_TO_GROUP_EVENTS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);
	ENCODE_FIELD( groupId);

	closeAndSend( msg.str());
}

void EClientSocketBase::updateDisplayGroup( int reqId, const std::string& contractInfo)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support updateDisplayGroup request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( UPDATE_DISPLAY_GROUP);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);
	ENCODE_FIELD( contractInfo);

	closeAndSend( msg.str());
}

void EClientSocketBase::startApi()
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 2;

	ENCODE_FIELD( START_API);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( m_clientId);

	if (m_serverVersion >= MIN_SERVER_VER_OPTIONAL_CAPABILITIES)
		ENCODE_FIELD(m_optionalCapabilities);

	closeAndSend( msg.str());
}

void EClientSocketBase::unsubscribeFromGroupEvents( int reqId)
{
	// not connected?
	if( !isConnected()) {
		m_pEWrapper->error( NO_VALID_ID, NOT_CONNECTED.code(), NOT_CONNECTED.msg());
		return;
	}

	if( m_serverVersion < MIN_SERVER_VER_LINKING) {
		m_pEWrapper->error(NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg() +
			"  It does not support unsubscribeFromGroupEvents request.");
		return;
	}

	std::ostringstream msg;
	prepareBuffer( msg);

	const int VERSION = 1;

	ENCODE_FIELD( UNSUBSCRIBE_FROM_GROUP_EVENTS);
	ENCODE_FIELD( VERSION);
	ENCODE_FIELD( reqId);

	closeAndSend( msg.str());
}

bool EClientSocketBase::checkMessages()
{
	if( !isSocketOK())
		return false;

	if( bufferedRead() <= 0) {;
		return false;
	}

	const char*	beginPtr = &m_inBuffer[0];
	const char*	ptr = beginPtr;
	const char*	endPtr = ptr + m_inBuffer.size();

	try {
		if (isConnecting() && processConnectAck( ptr, endPtr) > 0) {
			if( (ptr - beginPtr) >= (int)m_inBuffer.size())
				return false;
		}
	}
	catch (...) {
		CleanupBuffer( m_inBuffer, (ptr - beginPtr));
		throw;
	}

	CleanupBuffer( m_inBuffer, (ptr - beginPtr));
	return true;
}

int EClientSocketBase::processConnectAckImpl(const char*& beginPtr, const char* endPtr)
{
	// process a connect Ack message from the buffer;
	// return number of bytes consumed
	assert( beginPtr && beginPtr < endPtr);

	try {

		const char* ptr = beginPtr;

		// check server version
		DECODE_FIELD( m_serverVersion);
		if( m_useV100Plus) {

			// handle redirects
			if( m_serverVersion < 0) {

				std::string hostport;
				DECODE_FIELD( hostport);

				std::string::size_type sep = hostport.find( ':');
				if( sep != std::string::npos) {
					m_host = hostport.substr(0, sep);
					m_port = atoi( hostport.c_str() + ++sep);
				}
				else {
					m_host = hostport;
				}

				m_connState = CS_REDIRECT;

				int processed = ptr - beginPtr;
				beginPtr = ptr;
				return processed;
			}

			if( m_serverVersion < MIN_CLIENT_VER || m_serverVersion > MAX_CLIENT_VER) {
				eDisconnect();
				m_pEWrapper->error( NO_VALID_ID, UNSUPPORTED_VERSION.code(), UNSUPPORTED_VERSION.msg());
				return -1;
			}
		}
		if( m_serverVersion >= 20) {
			DECODE_FIELD( m_TwsTime);
		}

		if( m_serverVersion < SERVER_VERSION) {
			eDisconnect();
			m_pEWrapper->error( NO_VALID_ID, UPDATE_TWS.code(), UPDATE_TWS.msg());
			return -1;
		}

		m_connState = CS_CONNECTED;

		// send the clientId
		if( m_serverVersion >= 3) {
			if( m_serverVersion < MIN_SERVER_VER_LINKING) {
				std::ostringstream msg;
				ENCODE_FIELD( m_clientId);
				bufferedSend( msg.str());
			}
			else if (!m_extraAuth) {
				startApi();
			}
		}

		// That would be the place to notify client
		// that we are fully connected
		// e.g: m_pEWrapper->connected();

		int processed = ptr - beginPtr;
		beginPtr = ptr;
		return processed;
	}
	catch(  std::exception e) {
		m_pEWrapper->error( NO_VALID_ID, SOCKET_EXCEPTION.code(),
			SOCKET_EXCEPTION.msg() + errMsg( e) );
	}
	return 0;
}

int EClientSocketBase::processMsgImpl(const char*& beginPtr, const char* endPtr)
{
    EDecoder decoder(this, m_pEWrapper);

    return decoder.parseAndProcessMsg(beginPtr, endPtr);
}

int EClientSocketBase::processOnePrefixedMsg(const char*& beginPtr, const char* endPtr, messageHandler handler)
{
	if( beginPtr + HEADER_LEN >= endPtr)
		return 0;

	assert( sizeof(unsigned) == HEADER_LEN);

	unsigned netLen = 0;
	memcpy( &netLen, beginPtr, HEADER_LEN);

	const unsigned msgLen = ntohl(netLen);

	// shold never happen, but still....
	if( !msgLen) {
		beginPtr += HEADER_LEN;
		return HEADER_LEN;
	}

	// enforce max msg len limit
	if( msgLen > MAX_MSG_LEN) {
		m_pEWrapper->error( NO_VALID_ID, BAD_LENGTH.code(), BAD_LENGTH.msg());
		eDisconnect();
		m_pEWrapper->connectionClosed();
		return 0;
	}

	const char* msgStart = beginPtr + HEADER_LEN;
	const char* msgEnd = msgStart + msgLen;

	// handle incomplete messages
	if( msgStart > endPtr) {
		return 0;
	}

	int decoded = (this->*handler)( msgStart, msgEnd);
	if( decoded <= 0) {
		// this would mean something went real wrong
		// and message was incomplete from decoder POV
		m_pEWrapper->error( NO_VALID_ID, BAD_MESSAGE.code(), BAD_MESSAGE.msg());
		eDisconnect();
		m_pEWrapper->connectionClosed();
		return 0;
	}

	int consumed = msgEnd - beginPtr;
	beginPtr = msgEnd;
	return consumed;
}

int EClientSocketBase::processConnectAck(const char*& beginPtr, const char* endPtr)
{
	if( !m_useV100Plus) {
		return processConnectAckImpl( beginPtr, endPtr);
	}
	return processOnePrefixedMsg( beginPtr, endPtr, &EClientSocketBase::processConnectAckImpl);
}

int EClientSocketBase::processMsg(const char*& beginPtr, const char* endPtr)
{
	if( !m_useV100Plus) {
		return processMsgImpl( beginPtr, endPtr);
	}
	return processOnePrefixedMsg( beginPtr, endPtr, &EClientSocketBase::processMsgImpl);
}

EWrapper * EClientSocketBase::getWrapper() const
{
	return m_pEWrapper;
}

void EClientSocketBase::setClientId( int clientId)
{
	m_clientId = clientId;
}

void EClientSocketBase::setExtraAuth( bool extraAuth)
{
	m_extraAuth = extraAuth;
}

void EClientSocketBase::setHost( const std::string& host)
{
	m_host = host;
}

void EClientSocketBase::setPort( unsigned port)
{
	m_port = port;
}


///////////////////////////////////////////////////////////
// callbacks from socket
void EClientSocketBase::onConnectBase()
{
	m_connState = CS_CONNECTING;

	// send client version
	std::ostringstream msg;
	if( m_useV100Plus) {
		msg.write( API_SIGN, sizeof(API_SIGN));
		prepareBufferImpl( msg);
		if( MIN_CLIENT_VER < MAX_CLIENT_VER) {
			msg << 'v' << MIN_CLIENT_VER << ".." << MAX_CLIENT_VER;
		}
		else {
			msg << 'v' << MIN_CLIENT_VER;
		}
		if( !m_connectOptions.empty()) {
			msg << ' ' << m_connectOptions;
		}
		closeAndSend( msg.str(), sizeof(API_SIGN));
		return;
	}
	ENCODE_FIELD( CLIENT_VERSION);
	bufferedSend( msg.str());
}

bool EClientSocketBase::isInBufferEmpty() const
{
	return m_inBuffer.empty();
}

bool EClientSocketBase::isOutBufferEmpty() const
{
	return m_outBuffer.empty();
}

#endif
