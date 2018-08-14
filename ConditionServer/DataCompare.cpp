#include "DataCompare.h"
#include <map>

using namespace std;
using namespace HQExtend;
using namespace HQSys;

CondStockCompare::CondStockCompare(int sortFct, int colType, int sortType) :sortFct(sortFct), colType(colType), sortType(sortType)
{

}

bool CondStockCompare::operator()(const HQExtend::SelectedCondStock & a, const HQExtend::SelectedCondStock & b)
{
	bool res = false;
	//老的排序规则
	if (colType == FACTOR_NONE)
	{
		E_CONDITION_FACTOR eFct = (E_CONDITION_FACTOR)sortFct;
		switch (eFct)
		{
		case FACTOR_NONE:
			res = a.stStockPrice.stPrice.dChangeRate > b.stStockPrice.stPrice.dChangeRate;
			break;

		default:
			//迭代器实现
			double dA, dB;
			map<E_CONDITION_FACTOR, double>::const_iterator it = a.mFctList.find(eFct);
			if (it == a.mFctList.end())
			{
				TLOG_ERROR("a.mFctList is not find eFct,aStock is " << a.stStockPrice.stInfo.shtMarket << ":" << a.stStockPrice.stInfo.sCode << endl);
				break;
			}
			else
			{
				dA = it->second;
			}

			map<E_CONDITION_FACTOR, double>::const_iterator iter = b.mFctList.find(eFct);
			if (iter == b.mFctList.end())
			{
				TLOG_ERROR("b.mFctList is not find eFct,bStock is " << b.stStockPrice.stInfo.shtMarket << ":" << b.stStockPrice.stInfo.sCode << endl);
				break;
			}
			else
			{
				dB = iter->second;
			}

			res = dA < dB;
			break;
			//res = a.mFctList.at(eFct) < b.mFctList.at(eFct);
		}

		return res;
	}
	//新的排序规则
	else
	{
		//不排序
		if (E_SORT_DEFAULT == sortType)
		{
			return false;
		}

		E_CONDITION_FACTOR eFct = (E_CONDITION_FACTOR)colType;
		switch (eFct)
		{
		case FACTOR_NAME:
			res = a.stStockPrice.stInfo.sName < b.stStockPrice.stInfo.sName;
			break;
		case FACTOR_LASTPRICE:
			res = a.stStockPrice.stPrice.dPrice < b.stStockPrice.stPrice.dPrice;
			break;

		default:
			//迭代器实现
			double dA, dB;
			map<E_CONDITION_FACTOR, double>::const_iterator it = a.mFctList.find(eFct);
			if (it == a.mFctList.end())
			{
				TLOG_ERROR("a.mFctList is not find eFct,aStock is " << a.stStockPrice.stInfo.shtMarket << ":" << a.stStockPrice.stInfo.sCode << endl);
				break;
			}
			else
			{
				dA = it->second;
			}

			map<E_CONDITION_FACTOR, double>::const_iterator iter = b.mFctList.find(eFct);
			if (iter == b.mFctList.end())
			{
				TLOG_ERROR("b.mFctList is not find eFct,bStock is " << b.stStockPrice.stInfo.shtMarket << ":" << b.stStockPrice.stInfo.sCode << endl);
				break;
			}
			else
			{
				dB = iter->second;
			}

			res = dA < dB;
			break;
			//res = a.mFctList.at(eFct) < b.mFctList.at(eFct);
		}

		if (HQSys::E_SORT_ASCEND == sortType)
		{
			return res;
		}
		else
		{
			return !res;
		}
	}

	return res;
}
