import argparse
import sys
import sqlite3
import pandas as pd
from pathlib import Path

#connect the database
#query the result filter the sub_
def filter_bindiff_res(idb3_path:str)->list:
    '''
        #the result format is ref the bindiff result function table struct
        # id add1 func_name1 add2 func_name2 sim conf flags algrithm ....
    '''
    #connect to idb3
    conn=sqlite3.connect(idb3_path)
    cursor=conn.cursor()
    sql1='SELECT *  FROM function WHERE (name1 LIKE "sub%" OR name2 LIKE "sub%" )AND instructions > 30;'
    sql2="SELECT filename FROM file;"
    sql3="SELECT *  FROM file;"
    try:
        cursor.execute(sql1)
        result=cursor.fetchall()
        #for row in result:
            #print(row)

        #get the file name 
        cursor.execute(sql2)
        files_name=cursor.fetchall()

        #get the normal function num from bindiff result
        cursor.execute(sql3)
        func_normal_num=cursor.fetchall()

        ...
    except sqlite3.Error as err:
        print(err)
    finally:
        if conn:
            cursor.close()
            conn.close()

    return result,[i[0] for i in files_name],[i[4] for i in func_normal_num] #the result is function table data, [] is the file names

def res_sqlite_proc(result:list,func_normal_num)->any:
    '''
    caculate the normal similary
    the algorithm is E(sim * conf)
    return sim_mean
    '''
    sim_mean=0
    len_=sum(func_normal_num)/2
    for item in result:
        #item[5] is the sim,item[6] is confidence ,the alogrithm will use sim*conf 
        sim_mean+=item[5]*item[6]

    
    assert len_!=0,"the result is null"
    sim_mean/=len_
    return sim_mean

def ret2excel(*argv,xlsx_path=None)->bool:
    '''
    argv is a list that contain  (sim_mean , normal func item)
    argv[0] is [file1_name,file2_name,sim_mean]
    argv[1] is res [[],[]] from bindiff
    xlsx is the save xlsx file path
    '''
    #create a writer instance
    with pd.ExcelWriter(xlsx_path,engine='xlsxwriter') as writer:
        for id, arg  in enumerate(argv):  
            #write the sim_
            df_=pd.DataFrame(arg)
            df_.to_excel(writer,sheet_name=str(id),index=False)


def caculate_normal_sim_api(bindiff_path:str):
    '''
    discription: calculate the similary of the normal function in the tow binary 
    arg: bindiff database
    return: sim_mean
    '''
    res,_,func_normal_num=filter_bindiff_res(bindiff_path)
    sim_mean=res_sqlite_proc(res,func_normal_num)
    return sim_mean

if __name__=="__main__":
    arg_parse_class=argparse.ArgumentParser(description="specify the result file")
    arg_parse_class.add_argument("-f","--res_file",
                                 dest="res_path",
                                 default=r"E:\MyProjectTest\BinSimiProject\bindiffTest_image\outputDir\CpiWmBRP.exe_vs_ItmwSYfJ.exe.BinDiff",
                                 help="the bindiff result file")
    arg_parse_class.add_argument("-s","--save-path",dest='excel_save_path',
                                 default=str(Path(__file__).parent/Path("./outputDir")),
                                 help="the save excel path")
    argv=arg_parse_class.parse_args(sys.argv[1:])
    bindiff_data_path=argv.res_path
    summary_xlsx_dir=argv.excel_save_path
    res,files_name,func_norm_num=filter_bindiff_res(bindiff_data_path)
    sim_mean=res_sqlite_proc(res,func_norm_num)
    summary=[files_name[0],files_name[1],sim_mean]
    xlsx_path=Path(summary_xlsx_dir)/"".join([summary[0],"_vs_",summary[1],".xlsx"])
    ret2excel(summary,res,xlsx_path=xlsx_path)
    ...
    
    
    ...