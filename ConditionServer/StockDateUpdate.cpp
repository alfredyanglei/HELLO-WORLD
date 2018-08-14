/*
 * StockDateUpdate.cpp
 *
 *  Created on: 2017年2月6日
 *      Author: 8888
 */
#include "StockDateUpdate.h"
#include "comdatetime.h"

#define END_DATE_LEN 10
#define YMD_LEN 8

using namespace HQSys;
using namespace BackCom;

StockDateUpdate::StockDateUpdate() :_sql(GlobalConfig::getInstance()->getDBConf()), _sDayLastDate(""),_sCalcLastDate(""),_iFreshTime(GlobalConfig::getInstance()->getFreshTime())
	,_InfoTime(GlobalConfig::getInstance()->getInfoTime()),_HqTime(GlobalConfig::getInstance()->getHqTime()),_sFreshRowList(GlobalConfig::getInstance()->getRowList())
{

	_basicPrx = Application::getCommunicator()->stringToProxy<BasicHqPrx>(GlobalConfig::getInstance()->getBasicHqObj());

	try
	{
		//连接数据库
		_sql.connect();

		//初始化缓存数据
		updateUni2Stock();
		updateFreshTable();
		updateStockCondHq();
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

StockDateUpdate::~StockDateUpdate()
{
    if(_running)
    {
        terminate();
    }
}

void StockDateUpdate::terminate()
{
	_running = false;

	try
	{
		TC_ThreadLock::Lock sync(_dataLock);
		_dataLock.notifyAll();
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

string StockDateUpdate::getDayLastDate()
{ 
	TC_ThreadRLock lock(_lock);
	int iTime = TC_Common::strto<int>( TC_Common::now2str("%H%M%S") );
	string sDate = TC_Common::now2str("%Y-%m-%d");
	// 8:00PM之前使用前一个交易日
	if ( iTime < 200000 )
	{
		if ( sDate == _sDayLastDate )
		{
			for(int i = int(_vEndDate.size()) - 1; i > 0; i--)
			{
				if( _vEndDate[i] == _sDayLastDate )
				{
					return _vEndDate[i - 1];
				}
			}
		}
		else
		{
			return _sDayLastDate;
		}
	}
	// 8:00PM之后使用当前交易日
	else
	{
		return _sDayLastDate;
	}

	TLOG_DEBUG("_vEndData.size:"<<_vEndDate.size()<<"|_sDayLastDate:"<<_sDayLastDate<<endl);
	return _sDayLastDate;
}

string StockDateUpdate::getCalcLastDate()
{
	TC_ThreadRLock lock(_lock);
	return _sCalcLastDate;
}

void StockDateUpdate::updateUni2Stock()
{
	//构建数据库查询语句
	string sSql = "SELECT STK_UNI_CODE,STK_CODE,STK_SHORT_NAME,SEC_MAR_PAR,STK_TYPE_PAR,LIST_STA_PAR FROM stk_basic_info;";
	TLOG_DEBUG("query sql is " << sSql << endl);

	//查询stk_basic_info获取所有股票
	getStockBySql(sSql);
}

void StockDateUpdate::updateFreshTable()
{
	try
	{
		size_t sum = _sql.deleteRecord("quant_fct_value_row_q_fresh");
		TLOG_DEBUG("quant_fct_value_row_q_fresh delete the total number is " << sum << " sql is " << _sql.getLastSQL() << endl);

		//关联stk_basic_info表，插入深沪A股的最新季度数据到quant_fct_value_row_q_fresh，耗时大约需要10s
		//关联查询：深沪、A股、正常上市,恢复上市
		string sSql = "INSERT INTO quant_fct_value_row_q_fresh(" + _sFreshRowList + ") (SELECT " + _sFreshRowList + " FROM(SELECT b.*FROM stk_basic_info a, quant_fct_value_row_q b WHERE a.STK_UNI_CODE = b.SEC_UNI_CODE AND a.SEC_MAR_PAR IN(1, 2) AND a.STK_TYPE_PAR = 1 AND a.LIST_STA_PAR IN(1, 4) ORDER BY b.END_DATE DESC) AS c GROUP BY c.SEC_UNI_CODE ORDER BY c.END_DATE DESC); ";
		TLOG_DEBUG("insert into sql is " << sSql << endl);

		_sql.execute(sSql);
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

void StockDateUpdate::updateStockCondHq()
{
	TELL_TIME_COST_THIS;

	HType2StockSimpleReq stReq;
	stReq.stHeader.shtMarket = 1;
	stReq.eBussType = EBT_A;
	stReq.eColumn = E_HQ_COLUMN_CHG;
	stReq.eSort = E_SORT_DEFAULT;
	stReq.eHqData = (E_STOCK_HQ_DATA)(E_SHD_SIMPLE | E_SHD_ORDER | E_SHD_DERIVE);
	HType2StockRsp stRsp;
	if (0 != _basicPrx->type2StockSimple(stReq, stRsp))
	{
		TLOG_ERROR("can't get type2StockSimple EBT_A for " << stReq.eBussType << endl);
		return;
	}

	TLOG_DEBUG("get type2StockSimple cost time is " << COST_NOW_THIS << "ms" << endl);

	if (!stRsp.vStock.empty())
	{
		vector<HQExtend::SelStockCondHq> vStockCondHq;
		map<string, HQExtend::SelStockCondHq> mStockCondHq;
		unsigned int iRealSize = stRsp.vStock.size();
		vStockCondHq.reserve(iRealSize);

		for (size_t i = 0; i < stRsp.vStock.size(); i++)
		{
			HStockHq &stStock = stRsp.vStock[i];
			//停牌股票有现价和昨收，没有开盘价！
			//if (stStock.stSimHq.fOpen < 0.0001)
			//{
			//	continue;
			//}

			convertStockCondHq(stStock, vStockCondHq, mStockCondHq);
		}

		TC_ThreadWLock lock(_lock);
		_vStockCondHq.swap(vStockCondHq);
		_mStockCondHq.swap(mStockCondHq);

		TLOG_DEBUG("_vStockCondHq:_mStockCondHq size is " << _vStockCondHq.size() << "|" << _mStockCondHq.size() << endl);
	}
}

bool StockDateUpdate::getStockByUni(const string &sUni, HQExtend::SelStockInfo &stockInfo)
{
	//第一次查询缓存中是否存在这只股票（现只要深沪股）
	{
		TC_ThreadRLock lock(_lock);
		if (_mUni2StockCache.count(sUni) > 0)
		{
			stockInfo = _mUni2StockCache[sUni];
			return true;
		}
	}

	return false;
}

bool StockDateUpdate::getStockByMarketCode(const string &sMarketCode, HQExtend::SelStockInfo &stockInfo)
{
	//查询缓存中是否存在这只股票（现只要深沪股）
	TC_ThreadRLock lock(_lock);
	if (_mMarketCode2StockCache.count(sMarketCode) > 0 && _mMarketCode2StockCache[sMarketCode].shtMarket != 47)
	{
		stockInfo = _mMarketCode2StockCache[sMarketCode];
		return true;
	}

	return false;
}

vector<HQExtend::SelStockCondHq> StockDateUpdate::getStockCondHq()
{
	TC_ThreadRLock lock(_lock);
	return _vStockCondHq;
}

bool StockDateUpdate::getStockCondHqByMarketCode(const string &sMarketCode, HQExtend::SelStockCondHq &stStockCondHq)
{
	//查询缓存中是否存在这只股票（现只要深沪股）
	TC_ThreadRLock lock(_lock);
	if (_mStockCondHq.count(sMarketCode) > 0)
	{
		stStockCondHq = _mStockCondHq[sMarketCode];
		return true;
	}

	return false;
}

void StockDateUpdate::getStockBySql(const string &sSql)
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

		//遍历结果集
		size_t num = stMysqlData.size();
		for (size_t i = 0; i < num; i++)
		{
			HQExtend::SelStockInfo stockinfo;
			TC_Mysql::MysqlRecord stRecord = stMysqlData[i];
			int iMarketPar = TC_Common::strto<int>(stRecord["SEC_MAR_PAR"]); //证券市场参数：1--深市 2--沪市 176--新三板
			int iTypePar = TC_Common::strto<int>(stRecord["STK_TYPE_PAR"]);  //股票类型参数：1--A股  2--B股
			int iStaPar = TC_Common::strto<int>(stRecord["LIST_STA_PAR"]);   //上市状态参数：1--正常上市 2--暂停上市 3--终止上市 4--恢复上市

			if ((iTypePar != 1) || (iStaPar != 1 && iStaPar != 4))
			{
				continue;
			}

			//深圳证券交易所
			if (iMarketPar == 1)
			{
				stockinfo.shtMarket = 0;
			}
			//上海证券交易所
			else if (iMarketPar == 2)
			{
				stockinfo.shtMarket = 1;
			}
			else
			{
				continue;
			}

			stockinfo.sCode = stRecord["STK_CODE"];
			stockinfo.sName = stRecord["STK_SHORT_NAME"];
			string sUnitCode = stRecord["STK_UNI_CODE"];

			string sMarketCode = TC_Common::tostr<short>(stockinfo.shtMarket) + stockinfo.sCode;

			TC_ThreadWLock lock(_lock);
			_mUni2StockCache[sUnitCode] = stockinfo;
			_mMarketCode2StockCache.insert(make_pair(sMarketCode, stockinfo));
		}

		TLOG_DEBUG("stockCache cost time is " << COST_NOW_THIS << "ms" << "|" << "num and StockCache size is " << num << "|" << _mUni2StockCache.size() << endl);
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


void StockDateUpdate::run()
{
	while(true)
	{
	    try
        {
	    	dataUpdate();

			{
				//1分钟更新一次
				TC_ThreadLock::Lock sync(_dataLock);
				_dataLock.timedWait(60000);
			}
        }
        catch (exception &e)
        {
            TLOG_ERROR("exception:" << e.what() << endl);
        }
        catch(...)
        {
            TLOG_ERROR("unknown exception" << endl);
        }

	}
}

void StockDateUpdate::dataUpdate()
{
	TELL_TIME_COST_THIS;

	CComDateTime comTime = CComDateTime::GetCurrentTime();
	int iMinute = comTime.GetHour() * 60 + comTime.GetMinute();

	//定时更新股票基本信息缓存
	if (0 == iMinute % _InfoTime)
	{
		updateUni2Stock();
	}

	//定时更新quant_fct_value_row_q_fresh
	if (0 == iMinute%_iFreshTime)
	{
		updateFreshTable();
	}

	//定时更新实时行情数据
	if (0 == iMinute % _HqTime)
	{
		updateStockCondHq();
	}

	//更新quant_fct_value_row_d最新的数据时期
	{
		string sSql = "SELECT MAX(END_DATE) as END_DATE FROM quant_fct_value_row_d WHERE SEC_UNI_CODE in (2010001577,2010001611)  AND A7041000001 > 0;";
		TLOG_DEBUG("query sql is " << sSql << endl);

		string sLastDate("");
		getLastDateBySql(sSql, "END_DATE", sLastDate);
		if (sLastDate.size() == END_DATE_LEN)
		{
			TC_ThreadWLock lock(_lock);
			_sDayLastDate = sLastDate;
			TLOG_DEBUG("quant_fct_value_row_d last date is " << _sDayLastDate << endl);
		}
		else
		{
			TLOG_ERROR("quant_fct_value_row_d END_DATE is error.END_DATE is " << sLastDate << endl);
		}

		//更新quant_fct_value_row_d的日期数据
		updateDayEndDate();
	}

	//更新quant_stk_calc_d最新的数据时期
	{
		string sSql = "SELECT MAX(ymd) as ymd FROM quant_stk_calc_d WHERE gscode='FCT_MACD';";
		TLOG_DEBUG("query sql is " << sSql << endl);

		string sLastDate("");
		getLastDateBySql(sSql, "ymd", sLastDate);
		if (sLastDate.size() == YMD_LEN)
		{
			TC_ThreadWLock lock(_lock);
			_sCalcLastDate = sLastDate;
			TLOG_DEBUG("quant_stk_calc_d last date is " << _sCalcLastDate << endl);
		}
		else
		{
			TLOG_ERROR("quant_stk_calc_d ymd is error.ymd is " << sLastDate << endl);
		}
	}

	TLOG_DEBUG("update cost time is " << COST_NOW_THIS << "ms" << endl);
}

void StockDateUpdate::updateDayEndDate()
{
	try
	{
		TELL_TIME_COST_THIS;

		vector<string> vDate;
		stringstream sSql;
		sSql << "select distinct END_DATE from quant_fct_value_row_d order by END_DATE asc";
		TC_Mysql::MysqlData data = _sql.queryRecord(sSql.str());
		if (0 == data.size())
		{
			//没有查询到对应的结果
			TLOG_ERROR("TC_Mysql::MysqlData  is NULL.Query Sql is " << sSql << endl);
			return;
		}

		for (size_t i = 0; i< data.size(); i++)
		{
			string sDate = TC_Common::trim(data[i]["END_DATE"]);
			if (!sDate.empty())
			{
				vDate.push_back(sDate);
			}
		}

		TC_ThreadWLock lock(_lock);
		_vEndDate.swap(vDate);

		TLOG_DEBUG("updateDayEndDate cost time is " << COST_NOW_THIS << "ms" << "|" << "_vEndDate size is " << _vEndDate.size() << endl);
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

void StockDateUpdate::getLastDateBySql(const string &sSql, const string &sValue, string &sLastDate)
{
	if (sSql.empty() || sValue.empty())
	{
		return;
	}

	try
	{
		//查询数据,获取结果集
		TC_Mysql::MysqlData  stMysqlData = _sql.queryRecord(sSql);
		if (0 == stMysqlData.size())
		{
			//没有查询到对应的结果
			TLOG_ERROR("TC_Mysql::MysqlData is NULL.Query Sql is " << sSql << endl);
			return;
		}

		//读取结果,此时size应该等于1
		if (1 == stMysqlData.size())
		{
			HQExtend::SelStockInfo stkinfo;
			TC_Mysql::MysqlRecord stRecord = stMysqlData[0];
			string sDate = TC_Common::trim(stRecord[sValue]);
			if (!sDate.empty())
			{
				sLastDate = sDate;
			}
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

void StockDateUpdate::convertStockCondHq(const HQSys::HStockHq &stStock, vector<HQExtend::SelStockCondHq> &vStockCondHq, map<string, HQExtend::SelStockCondHq> &mStockCondHq)
{
	HQExtend::SelStockCondHq stStk;

	stStk.stStockPrice.stInfo.shtMarket = stStock.shtSetcode;
	stStk.stStockPrice.stInfo.sCode = stStock.sCode;
	stStk.stStockPrice.stInfo.sName = stStock.sName;
	stStk.stStockPrice.stPrice.dPrice = stStock.stSimHq.fNowPrice;
	stStk.stStockPrice.stPrice.dChange = stStock.stSimHq.fChgValue;
	stStk.stStockPrice.stPrice.dChangeRate = stStock.stSimHq.fChgRatio;

	stStk.stCondHq.dChgRatio = stStock.stSimHq.fChgRatio;
	stStk.stCondHq.dTurnoverRate = stStock.stExHq.fTurnoverRate;
	stStk.stCondHq.dZhenfu = stStock.stSimHq.fZhenfu;
	stStk.stCondHq.dAmount = stStock.stSimHq.fAmount;
	stStk.stCondHq.dVolume = stStock.stSimHq.lVolume;
	stStk.stCondHq.dNowPrice = stStock.stSimHq.fNowPrice;
	stStk.stCondHq.dLiangBi = stStock.stDeriveHq.dLiangBi;

	//计算委比
	double wlc = 0.0, wl = 0.0;
	size_t iNum = stStock.stExHq.vBuyv.size();
	for (size_t i = 0; i < iNum; i++)
	{
		wlc = wlc + stStock.stExHq.vBuyv[i] - stStock.stExHq.vSellv[i];
		wl = wl + stStock.stExHq.vBuyv[i] + stStock.stExHq.vSellv[i];
	}
	
	if (wl > 0.001)
	{
		stStk.stCondHq.dWeiBi = (100.f * wlc) / wl;
	}


	if (!getStockBaseInfo(stStock, stStock.stSimHq.fNowPrice, stStk))
	{
		//获取失败，再一次请求这只股票
		getStockBaseInfo(stStock, stStock.stSimHq.fNowPrice, stStk);
	}

	stStk.stCondHqAddition.bTransactionStatus = stStock.stExHq.bTransactionStatus;

	string sMarketCode = TC_Common::tostr(stStk.stStockPrice.stInfo.shtMarket) + stStk.stStockPrice.stInfo.sCode;
		 
	vStockCondHq.push_back(stStk);
	mStockCondHq.insert(make_pair(sMarketCode, stStk));
}

bool StockDateUpdate::getStockBaseInfo(const HQSys::HStockHq &stStock, double dNowPrice, HQExtend::SelStockCondHq &stStk)
{
	try
	{
		HStockBaseInfoReq stReq;
		stReq.stHeader.shtMarket = stStock.shtSetcode;
		HStockUnique stUnique;
		stUnique.shtSetcode = stStock.shtSetcode;
		stUnique.sCode = stStock.sCode;
		stReq.vStock.push_back(stUnique);
		HStockBaseInfoRsp stRsp;
		if (0 != _basicPrx->stockBaseInfo(stReq, stRsp))
		{
			TLOG_ERROR("can't get stockBaseInfo,stock is  " << TC_Common::tostr(stStock.shtSetcode) << "|" << stStock.sCode << endl);
			return false;
		}

		if (!stRsp.vStockInfo.empty())
		{
			stStk.iType = stRsp.vStockInfo[0].iType;
			if (stRsp.vStockInfo[0].dDTSY > 0.001 || stRsp.vStockInfo[0].dDTSY < -0.001)
			{
				stStk.stCondHq.dPeRatio = dNowPrice / stRsp.vStockInfo[0].dDTSY;
			}

			stStk.stCondHq.dTotalMarketValue = stRsp.vStockInfo[0].fTotalMarketValue;
			return true;
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

	return false;
}
