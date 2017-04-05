/* Copyright (C) 2013 Interactive Brokers LLC. All rights reserved.  This code is subject to the terms
 * and conditions of the IB API Non-Commercial License or the IB API Commercial License, as applicable. */
#pragma once
#ifndef CONTRACTSAMPLE
#define CONTRACTSAMPLE

struct Contract;

class ContractSamples {
public:
	static Contract IBMBond();
	static Contract IBKRStk();
	static Contract HKStk();
	static Contract EurGbpFx();
	static Contract Index();
	static Contract CFD();
	static Contract USStockCFD();
	static Contract EuropeanStockCFD();
	static Contract CashCFD();
	static Contract EuropeanStock();
	static Contract OptionAtIse();
	static Contract USStock();
	static Contract USStockAtSmart();
	static Contract IBMUSStockAtSmart();
	static Contract USStockWithPrimaryExch();
	static Contract BondWithCusip();
	static Contract Bond();
	static Contract MutualFund();
	static Contract Commodity();
	static Contract USOptionContract();
	static Contract OptionAtBox();
	static Contract NormalOption();
	static Contract OptionWithTradingClass();
	static Contract OptionWithLoacalSymbol();
	static Contract SimpleFuture();
	static Contract FutureWithLocalSymbol();
	static Contract FutureWithMultiplier();
	static Contract WrongContract();
	static Contract FuturesOnOptions();
	static Contract ByISIN();
	static Contract ByConId();
	static Contract OptionForQuery();
	static Contract StockComboContract();
	static Contract FutureComboContract();
	static Contract OptionComboContract();
	static Contract InterCmdtyFuturesContract();
	static Contract NewsFeedForQuery();
	static Contract BTbroadtapeNewsFeed();
	static Contract BZbroadtapeNewsFeed();
	static Contract FLYbroadtapeNewsFeed();
	static Contract MTbroadtapeNewsFeed();
};

#endif
