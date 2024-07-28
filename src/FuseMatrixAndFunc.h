#pragma once
#include "IRGeneralOperation.h"
class FuseMatrixAndFunc
{
public:
	vector<BrStructInfo> BrStructs;
	vector<PointInfo> KeyInfoList;
	vector<PointInfo> PointInfoList;
	int CallInstLoction(BasicBlock * Block, BasicBlock * FBlock);
	int FuseTwoInOne(BasicBlock * FBlock, int index, int bs);
	void FuseBranchBlocks(Function * Func);
	void FuseSequenceBlocks(PointInfo FusedPoint, BasicBlock * FusedBlock);
	void FuseBranchBlocks(PointInfo FusedPoint, BrStructInfo FuseStruct);
	void FuseTwoBlocks(PointInfo FusedPoint, BrStructInfo FuseStruct);
	void VerifyBlock();
	void FuseMatrixAndFunc::Fuse();
	
	
protected:
	map<BasicBlock*, BasicBlock*> m_map;;
	GlobalVariable *stage = NULL;
	
};
