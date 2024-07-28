/*****************************************************************************
Filename: main.cpp
Date    : 2024-07-10 10:59:01
Description: Execution code integration between matrix code and source code.
*****************************************************************************/

#include "IRGeneralOperation.h"
#include "AnalysisBranch.h"
#include "InterFuncShellize.h"
#include "ExtractFusePoints.h"
#include "FuseMatrixAndFunc.h"
#include "InlineFunctions.h"

int main(int argc, char**argv)
{
	string ModuleName = argv[1];
	string output = argv[2];	
	string FuncBCName = argv[3];
	string FusePoints = argv[4];
	string FileType = argv[5];   
	string PaddingFile = argv[6];   


	Module* matrix_mod = MyParseIRFile(ModuleName);

	// \Inline all functions into only one function
	InlineFunctions doInlineFunctions;
	Function* fused_func = doInlineFunctions.InlineAllFunc(FuncBCName);
	Module* mod = matrix_mod;

	// \Get the final inlined function
	fused_func = mod->getFunction(fused_func->getName());			
	AnalysisBranch doAnalysisBranch;
	doAnalysisBranch.ptFile = FusePoints;

	// \Find the blocks suitable for integration and split based on the control flow
	if (doAnalysisBranch.SplitFunctionBranch(fused_func) == 0)
	{
		errs() << "[ERROR]: Fused points is too small" << "\n";
		return 0;
	}		

	// \Rebase global variable
	InterFuncShellize doInterFuncShellize;
	doInterFuncShellize.BrStructs = doAnalysisBranch.BrStructs;
	doInterFuncShellize.GenFuncShellize(fused_func);

	// \Find the points in matrix that suitable for integration
	ExtractFusePoints doExtractFusePoints;
	doExtractFusePoints.MatrixInitial(mod, FusePoints);	


	// \Integrate source code and matrix code.
	FuseMatrixAndFunc doFuseMatrixAndFunc;
	doFuseMatrixAndFunc.KeyInfoList = doExtractFusePoints.KeyInfoList;
	doFuseMatrixAndFunc.PointInfoList = doExtractFusePoints.PointInfoList;
	doFuseMatrixAndFunc.BrStructs = doAnalysisBranch.BrStructs;
	doFuseMatrixAndFunc.Fuse();

	doWriteBackLL(mod, output);
	mod = MyParseIRFile(output);

	return 0;

}