/*
 * GlobalConfig.h
 *
 *  Created on: 2017年1月20日
 *      Author: 8888
 */

#ifndef GLOBALCONFIG_H_
#define GLOBALCONFIG_H_
#include "servant/Application.h"
#include "Common.h"
#include "Condition.h"
#include "util/tc_mysql.h"
#include "util/tc_common.h"

class GlobalConfig : public TC_Singleton<GlobalConfig>
{
public:
    GlobalConfig();
    virtual ~GlobalConfig();
public:
    void initialize();
    inline string getBasicHqObj(){return _sBasicHqObj;}
	inline taf::TC_DBConf getDBConf(){return _dbConf;}
	inline int getFreshTime(){ return _FreshTime; }
	inline int getInfoTime(){ return _InfoTime; }
	inline int getHqTime(){ return _HqTime; }
	inline string getRowList(){ return _sRowList; }

	string getFactorValue(HQExtend::E_CONDITION_FACTOR eFct);
	string getTableName(HQExtend::E_CONDITION_TABLE eTable);

	string getMacdValue(int iType);
	string getKdjValue(int iType);
	string getRsiValue(int iType);
	string getBollValue(int iType);
	string getKxtValue(int iType);

private:
    string _sBasicHqObj;
	taf::TC_DBConf _dbConf;

	int _FreshTime; //quant_fct_value_row_q_fresh更新数据时间
	int _InfoTime;  //stk_basic_info更新数据时间
	int _HqTime;    //更新最新行情数据时间

	//因子对应值
	map<string, string> _mFctValue;

	//因子对应的查询表
	map<string, string> _mTable;

	//技术指标因子枚举对应列
	map<string, string> _mMacdValue;
	map<string, string> _mKdjValue;
	map<string, string> _mBollValue;
	map<string, string> _mRsiValue;
	map<string, string> _mKxtValue;
	string _sRowList;
};

#endif /* GLOBALCONFIG_H_ */
