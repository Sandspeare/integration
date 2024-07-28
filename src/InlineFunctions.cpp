/*****************************************************************************
Filename: InlineFunctions.cpp
Date    : 2024-07-15 12:35:44
Description: Inline all functions into one single function
*****************************************************************************/
#include "InlineFunctions.h"
BOOL IsRecurseFunction(Function* Func)
{
	for (llvm::Value::user_iterator Useri = Func->user_begin(), Usere = Func->user_end(); Useri != Usere; Useri++)
	{
		if (Instruction* inst = dyn_cast<Instruction>(*Useri))
		{
			if (inst->getParent()->getParent() == Func)
			{
				return true;
			}
		}
	}
	return false;
}

Function* InlineFunctions::InlineAllFunc(string FuncBCName)
{
	Module *mod = MyParseIRFile(FuncBCName);
	vector<CallInst*> CallList;
	vector<Function*> NoUseFuncs;
	for (Function &F : *mod)
	{
		for (BasicBlock &B : F)
		{
			B.setName(B.getName() + ".formatrix");
			for (Instruction &I : B)
			{
				if (CallInst* CI = dyn_cast<CallInst>(&I))
				{
					Function* tempFunc = CI->getCalledFunction();
					if (tempFunc == NULL)
						continue;
					if (tempFunc->isVarArg())
						continue;
					if (IsRecurseFunction(tempFunc))
						continue;
					if (tempFunc->hasDLLImportStorageClass())
						continue;
					if (tempFunc->isDeclaration())
						continue;
					if (tempFunc->getName().find("llvm.") != string::npos)
						continue;
					if (tempFunc->getName().find("noinline") != string::npos)
						continue;
					CallList.push_back(CI);
					if (IfFuncInlist(tempFunc, NoUseFuncs) == 0)
						NoUseFuncs.push_back(tempFunc);
				}
			}
		}
	}

//！！！！！！！！do inline！！！！！！！！！！！！！！
	for (int i = 0; i < CallList.size(); i++)
	{
		InlineFunctionInfo IFI;
		if (InlineFunction(CallList[i], IFI) == 0)
		{
			Function* tempFunc = CallList[i]->getCalledFunction();
		}
	}
//！！！！！！！！delete inlined functions！！！！！！！！！！！！！！
	
	for (int i = 0; i < NoUseFuncs.size(); i++)		
	{
		if (NoUseFuncs[i]->getNumUses() == 0)
		{
			NoUseFuncs[i]->eraseFromParent();
		}
	}

	Function* Func;
	for (Function &F : *mod)	
	{
		if (F.getName().find("DataDecodeForMatrix") != string::npos)
		{
			errs() << F.getName() << "\n";
			Func = &F;
		}
	}

//！！！！！！！！modify alloca instruction into global variable！！！！！！！！！！！！！！
	vector<Instruction*> AllocaInsts;
	for (Function &F : *mod)
	{
		for (BasicBlock &B : F)
		{
			for (Instruction &I : B)
			{
				if (AllocaInst* Ii = dyn_cast<AllocaInst>(&I))
				{
					AllocaInsts.push_back(Ii);
				}
			}
		}
	}
	for (int i = 0; i < AllocaInsts.size(); i++)
	{
		LoadInst* LoadAlloca = new LoadInst(AllocaInsts[i], "temp", false, AllocaInsts[i]);
		GlobalVariable* Alloca_GV = new GlobalVariable(*mod, LoadAlloca->getType(), false, GlobalValue::InternalLinkage, Constant::getNullValue(LoadAlloca->getType()), AllocaInsts[i]->getName() + ".GV");

		AllocaInsts[i]->replaceAllUsesWith(Alloca_GV);
		AllocaInsts[i]->eraseFromParent();
		LoadAlloca->eraseFromParent();
	}

	return Func;
}