/*****************************************************************************
Filename: IRGeneralOperation.cpp
Date    : 2020-02-10 11:25:35
Description: Common functions
*****************************************************************************/
#include "IRGeneralOperation.h"

using namespace llvm;

void TransConstant2Inst(Function *Func)
{
	while (1)
	{
		int flag = 0;
		for (BasicBlock &Block : *Func)
		{
			for (Instruction &I : Block)
			{
				for (int k = 0; k < I.getNumOperands(); k++)
				{
					if (ConstantExpr* CExpr = dyn_cast<ConstantExpr>(I.getOperand(k)))
					{
						flag = 1;
						Instruction* CE2Inst = CExpr->getAsInstruction();
						I.replaceUsesOfWith(CExpr, CE2Inst);
						CE2Inst->insertBefore(&I);
					}
				}
			}
		}
		if (flag == 0)
			break;
	}
}

Module* MyParseIRFile(StringRef filename)
{
	SMDiagnostic Err;
	Module* M = NULL;
	std::unique_ptr<Module> mod = nullptr;
	LLVMContext& globalContext = llvm::getGlobalContext();
	mod = llvm::parseIRFile(filename, Err, globalContext);
	if(!mod)
	{
		Err.print("Open Module file error", errs());
		return NULL;
	}
	M = mod.get();
	if (!M)
	{
		errs() << ": error loading file '" << filename << "'\n";
		return NULL;
	}
	mod.release();
	return M;
}

//write module to a ll file
bool doWriteBackLL(Module* M, StringRef filename)
{
	std::error_code ErrorInfo;
	std::unique_ptr<tool_output_file> out(new tool_output_file(filename, ErrorInfo, llvm::sys::fs::F_None));
	
	if(ErrorInfo)
	{
		errs() << ErrorInfo.message() << "\n";
		return false;
	}
	
	M->print(out->os(), NULL);
	out->keep();
	return true;
}

//write module to a bc file
bool doWriteBackBC(Module* M, StringRef filename)
{
	std::error_code ErrorInfo;
	std::unique_ptr<tool_output_file> out(new tool_output_file(filename, ErrorInfo, llvm::sys::fs::F_None));

	if (ErrorInfo)
	{
		errs() << ErrorInfo.message() << "\n";
		return false;
	}
	
	WriteBitcodeToFile(M, out->os());
	out->keep();
	return true;
}

std::string RetrieveFuncName(Function *F)
{
	std::string TmpName = "";
	std::string FuncRealName = "";
	int Pos1 = 0;
	int Pos2 = 0;
	TmpName = F->getName();

	Pos1 = TmpName.find_first_of("?");

	if (Pos1 == std::string::npos) 
	{
		if ((Pos1 = TmpName.find_first_of("_")) == std::string::npos)
		{
			FuncRealName = TmpName;
			return FuncRealName;
		}
	}

	Pos1 += 1;
	Pos2 = TmpName.find_first_of("@");
	if (Pos2 == std::string::npos)
		Pos2 = TmpName.size() - 1;
	else
		Pos2--;

	FuncRealName.clear();
	FuncRealName.append(TmpName, Pos1, Pos2 - Pos1 + 1);
	return FuncRealName;
}

void SubstitueNonAlpha2ByUnderline(std::string& str)
{
	int i;
	int length = str.size();
	for (i = 0;i < length;i++)
	{
		char* p = (char*)str.data();
		if (!isAlphaorNum(p[i]))
			p[i] = '_';
	}
}
bool isAlphaorNum(char C)
{
	return (C >= 'a'&&C <= 'z') || (C >= 'A'&&C <= 'Z') || (C >= '0'&&C <= '9');
}

int IfStringInlist(string ins, vector<string> SearchIns)
{
	int flag = 0;
	for (int i = 0; i < SearchIns.size(); i++)
	{
		if (ins == SearchIns[i])
		{
			flag = 1;
			break;
		}
	}
	return flag;
}


int IfFuncInlist(Function* ins, vector<Function*> SearchIns)
{
	int flag = 0;
	for (int i = 0; i < SearchIns.size(); i++)
	{
		if (ins == SearchIns[i])
		{
			flag = 1;
			break;
		}
	}
	return flag;
}


int IfBlockInlist(BasicBlock* ins, vector<BasicBlock*> SearchIns)
{
	int flag = 0;
	for (int i = 0; i < SearchIns.size(); i++)
	{
		if (ins == SearchIns[i])
		{
			flag = 1;
			break;
		}
	}
	return flag;
}

