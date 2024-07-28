/*****************************************************************************
Filename: ExtractFusePoints.cpp
Date    : 2024-07-19 16:21:15
Description: Analyze json file to get points can be integration
*****************************************************************************/

#include "AnalysisBranch.h"
#include "InterFuncShellize.h"
#include "ExtractFusePoints.h"

#include <fstream>

Instruction* ExtractFusePoints::GetKeyPointByJson(Module *mod, string PointName, int value)
{
	int pos = PointName.find_last_of('----');
	string tempName2 = PointName.substr(0, pos - 3);
	int Location = atoi(PointName.substr(pos + 1).c_str());

	pos = tempName2.find_last_of('----');
	string FuncName = tempName2.substr(0, pos - 3);
	string BlockName = tempName2.substr(pos + 1);

	Function* F = mod->getFunction(FuncName);
	Instruction* ins = NULL;
	for (BasicBlock &B : *F)
	{
		if (B.getName() == BlockName)
		{
			int index = 1;
			if (Location == 0)
			{
				ins = &*B.getFirstInsertionPt();
				break;
			}
			for (Instruction &I : B)
			{
				if (index == Location)
				{
					ins = &I;
				}
				index++;
			}
		}
	}

	PointInfo tempPointInfo;
	tempPointInfo.point = ins;
	tempPointInfo.value = value;

	KeyInfoList.push_back(tempPointInfo);

	return ins;

}


Instruction* ExtractFusePoints::GetFusedPointByJson(Module *mod, string PointName, int value, string valuestring)
{

	int pos = PointName.find_last_of('----');
	string tempName2 = PointName.substr(0, pos - 3);
	int Location = atoi(PointName.substr(pos + 1).c_str());

	pos = tempName2.find_last_of('----');
	string FuncName = tempName2.substr(0, pos - 3);
	string BlockName = tempName2.substr(pos + 1);

	Function* F = mod->getFunction(FuncName);
	Instruction* ins = NULL;
	for (BasicBlock &B : *F)
	{
		if (B.getName() == BlockName)
		{
			int index = 1;
			if (Location == 0)
			{
				ins = &*B.getFirstInsertionPt();
			}
			for (Instruction &I : B)
			{
				if (index == Location)
				{
					ins = &I;
				}
				index++;
			}
		}
	}
	if (ins == NULL)
		return ins;
	if (ins->getParent()->getInstList().size() < 3)
		return NULL;
	if (PHINode* Ii = dyn_cast<PHINode>(ins))
		return NULL;

	PointInfo tempPointInfo;
	tempPointInfo.point = ins;

	if (valuestring != "UNK")
	{
		tempPointInfo.IsUNK = FALSE;
		tempPointInfo.value = value;
	}
	else
	{
		tempPointInfo.IsUNK = TRUE;
	}

	PointInfoList.push_back(tempPointInfo);
	return ins;
}


void ExtractFusePoints::ParseJsonFile(Module* mod, string ptFile)
{
	FILE* fp = NULL;
	char buf[1000000] = { 0 };
	fopen_s(&fp, ptFile.c_str(), "r");
	if (fp)
	{
		fread(buf, 1, 1000000, fp);
	}
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
			if (strcmp(item_json->string, "Trigger") == 0)
			{
				cJSON * Trigger = item_json->child;
				while (Trigger != NULL)	
				{
					Instruction *ins = GetKeyPointByJson(mod, Trigger->string, Trigger->valueint);

					cJSON*  contentArrayItem = cJSON_GetObjectItem(Trigger, Trigger->string);
					Trigger = Trigger->next;

				}
			}
			else
			{
				Instruction *ins = NULL;
				if (item_json->valuestring != NULL)
				{
					ins = GetFusedPointByJson(mod, string(item_json->string), 0, "UNK");
				}
				else
				{
					ins = GetFusedPointByJson(mod, string(item_json->string), item_json->valueint, "NK");
				}
				
				
			}
			item_json = item_json->next;

		}
	}

	cJSON_Delete(root);
	fclose(fp);
}

void ExtractFusePoints::MatrixInitial(Module* mod, string ptFile)
{
	
	ParseJsonFile(mod, ptFile);
	return;
}
