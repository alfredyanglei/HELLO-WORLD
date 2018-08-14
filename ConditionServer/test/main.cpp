#include "servant/Application.h"
#include "util/tc_option.h"
#include "Condition.h"
#include <math.h>
#include "StreamUtil.h"
#include "CountTimeApp.h"
#include "Common.h"


using namespace taf;
using namespace HQExtend;
using namespace HQSys;

TC_Option g_option;
ConditionPrx g_prx;

TC_Atomic g_RecvCount = 0;

void testCond(bool bDisplay = true)
{
	ConditionItem c1;
	c1.eFactor = FACTOR_MARKET;
	c1.iValue = 3;

	ConditionItem c2;
	c2.eFactor = FACTOR_B2S;
	c2.sCode = "880015";

	vector<ConditionItem> vCondList;
	vCondList.push_back(c1);
	vCondList.push_back(c2);

	SelectCondReq req;
	req.vCondList = vCondList;

	if (bDisplay)
	{
		req.display(cout);
	}

	SelectCondRsp rsp;
	int iRet = g_prx->selectCondStock(req, rsp);
	if (bDisplay)
	{
		cout << iRet << endl;
		if (0 == iRet)
		{
			rsp.display(cout);
		}
	}
}

void testSQL(bool bDisplay = true)
{
	ConditionItem c1;
	c1.eFactor = FACTOR_PB;
	c1.dMinValue = 1;
	c1.dMaxValue = 10;

	ConditionItem c2;
	c2.eFactor = FACTOR_ZGB;
	c2.dMinValue = 10000;
	c2.dMaxValue = 6000000000000;

	ConditionItem c3;
	c3.eFactor = FACTOR_MACD;
	c3.dMinValue = 10000;
	c3.dMaxValue = 6000000000000;
	c3.iValue = 1;

	vector<ConditionItem> vCondList;
	vCondList.push_back(c1);
	vCondList.push_back(c2);
	vCondList.push_back(c3);

	SelectCondReq req;
	req.vCondList = vCondList;

	if (bDisplay)
	{
		req.display(cout);
	}

	SelectCondRsp rsp;
	int iRet = g_prx->selectCondStock(req, rsp);
	if (bDisplay)
	{
		cout << iRet << endl;
		if (0 == iRet)
		{
			rsp.display(cout);
		}
	}
}

void testSingleCondition(bool bDisplay = true)
{
	ConditionItem c1;
	c1.eFactor = g_option.hasParam("factor") ? (E_CONDITION_FACTOR)TC_Common::strto<int>(g_option.getValue("factor")) : FACTOR_PB;
	c1.dMinValue = g_option.hasParam("min") ? TC_Common::strto<double>(g_option.getValue("min")) : 0.f;
	c1.dMaxValue = g_option.hasParam("max") ? TC_Common::strto<double>(g_option.getValue("max")) : 3.f;
	c1.iValue = g_option.hasParam("value") ? TC_Common::strto<int>(g_option.getValue("value")) : 1;
	c1.sCode = g_option.hasParam("code") ? g_option.getValue("code") : "880022";

	vector<ConditionItem> vCondList;
	vCondList.push_back(c1);

	SelectCondReq req;
	req.vCondList = vCondList;
	req.eColumn = g_option.hasParam("col") ? (E_CONDITION_FACTOR)TC_Common::strto<int>(g_option.getValue("col")) : FACTOR_PB;
	req.eSort = g_option.hasParam("sort") ? (E_SORT_METHOD)TC_Common::strto<int>(g_option.getValue("sort")) : E_SORT_DEFAULT;


	if (bDisplay)
	{
		req.display(cout);
	}

	SelectCondRsp rsp;
	int iRet = g_prx->selectCondStock(req, rsp);
	if (bDisplay)
	{
		cout << iRet << endl;
		if (0 == iRet)
		{
			rsp.display(cout);
		}
	}
}

class BenchThread : public TC_Thread, public TC_HandleBase
{
public:
	virtual void run()
	{
		int type = TC_Common::strto<int>(g_option.getValue("type"));
		bTerminate = false;
		while (!bTerminate)
		{
			try
			{
				if (type == 0)
				{
					testCond(false);
				}
				else if (type == 1)
				{
					testSQL(false);
				}
				else if (type == 2)
				{
					testSingleCondition(false);
				}

				g_RecvCount += 1;
			}
			catch (exception &e)
			{
				cerr << "exception:" << e.what() << endl;
			}
			catch (...)
			{
				cerr << "unknown exception" << endl;
			}

		}
	}
public:
	bool bTerminate;
};

typedef TC_AutoPtr<BenchThread> BenchThreadPtr;

void testBenchmark()
{

	Application::getCommunicator()->setProperty("sendqueuelimit", "10000");
	//    long sendCount = g_option.hasParam("count") ? TC_Common::strto<long>( g_option.getValue("count") ) : 100;
	int curCurrent = g_option.hasParam("thread") ? TC_Common::strto<int>(g_option.getValue("thread")) : 1;


	vector<BenchThreadPtr> vThread;
	for (int i = 0; i<curCurrent; i++)
	{
		BenchThreadPtr thread = new BenchThread();
		thread->start();
		vThread.push_back(thread);
	}
	int64_t begin = TNOWMS;
	while (1)
	{
		int cost = TNOWMS - begin;
		if (cost > 0 && g_RecvCount > 10)
		{
			cout << "totol request:" << g_RecvCount << ",cost=" << cost << "ms" << ",tps=" << g_RecvCount * 1000 / cost << "" << endl;
		}
		sleep(1);
	}
}

int main(int argc, char ** argv)
{
    try
    {
        g_option.decode(argc, argv);

        string sServerDefault = "HQExtend.ConditionServer.ConditionObj@tcp -h 172.16.8.150 -t 60000 -p 10000";

        string sServer = g_option.hasParam("server") ? g_option.getValue("server") : sServerDefault;
        g_prx =  Application::getCommunicator()->stringToProxy<ConditionPrx>(sServer);

		int req = g_option.hasParam("req") ? TC_Common::strto<int>(g_option.getValue("req")) : 1;
		switch (req)
		{
		case 1:
			testCond();
			break;
		case 2:
			testSQL();
			break;
		case 3:
			testSingleCondition();
			break;

		case 99:
			testBenchmark();
			break;
		default:
			cout << "req is not know." << req << endl;
		}

        return 0;
    }
    catch (exception &e)
    {
        cout << "exception:" << e.what() << endl;
    }
    catch(...)
    {
        cout << "unknown exception" << endl;
    }
    return -1;

}
