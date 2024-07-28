#pragma once

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InlineAsm.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Obfuscation/Hello.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/CallSite.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/GlobalValue.h" 
#include "llvm/Transforms/Scalar.h"
#include "llvm/Support/Allocator.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Support/BranchProbability.h"
#include "llvm/Analysis/BranchProbabilityInfo.h"
#include "llvm/CodeGen/MachineBranchProbabilityInfo.h"
#include "llvm/Support/BlockFrequency.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"
#include "llvm/Analysis//BlockFrequencyInfoImpl.h"
#include "llvm/CodeGen/MachineBlockFrequencyInfo.h"
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/IR/ConstantRange.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/SystemUtils.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/ToolOutputFile.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Linker/Linker.h"
#include "cJSON.h"
#include <windows.h>
#include <memory>
#include <vector>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <wincrypt.h>
using namespace std;
using namespace llvm;

Module*  MyParseIRFile(StringRef filename);
bool doWriteBackLL(Module* M, StringRef filename);
bool doWriteBackBC(Module* M, StringRef filename);
std::string RetrieveFuncName(Function *F);
void SubstitueNonAlpha2ByUnderline(std::string& str);
bool isAlphaorNum(char C);
int IfBlockInlist(BasicBlock* ins, vector<BasicBlock*> SearchIns);
int IfFuncInlist(Function* ins, vector<Function*> SearchIns);
int IfStringInlist(string ins, vector<string> SearchIns);
void TransConstant2Inst(Function *Func);
struct BrStructInfo
{
	BOOL IsBranch;
	BasicBlock* entry;		//Fused branch entry
	BasicBlock* end;		//Fused branch end
	BasicBlock* back;		//Fused branch back
	vector<BasicBlock* > BranchBlocks;
};

struct PointInfo
{
	Instruction* point;
	int value;
	int number;
	BOOL IsUNK;

};