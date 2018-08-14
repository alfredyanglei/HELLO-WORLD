#include "ConditionImp.h"
#include "servant/Application.h"
#include "util/tc_common.h"
#include "StreamUtil.h"
#include "CountTimeApp.h"
#include "ConditionFactor.h"
#include "GlobalConfig.h"
#include "DataCompare.h"
#include "util/tc_common.h"

using namespace std;
using namespace taf;
using namespace HQExtend;
using namespace HQSys;

//////////////////////////////////////////////////////

ConditionImp::ConditionImp() :_sql(GlobalConfig::getInstance()->getDBConf())
{

}

void ConditionImp::initialize()
{
	//initialize servant here:
	_basicPrx = Application::getCommunicator()->stringToProxy<BasicHqPrx>(GlobalConfig::getInstance()->getBasicHqObj());

	try
	{
		//连接数据库
		_sql.connect();
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

//////////////////////////////////////////////////////
void ConditionImp::destroy()
{
	//destroy servant here:
	//...
}

taf::Int32 ConditionImp::selectCondStock(const HQExtend::SelectCondReq & stReq, HQExtend::SelectCondRsp &stRsp, taf::JceCurrentPtr current)
{
	stReq.display(LOG->debug());
	int iRet = CONDITION_UNKNOWN;
	TELL_TIME_COST_THIS;
	try
	{
		do
		{
			vector<HQExtend::SelectedCondStock> vDestList;  //合并后的交集
			E_CONDITION_FACTOR eSortFct = FACTOR_NONE;      //排序因子
			for (size_t i = 0; i < stReq.vCondList.size(); i++)
			{
				//如果为空，说明是无效的factor
				if (stReq.vCondList[i].eFactor == HQExtend::FACTOR_NONE)
				{
					iRet = CONDITION_FCT_NONE;
					break;
				}

				//找到排序因子
				if (eSortFct == FACTOR_NONE && _isSortFactor(stReq.vCondList[i].eFactor))
				{
					eSortFct = stReq.vCondList[i].eFactor;
					TLOG_DEBUG(" find SortFct is " << etos(eSortFct) << endl);
				}
				
				//获取因子股票列表
				vector<HQExtend::SelectedCondStock> vStock;
				int res = _selectFactorStock(stReq.vCondList[i], vStock);
				if (0 != res)
				{
					TLOG_ERROR("selectFactorStock is not find. eFactor:" << etos(stReq.vCondList[i].eFactor) << endl);
					iRet = CONDITION_FCT_NO_DATA;
					break;
				}

				TLOG_DEBUG("selectFactorStock:vStock is " << etos(stReq.vCondList[i].eFactor) << ":" << vStock.size() << endl);

				//对股票列表取交集
				if (i > 0)
				{
					//vStock的因子数据是否有SelSimpleStockPrice数据
					bool bContainPrice = _containStockPrice(stReq.vCondList[i].eFactor);
					vDestList = _mergeStockList(vDestList, vStock, bContainPrice);
				}
				//将第一个结果存储
				else
				{
					vDestList = vStock;
				}

				if (vDestList.empty())
				{
					iRet = CONDITION_INTERSECTION_NO_DATA;
					break;
				}

				iRet = CONDITION_OK;
			}
			
			//结果集按指定因子列排序
			if (CONDITION_OK == iRet)
			{
				//TODO填充vDestList里部分没有行情数据的SelSimpleStockPrice（sql获取的股票没有赋值行情数据）
				if (_fillDestList(vDestList))
				{
					stRsp.eSortFct = eSortFct;
					stRsp.vStock = _sortByFct(vDestList, eSortFct, stReq.eColumn, stReq.eSort);
				}
			}

		} while (0);

	}
	catch (exception &e)
	{
		TLOG_ERROR("exception:" << e.what() << endl);
	}
	catch (...)
	{
		TLOG_ERROR("unknown exception" << endl);
	}

	ostringstream oss;
	stReq.displaySimple(oss);
	FDLOG("condition") << iRet << "|" << oss.str() << "|" << stRsp.vStock.size() << "|" << COST_NOW_THIS << "ms" << endl;
	return iRet;
}

ConditionFactorPtr ConditionImp::getFctHandler(HQExtend::E_CONDITION_FACTOR eFct)
{
	TLOG_DEBUG("eFct:" << etos(eFct) << endl);
	if (_mFctHandler.count(eFct) == 0)
	{
		//生成handler
		TC_ThreadWLock lock(_fctLock);
		switch (eFct)
		{
		//市场
		case FACTOR_MARKET:
			_mFctHandler[eFct] = new CondFctMarket();
			break;
		case FACTOR_B2S:
			_mFctHandler[eFct] = new CondFctB2S();
			break;

		//技术面计算指标
		case FACTOR_MACD:
		case FACTOR_KDJ:
		case FACTOR_BOLL:
		case FACTOR_RSI:
		case FACTOR_KXT:

		//数据库已有数据因子
		case FACTOR_PB:
		case FACTOR_PS:
		case FACTOR_PCF:
		case FACTOR_LTSZ:
		case FACTOR_ZGB:
		case FACTOR_GDHS:
		case FACTOR_HJCG:
		case FACTOR_MGSYZZL:
		case FACTOR_XSMLL:
		case FACTOR_YYZSR:
		case FACTOR_YYZSRZZL:
		case FACTOR_JLR:
		case FACTOR_JLRZZL:
		case FACTOR_ZCFZL:

		//数据库待增加数据因子
		case FACTOR_ZZCBCL:
		case FACTOR_ZCCJLL:
		case FACTOR_MGJYXJL:
		case FACTOR_MGSY:
		case FACTOR_MGJZC:
		case FACTOR_MGWFPLR:
		case FACTOR_MGZBGJ:
		case FACTOR_MGGL:
		case FACTOR_JZCSYL:

		//龙虎榜数据
		case FACTOR_LHB_JGJMR:
		case FACTOR_LHB_JGJMC:
		case FACTOR_LHB_YYBJMR:
		case FACTOR_LHB_YYBJMC:
		case FACTOR_LHB_MJGMR:
		case FACTOR_LHB_MJGMC:
		case FACTOR_LHB_SBN:

			_mFctHandler[eFct] = new CondFctSQL(_sql);
			break;

			//行情趋势
		case FACTOR_PE:
		case FACTOR_ZSZ:
		case FACTOR_CHG:
		case FACTOR_HSL:
		case FACTOR_ZHENFU:
		case FACTOR_AMOUNT:
		case FACTOR_VOLUME:
		case FACTOR_PRICE:
		case FACTOR_LIANGBI:
		case FACTOR_WEIBI:
			_mFctHandler[eFct] = new CondFctHqTrend();
			break;

		default:
			break;
		}
	}

	TC_ThreadRLock lock(_fctLock);
	if (_mFctHandler.count(eFct) > 0)
	{
		return _mFctHandler[eFct];
	}
	return NULL;
}

int ConditionImp::_selectFactorStock(const HQExtend::ConditionItem & stItem, vector<HQExtend::SelectedCondStock> &vStock)
{
	HQExtend::E_CONDITION_FACTOR eFct = (HQExtend::E_CONDITION_FACTOR)stItem.eFactor;
	ConditionFactorPtr fctHandler = getFctHandler(eFct);
	if (!fctHandler)
	{
		TLOG_ERROR("unsupported fct:" << etos(eFct) << endl);
		return -1;
	}

	int iRet = fctHandler->selectCondStock(stItem, vStock);

	return iRet;
}

bool ConditionImp::_isSortFactor(HQExtend::E_CONDITION_FACTOR eFct)
{
	bool bRet = false;
	switch (eFct)
	{
	case FACTOR_MACD:
	case FACTOR_KDJ:
	case FACTOR_BOLL:
	case FACTOR_RSI:
	case FACTOR_KXT:
	case FACTOR_MARKET:
	case FACTOR_B2S:
		break;
	default:
		bRet = true;
		break;
	}

	return bRet;
}

bool ConditionImp::_containStockPrice(HQExtend::E_CONDITION_FACTOR eFct)
{
	bool bRet = true;
	switch (eFct)
	{
	case FACTOR_MARKET:
	case FACTOR_B2S:
	case FACTOR_PE:
	case FACTOR_ZSZ:
	case FACTOR_CHG:
	case FACTOR_HSL:
	case FACTOR_ZHENFU:
	case FACTOR_AMOUNT:
	case FACTOR_VOLUME:
	case FACTOR_PRICE:
	case FACTOR_LIANGBI:
	case FACTOR_WEIBI:
		break;
	default:
		bRet = false;
		break;
	}

	return bRet;
}

vector<HQExtend::SelectedCondStock> ConditionImp::_mergeStockList(const vector<HQExtend::SelectedCondStock> &vList1, const vector<HQExtend::SelectedCondStock> &vList2, bool bPrice)
{
	TELL_TIME_COST_THIS;

	//将二个股票列表合并的结果集
	vector<HQExtend::SelectedCondStock> vDest;

	if (vList1.empty() || vList2.empty())
	{
		TLOG_ERROR("vList have empty.vList1:vList2 size is " << vList1.size() << ":" << vList2.size() << endl);
		return vDest;
	}

	//分配容器大小，提高效率
	unsigned int iRealSize = vList1.size() > vList2.size() ? vList2.size() : vList1.size();
	vDest.reserve(iRealSize);

	map<string, pair<bool, SelectedCondStock> > mStockTemp;
	//遍历vList1生成临时的map,bool来标记已经被找到的股票
	for (size_t i = 0; i < vList1.size(); i++)
	{
		string sMarketCode = TC_Common::tostr(vList1[i].stStockPrice.stInfo.shtMarket) + vList1[i].stStockPrice.stInfo.sCode;
		mStockTemp[sMarketCode] = pair<bool, SelectedCondStock>(false, vList1[i]);
	}

	//找交集并去重
	for (size_t j = 0; j < vList2.size(); j++)
	{
		string sMarketCode = TC_Common::tostr(vList2[j].stStockPrice.stInfo.shtMarket) + vList2[j].stStockPrice.stInfo.sCode;
		map<string, pair<bool,SelectedCondStock> >::iterator itr = mStockTemp.find(sMarketCode);
		if (itr != mStockTemp.end())
		{
			if (itr->second.first == true)
			{
				continue;
			}

			HQExtend::SelectedCondStock stStock;
			if (bPrice)
			{
				stStock.stStockPrice = vList2[j].stStockPrice;
				stStock.bTransactionStatus = vList2[j].bTransactionStatus;
			}
			else
			{
				stStock.stStockPrice = itr->second.second.stStockPrice;
				stStock.bTransactionStatus = itr->second.second.bTransactionStatus;
			}

			//增加vList2的mFctList,如果factor是不显示数值的因子map就为空
			if (!vList2[j].mFctList.empty())
			{
				itr->second.second.mFctList.insert(vList2[j].mFctList.begin(), vList2[j].mFctList.end());
			}

			stStock.mFctList.swap(itr->second.second.mFctList);
			vDest.push_back(stStock);

			//标记找到的股票
			itr->second.first = true;
		}
	}

	TLOG_DEBUG("Merge StockList cost time is " << vDest.size() << "|" << COST_NOW_THIS << "ms" << endl);
	return vDest;
}

bool ConditionImp::_fillDestList(vector<HQExtend::SelectedCondStock> &vDestList)
{
	TELL_TIME_COST_THIS;

	if (vDestList.empty())
	{
		return false;
	}

	//可能会存在部分股票（sql查询）没有精简行情
	for (size_t i = 0; i < vDestList.size(); i++)
	{
		//筛出从数据库获取的股票，通过行情获取的数据都有price，停牌的股票也有price和close
		if (vDestList[i].stStockPrice.stPrice.dPrice < 0.001)
		{
			string sMarketCode = TC_Common::tostr(vDestList[i].stStockPrice.stInfo.shtMarket) + vDestList[i].stStockPrice.stInfo.sCode;
			HQExtend::SelStockCondHq stStockCondHq;
			if (false == StockDateUpdate::getInstance()->getStockCondHqByMarketCode(sMarketCode, stStockCondHq))
			{
				TLOG_ERROR("getStockCondHqByMarketCode sMarketCode is not find. sMarketCode is" << sMarketCode << endl);
				continue;
			}

			vDestList[i].stStockPrice = stStockCondHq.stStockPrice;
			vDestList[i].bTransactionStatus = stStockCondHq.stCondHqAddition.bTransactionStatus;
		}
	}

	TLOG_DEBUG("fill simpleStockPrice cost time is " << vDestList.size() << "|" << COST_NOW_THIS << "ms" << endl);
	return true;
}

vector<HQExtend::SelectedCondStock> ConditionImp::_sortByFct(const vector<HQExtend::SelectedCondStock> &vDestList, HQExtend::E_CONDITION_FACTOR eSortFct, HQExtend::E_CONDITION_FACTOR eColumn, E_SORT_METHOD eSort)
{
	//将二个股票列表合并的结果集
	vector<HQExtend::SelectedCondStock> vDest;

	if (vDestList.empty())
	{
		TLOG_ERROR("vDestList is empty." << endl);
		return vDest;
	}

	TELL_TIME_COST_THIS;

	//分配容器大小，提高效率
	unsigned int iRealSize = vDestList.size();
	vDest.reserve(iRealSize);

	//排序
	CondStockCompare stCmp(eSortFct, eColumn, eSort);
	//使用iterator，减少一次复制操作
	multiset<SelectedCondStock, CondStockCompare> setStock(stCmp);
	for (vector<SelectedCondStock>::const_iterator itr = vDestList.begin(); itr != vDestList.end(); itr++)
	{
		setStock.insert(*itr);
	}
	multiset<SelectedCondStock, CondStockCompare>::iterator it = setStock.begin();
	for (; it != setStock.end(); it++)
	{
		vDest.push_back(*it);
	}

	TLOG_DEBUG("sort cost time is " << iRealSize << "|" << COST_NOW_THIS << "ms" << endl);
	return vDest;

}

taf::Int32 ConditionImp::selectStock(const HQExtend::SelectReq& stReq, HQExtend::SelectRsp& stRsp, taf::JceCurrentPtr current)
{
	return 0;
}

//比较函数
bool ConditionImp::_selCompare(const HQExtend::SelectedStock &a, const HQExtend::SelectedStock &b)
{
	if (a.shtMarket == b.shtMarket)
	{
		return a.sCode < b.sCode;
	}
	else
	{
		return a.shtMarket < b.shtMarket;
	}
}

//集合交集
void ConditionImp::_stockIntersect(selIter first1, selIter last1, selIter first2, selIter last2, selIter result, size_t &num)
{
	while (first1 != last1 && first2 != last2)
	{
		//在两个区间内分别移动迭代器。若二者值相同用result记录该值，移动first1,first2和result 
		//若first1较小，则移动first1，其他不动  
		//若first2较小，则移动first2，其他不动  
		if (_selCompare(*first1, *first2))
		{
			first1++;
		}
		else if (_selCompare(*first2, *first1))
		{
			first2++;
		}
		else
		{
			*result = *first1;
			first1++;
			first2++;
			result++;
			num++;
		}
	}
}

vector<HQExtend::SelectedStock> ConditionImp::_mergeStockList(vector<HQExtend::SelectedStock> &vList1, vector<HQExtend::SelectedStock> &vList2, HQExtend::E_CONDTION_OPERATION eOp)
{
	//根据规则，将二个股票列表合并
	vector<HQExtend::SelectedStock> vDest;

	TLOG_DEBUG("OP is " << eOp << "vList1 size is " << vList1.size() << " vList2 size is " << vList2.size() << endl);
	//实现and或者or逻辑
	if (HQExtend::OP_OR == eOp)
	{
		//集合并集
		vDest.insert(vDest.end(), vList1.begin(), vList1.end());
		vDest.insert(vDest.end(), vList2.begin(), vList2.end());

		//去重
		vector<HQExtend::SelectedStock>::iterator iter;
		::sort(vDest.begin(), vDest.end(), ConditionImp::_selCompare);
		iter = unique(vDest.begin(), vDest.end());
		if (iter != vDest.end())
		{
			vDest.erase(iter, vDest.end());
		}
	}
	else if (HQExtend::OP_AND == eOp)
	{
		//集合交集
		size_t maxNum = vList1.size() < vList2.size() ? vList1.size() : vList2.size();
		vDest.resize(maxNum);
		sort(vList1.begin(), vList1.end(), ConditionImp::_selCompare);
		sort(vList2.begin(), vList2.end(), ConditionImp::_selCompare);

		size_t num = 0;
		_stockIntersect(vList1.begin(), vList1.end(), vList2.begin(), vList2.end(), vDest.begin(), num);
		TLOG_DEBUG("intersect num is " << num << endl);
		vDest.resize(num);
	}
	else
	{
		vDest.clear();
	}

	TLOG_DEBUG("OP is " << eOp << "vDest size is " << vDest.size() << endl);
	return vDest;
}

vector<HQExtend::SelectedStock> ConditionImp::_parseGroup(HQExtend::ConditionGroup stSrc)
{
	//如果不是空，说明是有效的item
	if (stSrc.stItem.eFactor != HQExtend::FACTOR_NONE)
	{
		//根据item获取对应股票列表
		vector<HQExtend::SelectedStock> vStock;
		int iRet = 0; //_selectFactorStock(stSrc, vStock);
		if (0 != iRet)
		{
			TLOG_ERROR("_selectFactorStock is not find. eFactor:" << stSrc.stItem.eFactor << endl);
		}

		TLOG_DEBUG(" _selectFactorStock:vStock is " << stSrc.stItem.eFactor << ":" << vStock.size() << endl);
		return vStock;
	}

	//如果不是item，则解析vSubCondition
	vector<HQExtend::SelectedStock> vDestList;
	for (size_t i = 0; i < stSrc.vSubCondition.size(); i++)
	{
		//获取股票列表
		vector<HQExtend::SelectedStock> vStock = _parseGroup(stSrc.vSubCondition[i]);

		//根据op，对股票列表进行合并
		if (i > 0)
		{
			vDestList = _mergeStockList(vDestList, vStock, stSrc.eSubOp);
		}
		//将第一个结果存储
		else
		{
			vDestList = vStock;
		}
	}
	return vDestList;
}

taf::Int32 ConditionImp::queryStock(const std::string& sQueryStr, HQExtend::SelectRsp& stRsp, taf::JceCurrentPtr current)
{
	LOG_DEBUG << "sQueryStr" << sQueryStr << endl;
	return 0;
}