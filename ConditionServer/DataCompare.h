/*
* DataCompare.h
*
*  Created on: 2017年2月9日
*      Author: 8888
*/

#ifndef _DATACOMPARE_H
#define _DATACOMPARE_H

#include "Condition.h"
#include "servant/Application.h"
#include "BasicHq.h"

using namespace HQExtend;

class CondStockCompare
{
public:
	CondStockCompare(int sortFct, int colType, int sortType);
	bool operator()(const HQExtend::SelectedCondStock & a, const HQExtend::SelectedCondStock & b);

private:
	int sortFct;
	int colType;
	int sortType;
};

#endif
