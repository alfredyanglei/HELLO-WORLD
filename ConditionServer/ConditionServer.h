#ifndef _ConditionServer_H_
#define _ConditionServer_H_

#include <iostream>
#include "servant/Application.h"

using namespace taf;

/**
 *
 **/
class ConditionServer : public Application
{
public:
	/**
	 *
	 **/
	virtual ~ConditionServer() {};

	/**
	 *
	 **/
	virtual void initialize();

	/**
	 *
	 **/
	virtual void destroyApp();
};

extern ConditionServer g_app;

////////////////////////////////////////////
#endif
