#pragma once
#include "IRGeneralOperation.h"
class AnalysisBranch
{
public:
	
	vector<BrStructInfo> BrStructs;
	vector<PointInfo> PointInfoList;
	
	string ptFile;
	void AnalysisFunctionBranch(Function* Func);
	int GetFusedPointSize();
	bool SplitFunctionBranch(Function * Func);


protected:
	void BranchAnalysis(BasicBlock * entry);
	
};
