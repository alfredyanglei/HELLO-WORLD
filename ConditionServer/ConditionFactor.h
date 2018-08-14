/*
 * ConditionFactor.h
 *
 *  Created on: 2017年1月20日
 *      Author: 8888
 */

#ifndef CONDITIONFACTOR_H_
#define CONDITIONFACTOR_H_
#include "servant/Application.h"
#include "util/tc_autoptr.h"
#include "util/tc_singleton.h"
#include "Condition.h"
#include "util/tc_thread_rwlock.h"
#include "util/tc_mysql.h"
#include "BasicHq.h"
#include "Common.h"
#include "CountTimeApp.h"
#include "StockDateUpdate.h"

class ConditionFactor : public TC_HandleBase
{
public:
	ConditionFactor();
	virtual ~ConditionFactor();

	virtual int selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock) = 0;

protected:
	HQSys::BasicHqPrx _basicPrx;
};


typedef TC_AutoPtr<ConditionFactor> ConditionFactorPtr;

/**
 * 市场
 * 例如：深市A股
 */
class CondFctMarket : public ConditionFactor
{
public:
	CondFctMarket();
	virtual int selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock);
protected:
	bool getStockType(int iValue, int &iType);
};

/**
* 板块：行业板块、概念板块的成分股
* 例如：建材板块：880016
*/
class CondFctB2S : public ConditionFactor
{
public:
	CondFctB2S();
	virtual int selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock);
};

/**
* quant数据库表查询，查询的相关表：quant_fct_value_row_d、quant_fct_value_row_q、quant_stk_calc_d
* 例如：市净率在【1，3】之间的股票
*/
class CondFctSQL : public ConditionFactor
{
public:
	CondFctSQL(taf::TC_Mysql &sql);
	virtual int selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock);

protected:
	//根据日表和季度表因子查询quant_fct_value_row_d、quant_fct_value_row_q对应股票
	void querySql(const string &sSql, const string &sFctValue, HQExtend::E_CONDITION_FACTOR eFct, vector<HQExtend::SelectedCondStock> &vStock);
	HQExtend::E_CONDITION_TABLE getTable(HQExtend::E_CONDITION_FACTOR eFct);

	//根据技术指标因子查询quant_stk_calc_d对应股票
	void queryCalcSql(const string &sSql, vector<HQExtend::SelectedCondStock> &vStock);
	bool isCalcFactor(HQExtend::E_CONDITION_FACTOR eFct);
	bool getCalcType(const HQExtend::ConditionItem &stItem, string & sIndex, string &sFctValue);
	string getMarketCodeByStock(const string &sStockInfo);
	double getRatio(HQExtend::E_CONDITION_FACTOR eFct);  // 龙虎榜中跟额度有关的因子都是以万元为单位

private:
	taf::TC_Mysql &_sql;
};

/**
* 行情趋势
* 例如：涨跌幅在【5，10】之间的股票
*/
class CondFctHqTrend : public ConditionFactor
{
public:
	CondFctHqTrend();
	virtual int selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock);
protected:
	bool isHqTrendType(HQExtend::E_CONDITION_FACTOR eFct);
	void convertStockWithPrice(const HQExtend::SelStockCondHq &stStock, HQExtend::E_CONDITION_FACTOR eFct, double dFctValue, vector<HQExtend::SelectedCondStock> &vStock);
};

#endif