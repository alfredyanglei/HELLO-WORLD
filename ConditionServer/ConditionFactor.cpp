/*
 * ConditonFactor.cpp
 *
 *  Created on: 2017年1月20日
 *      Author: 8888
 */

#include "ConditionFactor.h"
#include "StreamUtil.h"
#include "GlobalConfig.h"
#include "util/tc_common.h"
#include "ConMarketCode.h"

using namespace HQSys;
using namespace taf;
using namespace HQExtend;


ConditionFactor::ConditionFactor()
{
    // TODO Auto-generated constructor stub
	_basicPrx = Application::getCommunicator()->stringToProxy<BasicHqPrx>(GlobalConfig::getInstance()->getBasicHqObj());
}

ConditionFactor::~ConditionFactor()
{
    // TODO Auto-generated destructor stub
}

CondFctMarket::CondFctMarket()
{   

}

int CondFctMarket::selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock)
{
	try
	{
		int iType;
		if (false == getStockType(stItem.iValue, iType))
		{
			TLOG_ERROR("ConditionItem value is not define.value:" << stItem.iValue << endl);
			return -1;
		}

		vector<HQExtend::SelStockCondHq> vStockCondHq = StockDateUpdate::getInstance()->getStockCondHq();

		//分配容器大小，提高效率
		if (!vStockCondHq.empty())
		{
			unsigned int iRealSize = vStockCondHq.size();
			vStock.reserve(iRealSize);
		}

		for (size_t i = 0; i < vStockCondHq.size(); i++)
		{
			if (iType == vStockCondHq[i].iType)
			{
				//停牌股票有现价和昨收，没有成交额、成交量！
				//if (vStockCondHq[i].stCondHq.dAmount < 0.001 || vStockCondHq[i].stCondHq.dVolume < 0.001)
				//{
				//	continue;
				//}

				HQExtend::SelectedCondStock stStock;
				stStock.stStockPrice = vStockCondHq[i].stStockPrice;
				stStock.bTransactionStatus = vStockCondHq[i].stCondHqAddition.bTransactionStatus;
				vStock.push_back(stStock);
			}
		}

		if (!vStock.empty())
		{
			return 0;
		}

	}
	catch (exception &e)
	{
		TLOG_ERROR("exception:" << e.what() << endl);
	}
	catch (...)
	{
		TLOG_ERROR("unknown exception" << endl);
	}
	return -1;
}

bool CondFctMarket::getStockType(int iValue, int &iType)
{
	bool bRet = true;
	switch(iValue)
	{
	case HQExtend::MARKET_SHA:
		iType = (int)STKC_SH_AG;
		break;
	case HQExtend::MARKET_SZA:
		iType = (int)STKC_SZ_AG;
		break;
	case HQExtend::MARKET_CY:
		iType = (int)STKC_SZ_CY;
		break;
	case HQExtend::MARKET_SM:
		iType = (int)STKC_SZ_SM;
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
}

CondFctB2S::CondFctB2S()
{

}

int CondFctB2S::selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock)
{
	try
	{
		if (stItem.sCode.empty())
		{
			TLOG_ERROR("Block code is null." << endl);
			return -1;
		}

		TELL_TIME_COST_THIS;

		HBlock2StockReq stReq;
		stReq.stHeader.shtMarket = 1;
		stReq.vBlockCode.push_back(stItem.sCode);
		stReq.eColumn = HQSys::E_HQ_COLUMN_CHG;
		stReq.eSort = HQSys::E_SORT_DESCEN;
		stReq.eHqData = (E_STOCK_HQ_DATA)0x03;
		HBlock2StockRsp stRsp;
		if (0 != _basicPrx->block2Stock(stReq, stRsp))
		{
			TLOG_ERROR("can't get block2Stock for " << stItem.sCode << endl);
			return -1;
		}

		TLOG_DEBUG("get data cost time is " << etos(stItem.eFactor) << "|" << COST_NOW_THIS << "ms" << endl);

		map<string, HQSys::HType2StockRsp>::iterator itr = stRsp.mStockList.begin();
		for (; itr != stRsp.mStockList.end(); itr++)
		{
			for (size_t i = 0; i < itr->second.vStock.size(); i++)
			{
				//过滤掉停牌的股票，停牌股票有现价和昨收，没有开盘价、成交额、成交量！
				//if (itr->second.vStock[i].stSimHq.fOpen < 0.001)
				//{
				//	continue;
				//}

				HQExtend::SelectedCondStock stStock;
				stStock.stStockPrice.stInfo.shtMarket = itr->second.vStock[i].shtSetcode;
				stStock.stStockPrice.stInfo.sCode = itr->second.vStock[i].sCode;
				stStock.stStockPrice.stInfo.sName = itr->second.vStock[i].sName;
				stStock.stStockPrice.stPrice.dPrice = itr->second.vStock[i].stSimHq.fNowPrice;
				stStock.stStockPrice.stPrice.dChange = itr->second.vStock[i].stSimHq.fChgValue;
				stStock.stStockPrice.stPrice.dChangeRate = itr->second.vStock[i].stSimHq.fChgRatio;

				stStock.bTransactionStatus = itr->second.vStock[i].stExHq.bTransactionStatus;

				vStock.push_back(stStock);
			}
		}

		if (!vStock.empty())
		{
			return 0;
		}

	}
	catch (exception &e)
	{
		TLOG_ERROR("exception:" << e.what() << endl);
	}
	catch (...)
	{
		TLOG_ERROR("unknown exception" << endl);
	}
	return -1;
}

CondFctSQL::CondFctSQL(taf::TC_Mysql &sql) :_sql(sql)
{
}

int CondFctSQL::selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock)
{
	try
	{
		//查找对应的指标值和列名称
		string sIndex, sFctValue;

		//技术面指标因子
		if (isCalcFactor(stItem.eFactor))
		{
			if (false == getCalcType(stItem, sIndex, sFctValue))
			{
				TLOG_ERROR("ConditionItem value is not define.factor:" << etos(stItem.eFactor) << endl);
				return -1;
			}
			TLOG_DEBUG("getCalcType  sIndex|sFctValue is " << sIndex << "|" << sFctValue << endl);
		}
		//日表、季度表因子
		else
		{
			if (stItem.dMinValue > stItem.dMaxValue)
			{
				TLOG_ERROR("MinValue gt MaxValue." << etos(stItem.eFactor) << "|" << stItem.dMinValue << "|" << stItem.dMaxValue << endl);
				return -1;
			}

			//找到因子在数据表中的列名称
			sFctValue = GlobalConfig::getInstance()->getFactorValue(stItem.eFactor);
			if (sFctValue.empty())
			{
				TLOG_ERROR("factor.conf is not the factor.eFactor:" << etos(stItem.eFactor) << endl);
				return -1;
			}

			TLOG_DEBUG("sFctValue is " << sFctValue << endl);
		}

		//找到因子查询的数据表
		HQExtend::E_CONDITION_TABLE eTable = getTable(stItem.eFactor);
		string sTableName = GlobalConfig::getInstance()->getTableName(eTable);
		TLOG_DEBUG("TableName is " << sTableName << endl);
		if (sTableName.empty())
		{
			TLOG_ERROR("getTableName don't get Table name!eTable: " << etos(eTable) << endl);
			return -1;
		}

		//查询的是quant_fct_value_row_d表
		if (eTable == QUANT_DAY)
		{
			//构建数据库查询语句
			string sLastDate = StockDateUpdate::getInstance()->getDayLastDate();
			if (sLastDate.empty())
			{
				TLOG_ERROR("StockDateUpdate don't get LastDate! Factor: " << etos(stItem.eFactor) << endl);
				return -1;
			}

			string sSql = "SELECT END_DATE, SEC_UNI_CODE, " + sFctValue + " FROM (SELECT END_DATE, SEC_UNI_CODE, " + sFctValue + " FROM " + sTableName + " WHERE " + sFctValue + " BETWEEN " + TC_Common::tostr<double>(stItem.dMinValue)
				+ " AND " + TC_Common::tostr<double>(stItem.dMaxValue) + " AND END_DATE='" + sLastDate + "') AS boy GROUP BY SEC_UNI_CODE ORDER BY " + sFctValue + " DESC; ";
			TLOG_DEBUG("query sql is " << sSql << endl);

			//根据因子查询数据库
			querySql(sSql, sFctValue, stItem.eFactor, vStock);
		}
		//查询的是quant_fct_value_row_q_fresh表
		else if (eTable == QUANT_SEASON_FRESH)
		{
			//构建数据库查询语句
			string sSql = "SELECT END_DATE, SEC_UNI_CODE, " + sFctValue + " FROM  " + sTableName + " WHERE " + sFctValue + " BETWEEN "
				+ TC_Common::tostr<double>(stItem.dMinValue) + " AND " + TC_Common::tostr<double>(stItem.dMaxValue) + " ORDER BY " + sFctValue + " DESC;";
			TLOG_DEBUG("query sql is " << sSql << endl);

			//根据因子查询数据库
			querySql(sSql, sFctValue, stItem.eFactor, vStock);
		}
		//查询的是quant_stk_calc_d表
		else if (eTable == QUANT_CALC_DAY)
		{
			//构建数据库查询语句
			string sIndexLastDate = StockDateUpdate::getInstance()->getCalcLastDate();
			if (sIndexLastDate.empty())
			{
				TLOG_ERROR("StockDateUpdate don't get indexLastDate! Factor: " << etos(stItem.eFactor) << endl);
				return -1;
			}

			string sSql = "SELECT gpcode FROM " + sTableName + " WHERE gscode='" + sIndex + "' AND ymd=" + sIndexLastDate + " AND " + sFctValue + "=1;";
			TLOG_DEBUG("query sql is " << sSql << endl);

			//根据因子查询数据库
			queryCalcSql(sSql, vStock);
		}
		else
		{
			TLOG_ERROR("eTable is error.eTable:" << etos(eTable) << endl);
		}

		if (!vStock.empty())
		{
			return 0;
		}

	}
	catch (exception &e)
	{
		TLOG_ERROR("exception:" << e.what() << endl);
	}
	catch (...)
	{
		TLOG_ERROR("unknown exception" << endl);
	}
	return -1;
}

void CondFctSQL::querySql(const string &sSql, const string &sFctValue, HQExtend::E_CONDITION_FACTOR eFct, vector<HQExtend::SelectedCondStock> &vStock)
{
	TELL_TIME_COST_THIS;

	try
	{
		//查询数据,获取结果集
		TC_Mysql::MysqlData  stMysqlData = _sql.queryRecord(sSql);
		if (0 == stMysqlData.size())
		{
			//没有查询到对应的结果
			TLOG_ERROR("TC_Mysql::MysqlData  is NULL.Query Sql is " << sSql << endl);
			return;
		}

		TLOG_DEBUG("query sql result cost time is " << COST_NOW_THIS << "ms" << endl);

		//遍历结果集
		size_t num = stMysqlData.size();
		for (size_t i = 0; i < num; i++)
		{
			TC_Mysql::MysqlRecord stRecord = stMysqlData[i];
			string sEndDate = stRecord["END_DATE"];
			string sUnitCode = stRecord["SEC_UNI_CODE"];
			double dFctValue = TC_Common::strto<double>(stRecord[sFctValue]);
			double dRatio = getRatio(eFct);
			if ( dRatio > 0 )
			{
				dFctValue *= dRatio;
			}

			SelStockInfo stStockInfo;
			if (false == StockDateUpdate::getInstance()->getStockByUni(sUnitCode, stStockInfo))
			{
				continue;
			}

			HQExtend::SelectedCondStock stStock;
			stStock.stStockPrice.stInfo = stStockInfo;

			stStock.mFctList.insert(make_pair(eFct, dFctValue));

			vStock.push_back(stStock);
		}
	}
	catch (std::exception &ex)
	{
		LOG_ERROR << ex.what() << endl;
	}
	catch (...)
	{
		LOG_ERROR << "Unknown exception." << endl;
	}
}

HQExtend::E_CONDITION_TABLE CondFctSQL::getTable(HQExtend::E_CONDITION_FACTOR eFct)
{
	HQExtend::E_CONDITION_TABLE eTable = QUANT_NONE;
	switch (eFct)
	{
	case FACTOR_PB:
	case FACTOR_LTSZ:
	case FACTOR_PS:
	case FACTOR_PCF:
	case FACTOR_LHB_JGJMR:
	case FACTOR_LHB_JGJMC:
	case FACTOR_LHB_YYBJMR:
	case FACTOR_LHB_YYBJMC:
	case FACTOR_LHB_MJGMR:
	case FACTOR_LHB_SBN:
	case FACTOR_LHB_MJGMC:
		eTable = QUANT_DAY;
		break;
	case FACTOR_MACD:
	case FACTOR_KDJ:
	case FACTOR_BOLL:
	case FACTOR_RSI:
	case FACTOR_KXT:
		eTable = QUANT_CALC_DAY;
		break;
	default:
		eTable = QUANT_SEASON_FRESH;
		break;
	}

	return eTable;
}

bool CondFctSQL::isCalcFactor(HQExtend::E_CONDITION_FACTOR eFct)
{
	bool bRet = false;
	switch (eFct)
	{
	case FACTOR_MACD:
	case FACTOR_KDJ:
	case FACTOR_BOLL:
	case FACTOR_RSI:
	case FACTOR_KXT:
		bRet = true;
		break;
	default:
		break;
	}

	return bRet;
}

bool CondFctSQL::getCalcType(const HQExtend::ConditionItem &stItem, string & sIndex, string &sFctValue)
{
	bool bRet = true;
	sIndex = GlobalConfig::getInstance()->getFactorValue(stItem.eFactor);
	switch (stItem.eFactor)
	{
	case HQExtend::FACTOR_MACD:
		sFctValue = GlobalConfig::getInstance()->getMacdValue(stItem.iValue);
		break;
	case HQExtend::FACTOR_KDJ:
		sFctValue = GlobalConfig::getInstance()->getKdjValue(stItem.iValue);
		break;
	case HQExtend::FACTOR_BOLL:
		sFctValue = GlobalConfig::getInstance()->getBollValue(stItem.iValue);
		break;
	case HQExtend::FACTOR_RSI:
		sFctValue = GlobalConfig::getInstance()->getRsiValue(stItem.iValue);
		break;
	case HQExtend::FACTOR_KXT:
		sFctValue = GlobalConfig::getInstance()->getKxtValue(stItem.iValue);
		break;

	default:
		bRet = false;
		break;
	}

	if (sIndex.empty() || sFctValue.empty())
	{
		bRet = false;
	}

	return bRet;
}

void CondFctSQL::queryCalcSql(const string &sSql, vector<HQExtend::SelectedCondStock> &vStock)
{
	TELL_TIME_COST_THIS;

	try
	{
		//查询数据,获取结果集
		TC_Mysql::MysqlData  stMysqlData = _sql.queryRecord(sSql);
		if (0 == stMysqlData.size())
		{
			//没有查询到对应的结果
			TLOG_ERROR("TC_Mysql::MysqlData  is NULL. Query Sql is " << sSql << endl);
			return;
		}

		TLOG_DEBUG("query Calc sql result cost time is " << COST_NOW_THIS << "ms" << endl);

		//遍历结果集
		size_t num = stMysqlData.size();
		for (size_t i = 0; i < num; i++)
		{
			TC_Mysql::MysqlRecord stRecord = stMysqlData[i];
			string sStockInfo = stRecord["gpcode"];
			//获取股票SelStockInfo
			string sMarketCode = getMarketCodeByStock(sStockInfo);
			if (sMarketCode.empty())
			{
				TLOG_ERROR("getMarketCodeByStock is not find. stock is " << sStockInfo << endl);
				continue;
			}

			SelStockInfo stStockInfo;
			if (false == StockDateUpdate::getInstance()->getStockByMarketCode(sMarketCode, stStockInfo))
			{
				TLOG_ERROR("getStockByMarketCode sMarketCode is not find. sMarketCode is" << sMarketCode << endl);
				continue;
			}

			HQExtend::SelectedCondStock stStock;
			stStock.stStockPrice.stInfo = stStockInfo;

			vStock.push_back(stStock);
		}
	}
	catch (std::exception &ex)
	{
		LOG_ERROR << ex.what() << endl;
	}
	catch (...)
	{
		LOG_ERROR << "Unknown exception." << endl;
	}
}

string CondFctSQL::getMarketCodeByStock(const string &sStockInfo)
{
	//技术面表的股票都是8位，例如：SH000001
	if (sStockInfo.size() != 8)
	{
		return "";
	}

	string sMarket = sStockInfo.substr(0, 2);
	string sCode = sStockInfo.substr(2);
	int iMarket;
	if (sMarket == "SZ")
	{
		iMarket = 0;
	}
	else if (sMarket == "SH")
	{
		iMarket = 1;
	}
	else
	{
		return "";
	}

	return (TC_Common::tostr<int>(iMarket) + sCode);
}

double CondFctSQL::getRatio(HQExtend::E_CONDITION_FACTOR eFct)
{
	if ( eFct >= HQExtend::FACTOR_LHB_JGJMR && eFct <= HQExtend::FACTOR_LHB_YYBJMC )
	{
		return 10000;
	}
	else
	{
		return 0;
	}
}

CondFctHqTrend::CondFctHqTrend()
{

}

int CondFctHqTrend::selectCondStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock)
{
	try
	{
		if (stItem.dMinValue > stItem.dMaxValue)
		{
			TLOG_ERROR("MinValue gt MaxValue." << etos(stItem.eFactor) << "|" << stItem.dMinValue << "|" << stItem.dMaxValue << endl);
			return -1;
		}

		if (false == isHqTrendType(stItem.eFactor))
		{
			TLOG_ERROR("Factor is not in HqTrendType. Factor is " << etos(stItem.eFactor) << endl);
			return -1;
		}

		vector<HQExtend::SelStockCondHq> vStockCondHq = StockDateUpdate::getInstance()->getStockCondHq();
		if (!vStockCondHq.empty())
		{
			unsigned int iRealSize = vStockCondHq.size();
			vStock.reserve(iRealSize);
		}

		for (size_t i = 0; i < vStockCondHq.size(); i++)
		{
			HQExtend::SelStockCondHq &stStock = vStockCondHq[i];
			//停牌股票有现价和昨收，没有成交额、成交量！
			//if (stStock.stCondHq.dAmount < 0.001 || stStock.stCondHq.dVolume < 0.001)
			//{
			//	continue;
			//}

			double dFctValue = 0.0;
			switch (stItem.eFactor)
			{
			case FACTOR_PE:
				dFctValue = stStock.stCondHq.dPeRatio;
				break;
			case FACTOR_ZSZ:
				dFctValue = stStock.stCondHq.dTotalMarketValue;
				break;
			case FACTOR_CHG:
				dFctValue = stStock.stCondHq.dChgRatio;
				break;
			case FACTOR_HSL:
				dFctValue = stStock.stCondHq.dTurnoverRate;
				break;
			case FACTOR_ZHENFU:
				dFctValue = stStock.stCondHq.dZhenfu;
				break;
			case FACTOR_AMOUNT:
				dFctValue = stStock.stCondHq.dAmount;
				break;
			case FACTOR_VOLUME:
				dFctValue = stStock.stCondHq.dVolume;
				break;
			case FACTOR_PRICE:
				dFctValue = stStock.stCondHq.dNowPrice;
				break;
			case FACTOR_LIANGBI:
				dFctValue = stStock.stCondHq.dLiangBi;
				break;
			case FACTOR_WEIBI:
				dFctValue = stStock.stCondHq.dWeiBi;
				break;

			default:
				break;
			}

			if (dFctValue >= stItem.dMinValue && dFctValue <= stItem.dMaxValue)
			{
				convertStockWithPrice(stStock, stItem.eFactor, dFctValue, vStock);
			}
		}

		if (!vStock.empty())
		{
			return 0;
		}

	}
	catch (exception &e)
	{
		TLOG_ERROR("exception:" << e.what() << endl);
	}
	catch (...)
	{
		TLOG_ERROR("unknown exception" << endl);
	}
	return -1;
}

bool CondFctHqTrend::isHqTrendType(HQExtend::E_CONDITION_FACTOR eFct)
{
	bool bRet = false;
	switch (eFct)
	{
	case HQExtend::FACTOR_PE:
	case HQExtend::FACTOR_ZSZ:
	case HQExtend::FACTOR_CHG:
	case HQExtend::FACTOR_HSL:
	case HQExtend::FACTOR_ZHENFU:
	case HQExtend::FACTOR_AMOUNT:
	case HQExtend::FACTOR_VOLUME:
	case HQExtend::FACTOR_PRICE:
	case HQExtend::FACTOR_LIANGBI:
	case HQExtend::FACTOR_WEIBI:
		bRet = true;
		break;
	default:
		break;
	}

	return bRet;
}

void CondFctHqTrend::convertStockWithPrice(const HQExtend::SelStockCondHq &stStock, HQExtend::E_CONDITION_FACTOR eFct, double dFctValue, vector<HQExtend::SelectedCondStock> &vStock)
{
	HQExtend::SelectedCondStock stStk;
	stStk.stStockPrice = stStock.stStockPrice;
	stStk.mFctList.insert(make_pair(eFct, dFctValue));

	stStk.bTransactionStatus = stStock.stCondHqAddition.bTransactionStatus;

	vStock.push_back(stStk);
}