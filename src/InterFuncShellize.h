#pragma once
#include "IRGeneralOperation.h"
class InterFuncShellize
{
public:
	vector<BrStructInfo> BrStructs;
	void GenFuncShellize(Function* Func);
	
protected:
	//void BranchAnalysis(BasicBlock * entry);
	
};
