#include "ConditionServer.h"
#include "ConditionImp.h"
#include "GlobalConfig.h"
#include "StockDateUpdate.h"

using namespace std;

ConditionServer g_app;

/////////////////////////////////////////////////////////////////
void
ConditionServer::initialize()
{
	//initialize application here:
	//...

	addServant<ConditionImp>(ServerConfig::Application + "." + ServerConfig::ServerName + ".ConditionObj");
	addConfig(ServerConfig::ServerName + ".conf");
	addConfig("mysql.conf");
	addConfig("factor.conf");

	GlobalConfig::getInstance()->initialize();

	//启动更新数据库中的最新日期和股票uni线程
	StockDateUpdate::getInstance()->start();
}
/////////////////////////////////////////////////////////////////
void
ConditionServer::destroyApp()
{
	//destroy application here:
	//...
}
/////////////////////////////////////////////////////////////////
int
main(int argc, char* argv[])
{
	try
	{
		g_app.main(argc, argv);
		g_app.waitForShutdown();
	}
	catch (std::exception& e)
	{
		cerr << "std::exception:" << e.what() << std::endl;
	}
	catch (...)
	{
		cerr << "unknown exception." << std::endl;
	}
	return -1;
}
/////////////////////////////////////////////////////////////////
