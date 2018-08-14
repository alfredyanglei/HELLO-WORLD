/*
 * GlobalConfig.cpp
 *
 *  Created on: 2017年1月20日
 *      Author: 8888
 */

#include "GlobalConfig.h"

using namespace taf;


GlobalConfig::GlobalConfig()
{
    // TODO Auto-generated constructor stub

}

GlobalConfig::~GlobalConfig()
{
    // TODO Auto-generated destructor stub
}

void GlobalConfig::initialize()
{
	{
		//读取obj配置
		TC_Config conf;
		conf.parseFile(ServerConfig::BasePath + ServerConfig::ServerName + ".conf");
		_sBasicHqObj = conf.get("/conf/obj<HQSys.basicHq>", "HQSys.UnifyAccessServer.BasicHqObj");
		_FreshTime = TC_Common::strto<int>(conf.get("/conf/fixtime<freshtime>", "60"));
		_InfoTime = TC_Common::strto<int>(conf.get("/conf/fixtime<infotime>", "720"));
		_HqTime = TC_Common::strto<int>(conf.get("/conf/fixtime<hqtime>", "10"));
	}


	{
		//读取mysql配置
		TC_Config conf;
		conf.parseFile(ServerConfig::BasePath + "mysql.conf");
		map<string, string> mSql = conf.getDomainMap("/conf/sql");
		_dbConf.loadFromMap(mSql);
	}

	{
		//读取factor配置
		TC_Config conf;
		conf.parseFile(ServerConfig::BasePath + "factor.conf");
		_mFctValue = conf.getDomainMap("/conf/fct");
		_mTable = conf.getDomainMap("/conf/table");

		_mMacdValue = conf.getDomainMap("/conf/macd");
		_mKdjValue = conf.getDomainMap("/conf/kdj");
		_mBollValue = conf.getDomainMap("/conf/boll");
		_mRsiValue = conf.getDomainMap("/conf/rsi");
		_mKxtValue = conf.getDomainMap("/conf/kxt");

		_sRowList = conf.get("/conf/freshrowlist<rowlist>", "CREATETIME,UPDATETIME,SEC_UNI_CODE,END_DATE,DECL_DATE");
	}

}

string GlobalConfig::getFactorValue(HQExtend::E_CONDITION_FACTOR eFct)
{
	string sFct = etos(eFct);
	map<string, string>::iterator itr = _mFctValue.find(sFct);
	if (itr != _mFctValue.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getTableName(HQExtend::E_CONDITION_TABLE eTable)
{
	string sTableName = etos(eTable);
	map<string, string>::iterator itr = _mTable.find(sTableName);
	if (itr != _mTable.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getMacdValue(int iType)
{
	HQExtend::E_FACTOR_MACD eMacd = (HQExtend::E_FACTOR_MACD)iType;
	string sMacd = etos(eMacd);
	map<string, string>::iterator itr = _mMacdValue.find(sMacd);
	if (itr != _mMacdValue.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getKdjValue(int iType)
{
	HQExtend::E_FACTOR_KDJ eKdj = (HQExtend::E_FACTOR_KDJ)iType;
	string sKdj = etos(eKdj);
	map<string, string>::iterator itr = _mKdjValue.find(sKdj);
	if (itr != _mKdjValue.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getBollValue(int iType)
{
	HQExtend::E_FACTOR_BOLL eBoll = (HQExtend::E_FACTOR_BOLL)iType;
	string sBoll = etos(eBoll);
	map<string, string>::iterator itr = _mBollValue.find(sBoll);
	if (itr != _mBollValue.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getRsiValue(int iType)
{
	HQExtend::E_FACTOR_RSI eRsi = (HQExtend::E_FACTOR_RSI)iType;
	string sRsi = etos(eRsi);
	map<string, string>::iterator itr = _mRsiValue.find(sRsi);
	if (itr != _mRsiValue.end())
	{
		return itr->second;
	}

	return "";
}

string GlobalConfig::getKxtValue(int iType)
{
	HQExtend::E_FACTOR_KXT eKxt = (HQExtend::E_FACTOR_KXT)iType;
	string sKxt = etos(eKxt);
	map<string, string>::iterator itr = _mKxtValue.find(sKxt);
	if (itr != _mKxtValue.end())
	{
		return itr->second;
	}

	return "";
}