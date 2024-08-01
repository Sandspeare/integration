import numpy as np
import pandas as pd
import pickle
from  pathlib import Path
from bindiff_run_single_process import OUTPUT_DIR


def res2excel(res_path,excel_path):
    with open(res_path,"rb")as f:
        res=pickle.load(f)

    #the  result format is list [,,]
    file_name_list,sim_arr,confidence,normal_function_sim=res

    #create a writer instance
    with pd.ExcelWriter(excel_path,engine='xlsxwriter') as writer:
        #write the sim_
        df_sim=pd.DataFrame(sim_arr)
        df_conf=pd.DataFrame(confidence)
        df_file_name=pd.DataFrame(file_name_list,columns=["filename"])
        df_normal_function_sim=pd.DataFrame(normal_function_sim)
        df_sim.to_excel(writer,sheet_name="similary",index=False)
        df_conf.to_excel(writer,sheet_name="confidence",index=False)
        df_file_name.to_excel(writer,sheet_name="file_name",index=False)
        df_normal_function_sim.to_excel(writer,sheet_name="normal_func_sim",index=False)

if __name__=="__main__":
    res_path=r"E:\MyProjectTest\BinSimiProject\bindiffTest_image\outputDir\0"
    #res_path=r"E:\AppTemp\outputDir\0"
    excel_path=Path(__file__).parent/Path(OUTPUT_DIR)/"".join([Path(res_path).name,".xlsx"])
    res2excel(res_path,excel_path)