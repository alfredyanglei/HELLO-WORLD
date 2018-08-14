/*
 * StockDateUpdate.h
 *
 *  Created on: 2017年2月6日
 *      Author: 8888
 */

#ifndef STOCKDATEUPDATE_H_
#define STOCKDATEUPDATE_H_

#include "util/tc_thread_rwlock.h"
#include "util/tc_singleton.h"
#include "util/tc_mysql.h"
#include "CountTimeApp.h"
#include "Condition.h"
#include "GlobalConfig.h"
#include "BasicHq.h"

using namespace std;

class StockDateUpdate : public TC_Singleton<StockDateUpdate>, public TC_Thread
{
public:
	StockDateUpdate();
	virtual ~StockDateUpdate();

public:
	void terminate();
	string getDayLastDate();
	string getCalcLastDate();

	//根据uni获取股票stockInfo
	bool getStockByUni(const string &sUni, HQExtend::SelStockInfo &stockInfo);

	//根据market+code来获取股票stockInfo
	bool getStockByMarketCode(const string &sMarketCode, HQExtend::SelStockInfo &stockInfo);

	//获取所有股票的条件行情数据
	vector<HQExtend::SelStockCondHq> getStockCondHq();
	//获取某一只股票的条件行情数据
	bool getStockCondHqByMarketCode(const string &sMarketCode, HQExtend::SelStockCondHq &stStockCondHq);

protected:
	//线程任务
	virtual void run();

protected:
	//查询stk_basic_info缓存股票基本信息
	void updateUni2Stock();
	void getStockBySql(const string &sSql);

	void dataUpdate();
	void updateDayEndDate();
	void getLastDateBySql(const string &sSql, const string &sValue, string &sLastDate);
	void convertStockCondHq(const HQSys::HStockHq &stStock, vector<HQExtend::SelStockCondHq> &vStockCondHq, map<string, HQExtend::SelStockCondHq> &mStockCondHq);
	bool getStockBaseInfo(const HQSys::HStockHq &stStock, double dNowPrice, HQExtend::SelStockCondHq &stStk);

	void updateFreshTable();
	void updateStockCondHq();

protected:
    taf::TC_ThreadLock _dataLock;
    TC_ThreadRWLocker _lock;

	taf::TC_Mysql _sql;
	vector<string> _vEndDate; //quant_fct_value_row_d日表查询的交易时期
	string _sDayLastDate;  //quant_fct_value_row_d日表查询的最新交易时期
	string _sCalcLastDate; //quant_stk_calc_d指标表查询的最新交易时期
	int _iFreshTime;       //quant_fct_value_row_q_fresh更新数据时间
	int _InfoTime;         //stk_basic_info更新数据时间
	int _HqTime;           //更新最新行情数据时间
	string _sFreshRowList; //fresh表更新的所有列

	//缓存stk_basic_info里的股票(只缓存深沪A股正常上市的股票)
	map<string, HQExtend::SelStockInfo> _mUni2StockCache; 
	map<string, HQExtend::SelStockInfo> _mMarketCode2StockCache;

	HQSys::BasicHqPrx _basicPrx;
	//缓存的实时行情数据
	vector<HQExtend::SelStockCondHq> _vStockCondHq;
	map<string, HQExtend::SelStockCondHq> _mStockCondHq;
};


#endif /* STOCKDATEUPDATE_H_ */
