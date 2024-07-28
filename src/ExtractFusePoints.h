#pragma once
#include "IRGeneralOperation.h"
class ExtractFusePoints
{
public:
	vector<PointInfo> KeyInfoList;
	vector<PointInfo> PointInfoList;
	

	void ExtractFusePoints::MatrixInitial(Module* mod, string ptFile);
	
protected:
	//void BranchAnalysis(BasicBlock * entry);
	void ExtractFusePoints::ParseJsonFile(Module* mod, string ptFile);
	Instruction* ExtractFusePoints::GetKeyPointByJson(Module *mod, string PointName, int value);

	Instruction* GetFusedPointByJson(Module *mod, string PointName, int value, string valuestring);
	
};
