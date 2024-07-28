/*****************************************************************************
Filename: InterFuncShellize.cpp
Date    : 2020-07-01 20:43:08
Description: Retrive structure for local variable.
*****************************************************************************/

#include "AnalysisBranch.h"
#include "InterFuncShellize.h"

void InterFuncShellize::GenFuncShellize(Function * Func)
{
	IRBuilder<> builder(Func->getParent()->getContext());
	TransConstant2Inst(Func);
	vector<Instruction*> Gi_list;
	for (Function::iterator Bi = Func->begin(); Bi != Func->end(); Bi++)
	{
		for (BasicBlock::iterator Ii = Bi->begin(); Ii != Bi->end(); Ii++)
		{
			for (Value::user_iterator Useri = Ii->user_begin(), Usere = Ii->user_end(); Useri != Usere; Useri++)
			{
				if (Instruction* inst = dyn_cast<Instruction>(*Useri))
				{
					if (inst->getParent() != Ii->getParent())
					{
						int flag = 0;
						for (int i = 0; i < BrStructs.size(); i++)
						{
							if (IfBlockInlist(inst->getParent(), BrStructs[i].BranchBlocks) && IfBlockInlist(Ii->getParent(), BrStructs[i].BranchBlocks))
							{
								flag = 1;
							}
						}
						if (flag == 0)
						{
							Gi_list.push_back(&*Ii);
						}
						break;
					}	
				}
			}
		}
	}

	vector<Type*> BigStructFiled;
	for (int i = 0; i < Gi_list.size(); i++)
	{
		BigStructFiled.push_back(Gi_list[i]->getType());
	}

	StructType* BigStructType = StructType::create(Func->getParent()->getContext(), "struct.BigStruct");
	BigStructType->setBody(BigStructFiled, true);
	GlobalVariable* BigGV = new GlobalVariable(*Func->getParent(), BigStructType, false, GlobalValue::InternalLinkage, Constant::getNullValue(BigStructType), "struct.BigStruct");
	BigGV->setAlignment(4);

	
	for (int i = 0; i < Gi_list.size(); i++)
	{
		vector<Value*> vect;
		vect.push_back(builder.getInt32(0));
		vect.push_back(builder.getInt32(i));

		vector<Instruction* > UserList;
		for (Value::user_iterator Useri = Gi_list[i]->user_begin(), Usere = Gi_list[i]->user_end(); Useri != Usere; Useri++)
		{
			if (Instruction* inst = dyn_cast<Instruction>(*Useri))
			{
				UserList.push_back(inst);
			}
		}

		for (int j = 0; j < UserList.size(); j++)
		{
			GetElementPtrInst *gep1 = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(i), UserList[j]);
			LoadInst* loadRetValue = new LoadInst(gep1, "load_" + gep1->getName(), false, UserList[j]);
			UserList[j]->replaceUsesOfWith(Gi_list[i], loadRetValue);
		}

		Instruction* InsertLoc = Gi_list[i]->getNextNode();
		GetElementPtrInst *gep1 = GetElementPtrInst::Create(BigStructType, BigGV, vect, BigStructType->getName() + "_" + to_string(i), InsertLoc);
		StoreInst* temp = new StoreInst(Gi_list[i], gep1, InsertLoc);

	}

	return;
}
