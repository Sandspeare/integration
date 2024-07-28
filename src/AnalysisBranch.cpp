/*****************************************************************************
Filename: AnalysisBranch.cpp
Date    : 2024-07-10 15:21:42
Description: Find the blocks suitable for integration and split based on the control flow
*****************************************************************************/

#include "AnalysisBranch.h"

void AnalysisBranch::BranchAnalysis(BasicBlock* entry)
{
	if (IfBlockInlist(entry, BrStructs[BrStructs.size() - 1].BranchBlocks))
		return;
	BrStructs[BrStructs.size() - 1].BranchBlocks.push_back(entry);
	for (BasicBlock::iterator Ii = entry->begin(); Ii != entry->end(); Ii++)
	{
		Value* tempVal = &*Ii;
		if (BranchInst* BrInst = dyn_cast<BranchInst>(tempVal))
		{
			for (int i = 0; i < BrInst->getNumSuccessors(); i++)
			{
				if (entry == BrStructs[BrStructs.size() - 1].BranchBlocks[0] && BrInst->getSuccessor(i) != entry->getNextNode())
				{
					BrStructs[BrStructs.size() - 1].BranchBlocks.push_back(BrInst->getSuccessor(i));
					BrStructs[BrStructs.size() - 1].entry = entry;
					BrStructs[BrStructs.size() - 1].end = BrInst->getSuccessor(i);
					BranchAnalysis(BrInst->getSuccessor(i));
				}
			}
			for (int i = 0; i < BrInst->getNumSuccessors(); i++)
			{
				BranchAnalysis(BrInst->getSuccessor(i));
			}
		}
	}
	return;
}


void AnalysisBranch::AnalysisFunctionBranch(Function * Func)
{
	vector<BasicBlock*> tempBlockList;
	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		Bi->setName(string(Bi->getName()) + ".formatrix");
	}
	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		if (IfBlockInlist(&*Bi, tempBlockList))
			continue;
		tempBlockList.push_back(&*Bi);
		for (BasicBlock::iterator Ii = Bi->begin(); Ii != Bi->end(); Ii++)
		{
			if (BranchInst* BrInst = dyn_cast<BranchInst>(Ii))
			{
				if (BrInst->getNumSuccessors() > 1)
				{
					BrStructInfo CurBrStructInfo;
					CurBrStructInfo.IsBranch = TRUE;
					BrStructs.push_back(CurBrStructInfo);
					BranchAnalysis(&*Bi);				//Analyze branch to get all blocks
					for (int i = 0; i < BrStructs[BrStructs.size() - 1].BranchBlocks.size(); i++)
					{
						// Blocks only considered once
						tempBlockList.push_back(BrStructs[BrStructs.size() - 1].BranchBlocks[i]);	
					}
				}
			}
		}
	}

	for (int i = 0; i < BrStructs.size(); i++)
	{
		vector<BasicBlock*> tempBlocks;
		for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
		{
			if (IfBlockInlist(&*Bi, BrStructs[i].BranchBlocks))
			{
				tempBlocks.push_back(&*Bi);
			}
		}
		BrStructs[i].BranchBlocks = tempBlocks;
		BrStructs[i].entry = BrStructs[i].BranchBlocks[0];
	}
}

int AnalysisBranch::GetFusedPointSize()
{
	FILE* fp = NULL;
	char buf[1000000] = { 0 };
	fopen_s(&fp, ptFile.c_str(), "r");
	if (fp)
	{
		fread(buf, 1, 1000000, fp);
	}
	int sum = 0;
	cJSON* root = NULL;
	root = cJSON_Parse(buf);
	if (!root)
	{
		printf("Error before %s\n", cJSON_GetErrorPtr());
	}
	else
	{
		cJSON * item_json = root->child;
		while (item_json != NULL)
		{
			sum++;
			item_json = item_json->next;

		}
	}

	cJSON_Delete(root);
	fclose(fp);
	return sum;
}


bool AnalysisBranch::SplitFunctionBranch(Function* Func)
{
	int PointSize = GetFusedPointSize() - 1;
	errs() << Func->getName() << "\n";
	
	vector<BasicBlock*> MayEntryBlocks;
	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		for (BasicBlock::iterator Ii = Bi->begin(); Ii != Bi->end(); Ii++)
		{
			if (BranchInst* BrInst = dyn_cast<BranchInst>(Ii))
			{
				if (BrInst->getNumSuccessors() > 1)
				{
					MayEntryBlocks.push_back(&*Bi);
					break;
				}
			}
		}
	}
	for (int i = 0; i < MayEntryBlocks.size(); i++)
	{
		Instruction* SplitLoc = &*MayEntryBlocks[i]->getFirstInsertionPt();
		MayEntryBlocks[i]->splitBasicBlock(SplitLoc, "");
	}

	if (Func->getBasicBlockList().size() > PointSize)
		return FALSE;

	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		if ((&*Bi)->getInstList().size() < 8)
			continue;
		Instruction* SplitLoc = (&*Bi)->front().getNextNode()->getNextNode()->getNextNode()->getNextNode()->getNextNode()->getNextNode()->getNextNode();
		(&*Bi)->splitBasicBlock(SplitLoc, "");
		if (Func->getBasicBlockList().size() == PointSize - 2)
			break;
	}

	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		for (BasicBlock::iterator Ii = Bi->begin(); Ii != Bi->end(); Ii++)
		{
			if (Ii->getType() != Type::getVoidTy(Func->getParent()->getContext()))
			{
				Ii->setName(Ii->getName() + ".matrix");
			}
		}
	}

	AnalysisFunctionBranch(Func);

	vector<BasicBlock*> tempBlocks;
	// re order
	for (int i = 0; i < BrStructs.size(); i++)
	{
		for (int j = 0; j < BrStructs[i].BranchBlocks.size(); j++)
		{
			tempBlocks.push_back(BrStructs[i].BranchBlocks[j]);
		}
	}

	vector<BrStructInfo> tempBrStructs;
	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		if (IfBlockInlist(&*Bi, tempBlocks))
		{
			// re order
			for (int i = 0; i < BrStructs.size(); i++)
			{
				if (BrStructs[i].entry == &*Bi)
				{
					tempBrStructs.push_back(BrStructs[i]);
				}
			}
		}
		else
		{
			BrStructInfo CurBrStructInfo;
			CurBrStructInfo.IsBranch = FALSE;
			CurBrStructInfo.entry = &*Bi;
			tempBrStructs.push_back(CurBrStructInfo);
		}
	}

	BrStructs = tempBrStructs;
	return TRUE;
}
