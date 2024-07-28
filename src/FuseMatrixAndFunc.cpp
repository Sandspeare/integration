/*****************************************************************************
Filename: FuseMatrixAndFunc.cpp
Date    : 2024-07-17 8:42:51
Description: Integrate matrix code and source code
*****************************************************************************/
#include "AnalysisBranch.h"
#include "InterFuncShellize.h"
#include "FuseMatrixAndFunc.h"

BasicBlock* GetLastRetBlock(Function* Func)
{
	for (Function::iterator Bi = Func->end(); Bi != Func->begin(); Bi--)
	{
		for (BasicBlock::iterator I = Bi->begin(), Ie = Bi->end(); I != Ie; ++I)
		{
			if (ReturnInst* inst = dyn_cast<ReturnInst>(&*I))
			{
				return &*Bi;
			}
		}
	}
}

void FuseMatrixAndFunc::FuseSequenceBlocks(PointInfo FusedPoint, BasicBlock* FusedBlock)
{
	Module* mod = FusedBlock->getParent()->getParent();
	IRBuilder<> builder(mod->getContext());
	BasicBlock* SrcBlock = FusedPoint.point->getParent();
	BasicBlock* DesBlock = SrcBlock->splitBasicBlock(FusedPoint.point->getNextNode(), SrcBlock->getName() + ".split");
	BasicBlock* FinalretBlock = GetLastRetBlock(FusedBlock->getParent());
	ValueToValueMapTy VMap;
	BasicBlock* CloneBB = BasicBlock::Create(mod->getContext(), FusedBlock->getName(), FusedBlock->getParent(), DesBlock);
	AllocaInst *IndexInst = new AllocaInst(Type::getInt32Ty(mod->getContext()), 0, "indexLoc", CloneBB);
	for (BasicBlock::iterator I = FusedBlock->begin(), Ie = FusedBlock->end(); I != Ie; ++I) {
		Instruction *new_inst = I->clone();
		VMap[&*I] = new_inst;
		llvm::RemapInstruction(new_inst, VMap, RF_IgnoreMissingEntries);
		new_inst->setName(I->getName());
		new_inst->insertBefore(IndexInst);

	}
	IndexInst->eraseFromParent();

	if (BranchInst* inst = dyn_cast<BranchInst>(&CloneBB->back()))
	{
		inst->eraseFromParent();
	}

	for (BasicBlock::iterator I = CloneBB->begin(), Ie = CloneBB->end(); I != Ie; ++I)
	{
		// There may be a return instruction in the branch. 
		// If so, point the return instruction to the original control flow position to restore normal code execution
		if (ReturnInst* inst = dyn_cast<ReturnInst>(&*I))
		{
			if (FusedBlock == FinalretBlock)
			{
				inst->eraseFromParent();
				break;
			}
		}
	}

	Instruction* ins = SrcBlock->back().getPrevNode();		// integrate point
	SrcBlock->back().eraseFromParent();


	if (FusedPoint.value == 0)
	{
		BranchInst::Create(CloneBB, SrcBlock);
		BranchInst::Create(DesBlock, CloneBB);
	}
	else
	{
		
		ICmpInst * cmpinst = NULL;
		if (FusedPoint.IsUNK == FALSE)
		{
			if (FusedPoint.point->getType() == Type::getInt32Ty(mod->getContext()))
			{
				cmpinst = new ICmpInst(*SrcBlock, CmpInst::ICMP_EQ, ins, builder.getInt32(FusedPoint.value), ins->getName() + ".cmp");
			}
			else
			{
				cmpinst = new ICmpInst(*SrcBlock, CmpInst::ICMP_EQ, ins, builder.getInt8(FusedPoint.value), ins->getName() + ".cmp");
			}
			BranchInst::Create(CloneBB, DesBlock, (Value*)cmpinst, SrcBlock);
			BranchInst::Create(DesBlock, CloneBB);
		}
		else
		{
			GlobalVariable *number = new GlobalVariable(*mod, Type::getInt32Ty(mod->getContext()), false, GlobalValue::InternalLinkage, builder.getInt32(0), FusedPoint.point->getName() + "PointGV");
			LoadInst* load_GV = new LoadInst(number, "load_gv", false, ins);
			ICmpInst * cmpinst = new ICmpInst(*SrcBlock, FCmpInst::ICMP_EQ, builder.getInt32(0), load_GV, "cmp_gv");
			StoreInst* store2GV = new StoreInst(builder.getInt32(1), number, false, ins);

			BranchInst::Create(CloneBB, DesBlock, (Value*)cmpinst, SrcBlock);

			
			BranchInst::Create(DesBlock, CloneBB);

		}
	}

	Instruction* Split = &SrcBlock->back();
	BasicBlock* temp = SrcBlock->splitBasicBlock(Split, ".compare");
	Instruction* InsertLoc = &SrcBlock->back();
	LoadInst* load_Stage = new LoadInst(stage, "load_stage", false, InsertLoc);
	ICmpInst * cmpinst = new ICmpInst(*SrcBlock, FCmpInst::ICMP_EQ, load_Stage, builder.getInt32(0), "cmp_stage");
	BranchInst::Create(DesBlock, temp, (Value*)cmpinst, SrcBlock);
	InsertLoc->eraseFromParent();

}


void FuseMatrixAndFunc::FuseBranchBlocks(PointInfo FusedPoint, BrStructInfo FuseStruct)
{
	Module* mod = FuseStruct.entry->getParent()->getParent();
	IRBuilder<> builder(mod->getContext());
	BasicBlock* SrcBlock = FusedPoint.point->getParent();
	BasicBlock* DesBlock = SrcBlock->splitBasicBlock(FusedPoint.point->getNextNode(), SrcBlock->getName() + ".split");
	BasicBlock* entry = NULL;
	BasicBlock* end = NULL;
	

	BasicBlock* FinalretBlock = GetLastRetBlock(FuseStruct.entry->getParent());
	for (int i = 0; i < FuseStruct.BranchBlocks.size(); i++)
	{
		ValueToValueMapTy VMap;
		BasicBlock* CloneBB = BasicBlock::Create(mod->getContext(), FuseStruct.BranchBlocks[i]->getName(), DesBlock->getParent(), DesBlock);

		AllocaInst *IndexInst = new AllocaInst(Type::getInt32Ty(mod->getContext()), 0, "indexLoc", CloneBB);
		for (BasicBlock::iterator I = FuseStruct.BranchBlocks[i]->begin(), Ie = FuseStruct.BranchBlocks[i]->end(); I != Ie; ++I) {
			Instruction *new_inst = I->clone();
			VMap[&*I] = new_inst;
			llvm::RemapInstruction(new_inst, VMap, RF_IgnoreMissingEntries);
			new_inst->setName(I->getName());
			new_inst->insertBefore(IndexInst);

		}
		IndexInst->eraseFromParent();
		// If is terminate block, delete the branch instruction and add a new branch instruction
		if (FuseStruct.BranchBlocks[i] == FuseStruct.end)
		{
			end = CloneBB;				
			if (BranchInst* inst = dyn_cast<BranchInst>(&CloneBB->back()))
			{
				inst->eraseFromParent();
			}
		}
		// If entry block
		if (FuseStruct.BranchBlocks[i] == FuseStruct.entry)
		{
			entry = CloneBB;
		}
		
		for (BasicBlock::iterator I = CloneBB->begin(), Ie = CloneBB->end(); I != Ie; ++I)
		{
			// There may be a return instruction in the branch. 
			// If so, point the return instruction to the original control flow position to restore normal code execution
			if (CallInst* inst = dyn_cast<CallInst>(&*I))
			{
				if (inst->getCalledFunction() == mod->getFunction("exit"))
				{
					
					StoreInst* store2GV = new StoreInst(builder.getInt32(0), stage, false, &CloneBB->back());
					
					BranchInst::Create(DesBlock, &CloneBB->back());
					&CloneBB->back().eraseFromParent();
					inst->eraseFromParent();
					break;
				}
			}
			if (ReturnInst* inst = dyn_cast<ReturnInst>(&*I))
			{
				if (FuseStruct.BranchBlocks[i] == FinalretBlock)
				{
					inst->eraseFromParent();
					break;
				}
			}
		}
	}
	
	Instruction* ins = SrcBlock->back().getPrevNode();
	SrcBlock->back().eraseFromParent();
	ICmpInst * cmpinst = NULL;
	if (FusedPoint.value == 0)
	{
		BranchInst::Create(entry, SrcBlock);
		BranchInst::Create(DesBlock, end);
	}
	else
	{

		if (FusedPoint.IsUNK == FALSE)
		{
			if (FusedPoint.point->getType() == Type::getInt32Ty(mod->getContext()))
			{
				cmpinst = new ICmpInst(*SrcBlock, CmpInst::ICMP_EQ, ins, builder.getInt32(FusedPoint.value), ins->getName() + ".cmp");
			}
			else
			{
				cmpinst = new ICmpInst(*SrcBlock, CmpInst::ICMP_EQ, ins, builder.getInt8(FusedPoint.value), ins->getName() + ".cmp");
			}
			BranchInst::Create(entry, DesBlock, (Value*)cmpinst, SrcBlock);
			BranchInst::Create(DesBlock, end);
		}
		else
		{
			GlobalVariable *number = new GlobalVariable(*mod, Type::getInt32Ty(mod->getContext()), false, GlobalValue::InternalLinkage, builder.getInt32(0), FusedPoint.point->getName() + "PointGV");
			LoadInst* load_GV = new LoadInst(number, "load_gv", false, ins);
			ICmpInst * cmpinst = new ICmpInst(*SrcBlock, FCmpInst::ICMP_EQ, builder.getInt32(0), load_GV, "cmp_gv");
			StoreInst* store2GV = new StoreInst(builder.getInt32(1), number, false, ins);

			BranchInst::Create(entry, DesBlock, (Value*)cmpinst, SrcBlock);

			

			BranchInst::Create(DesBlock, end);
		}
	}

	Instruction* Split = &SrcBlock->back();
	BasicBlock* temp = SrcBlock->splitBasicBlock(Split, ".compare");
	Instruction* InsertLoc = &SrcBlock->back();
	LoadInst* load_Stage = new LoadInst(stage, "load_stage", false, InsertLoc);
	cmpinst = new ICmpInst(*SrcBlock, FCmpInst::ICMP_EQ, load_Stage, builder.getInt32(0), "cmp_stage");
	BranchInst::Create(DesBlock, temp, (Value*)cmpinst, SrcBlock);
	InsertLoc->eraseFromParent();
}

void FuseMatrixAndFunc::FuseTwoBlocks(PointInfo FusedPoint, BrStructInfo FuseStruct)
{
	if (FuseStruct.IsBranch == TRUE)
	{
		FuseBranchBlocks(FusedPoint, FuseStruct);
	}
	else
	{
		FuseSequenceBlocks(FusedPoint, FuseStruct.entry);
	}
}

void FuseMatrixAndFunc::Fuse()
{
	float step = float((PointInfoList.size() - 3 )/ (BrStructs.size() + 1));
	Module* mod = BrStructs[0].entry->getParent()->getParent();
	IRBuilder<> builder(mod->getContext());
	srand(time(NULL));
	stage = new GlobalVariable(*mod, Type::getInt32Ty(mod->getContext()), false, GlobalValue::InternalLinkage, builder.getInt32(1), "StageForMatrix");
	errs() << PointInfoList.size() << " : " << BrStructs.size() << "\n";
	for (int i = 0; i < BrStructs.size(); i++)
	{
		int index = rand() % (int(step - 1)) + int((step * i));
		FuseTwoBlocks(PointInfoList[index], BrStructs[i]);		
	}
	
}