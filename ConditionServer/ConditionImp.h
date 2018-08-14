#ifndef _ConditionImp_H_
#define _ConditionImp_H_

#include "servant/Application.h"
#include "Condition.h"
#include "BasicHq.h"
#include "ConditionFactor.h"

/**
 *
 *
 */
class ConditionImp : public HQExtend::Condition
{
public:
	/**
	*
	*/
	ConditionImp();
	/**
	 *
	 */
	virtual ~ConditionImp() {}

	/**
	 *
	 */
	virtual void initialize();

	/**
	 *
	 */
    virtual void destroy();

	/**
	 *
	 */

	// 方式一
    virtual taf::Int32 selectStock(const HQExtend::SelectReq & stReq, HQExtend::SelectRsp &stRsp,taf::JceCurrentPtr current);

	// 方式二:条件选股用这个方式实现
	virtual taf::Int32 selectCondStock(const HQExtend::SelectCondReq & stReq, HQExtend::SelectCondRsp &stRsp, taf::JceCurrentPtr current);

	// 方式三
    virtual taf::Int32 queryStock(const std::string & sQueryStr,HQExtend::SelectRsp &stRsp,taf::JceCurrentPtr current);

protected:

	//////////////////////////////方式一:begin///////////////////////////

	vector<HQExtend::SelectedStock> _mergeStockList(vector<HQExtend::SelectedStock> &vList1, vector<HQExtend::SelectedStock> &vList2, HQExtend::E_CONDTION_OPERATION eOp);
	vector<HQExtend::SelectedStock> _parseGroup(HQExtend::ConditionGroup stSrc);
	typedef vector<HQExtend::SelectedStock>::iterator selIter;
	void _stockIntersect(selIter first1, selIter last1, selIter first2, selIter last2, selIter result, size_t &num);
	static bool _selCompare(const HQExtend::SelectedStock &a, const HQExtend::SelectedStock &b);

	//////////////////////////////方式一:end/////////////////////////////

	//////////////////////////////方式二:begin///////////////////////////

	int _selectFactorStock(const HQExtend::ConditionItem &stItem, vector<HQExtend::SelectedCondStock> &vStock);
	bool _isSortFactor(HQExtend::E_CONDITION_FACTOR eFct);
	bool _containStockPrice(HQExtend::E_CONDITION_FACTOR eFct);
	vector<HQExtend::SelectedCondStock> _mergeStockList(const vector<HQExtend::SelectedCondStock> &vList1, const vector<HQExtend::SelectedCondStock> &vList2, bool bPrice);
	bool _fillDestList(vector<HQExtend::SelectedCondStock> &vDestList);
	//重设排序规则（兼容老的排序规则）
	vector<HQExtend::SelectedCondStock> _sortByFct(const vector<HQExtend::SelectedCondStock> &vDestList, HQExtend::E_CONDITION_FACTOR eSortFct, 
		HQExtend::E_CONDITION_FACTOR eColumn, HQSys::E_SORT_METHOD eSort);

	ConditionFactorPtr getFctHandler(HQExtend::E_CONDITION_FACTOR eFct);

	//////////////////////////////方式二:end/////////////////////////////

private:
	HQSys::BasicHqPrx _basicPrx;
	taf::TC_ThreadRWLocker _fctLock;
	taf::TC_Mysql _sql;
	map<HQExtend::E_CONDITION_FACTOR, ConditionFactorPtr> _mFctHandler;
};
/////////////////////////////////////////////////////
#endif
