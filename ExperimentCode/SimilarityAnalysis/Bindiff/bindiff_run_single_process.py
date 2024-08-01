import sys,os
import platform
import argparse
import subprocess
from pathlib import Path
import sqlite3
import json
from res_filter_lib import caculate_normal_sim_api

cwd_current=Path(__file__).parent

DEBUG=True #the DEBUG flag used for print the phase ,if don't want to print the message ,set it as Flase

#before run the script,config the envirenment varriable
with open (cwd_current/"config.json","r") as f:
    CONFIG:dict=json.load(f)

#define some constant
OUTPUT_DIR:str=str(cwd_current/"output")
TEMP_DIR:str=str(cwd_current/"temp")
IDA_PATH:str=CONFIG["IDA_PATH"]
BINDIFF_PATH:str=CONFIG["BINDIFF_PATH"]

def log_print(msg:str):
    '''
    print the msg base on the DEBUG
    '''
    if DEBUG:
        print(msg)

#read the result file of the bindiff application 
def _read_bindiff_res(bindifffilepath:str,md5file1:str="",md5file2:str="")->int :
    conn_sqlite=sqlite3.connect(bindifffilepath)
    mycursor_s=conn_sqlite.cursor()
    sql="select similarity,confidence from metadata;"
    try:
        mycursor_s.execute(sql)
        result=mycursor_s.fetchall()
        filesimi=result[0][0]
        fileconf=result[0][1]
    except sqlite3.Error as error:
        print("Error while select file similarity",error)
    finally:
        if conn_sqlite:
            mycursor_s.close()
            conn_sqlite.close()
    return filesimi,fileconf

def name_gen_binexport(file_path:str)->str:
    '''
    generate the binexport file name base on the file_path
    '''
    return "".join([str(Path(TEMP_DIR)/Path(file_path).name),".BinExport"])

def name_bindiff_res(file_path0:str,file_path1:str)->str:
    '''
    the function return the bindiff result file name base on file_path0 and file_path1.
    for example :foo.out_vs_liba.so.BinDiff
    '''
    return str(Path(OUTPUT_DIR)/ "".join([Path(file_path0).name,"_vs_",Path(file_path1).name,".BinDiff"]))

def ida_BinExport(file_path:str)->None:
    '''
    run ida to make .binexport file
    '''

    file_path=Path(file_path)
    
    if not file_path.exists():
        raise FileNotFoundError(f"The file '{file_path}' does not exist.")

    bin_export_filename=name_gen_binexport(str(file_path))
    cmd=" ".join([IDA_PATH,"-A", f"-OBinExportModule:{bin_export_filename}","-OBinExportAutoAction:BinExportBinary ",file_path.__str__()])
    process=subprocess.run(cmd,shell=True,capture_output=True)
    if process.returncode:
        log_print (f"make {file_path.name}.binExport file fail ")
    else:
        log_print(f"ida create {file_path.name}.binexport file successful")

def bindiff_run(binexportfile1:str,binexportfile2:str)->int:
    '''
    run bindiff, the output filename is bindiff auto generate
    
    '''
    cmd=" ".join([BINDIFF_PATH, "--logo=false",f" --primary={binexportfile1}",f"--secondary={binexportfile2}",f" --output_dir={str(Path(OUTPUT_DIR).resolve())}","--output_format=bin"])

    system_name = platform.system()

    if system_name == "Windows":
        dev_null='nul'
    elif system_name == "Linux":
        dev_null="/dev/null"
    else:
        raise Exception(f"the system is nodefine system: {system_name}")
    cmd=" ".join([cmd,">",dev_null," 2>&1"])
    process=subprocess.run(cmd,shell=True)
    return process.returncode

#generate the bindiff result file 
def bindiff_run(file1:str,file2:str):
    """
    the function generate the bindiff result file ,which is a sqlite database
    args: file1 file2 is two binary file path
    """
    file1_class,file2_class=Path(file1),Path(file2)
    if not file1_class.exists() or not file2_class.exists():
        log_print("the binary file do not exist")
        raise FileNotFoundError(f"The file '{file1}' or {file2} does not exist.")
        
    bin_export_filename1=name_gen_binexport(str(file1_class))
    bin_export_filename2=name_gen_binexport(str(file2_class))
    assert not bindiff_run(bin_export_filename1,bin_export_filename2)," run bindiff error"
    log_print("bindiff analysis successful")

def bindiff_res_read(res_bindiff_path:str)->any:
    '''
    read the result file of bindiff
    '''
    sim_file,confidence=_read_bindiff_res(bindifffilepath=res_bindiff_path)
    return sim_file,confidence

def bindiff_res_read(file0:str,file1:str)->any:
    '''
    return tow object: similary,confidence
    '''
    res_bindiff_path=name_bindiff_res(file0,file1)
    return _read_bindiff_res(res_bindiff_path)

def gen_normal_func_result(file1:str,file2:str):
    """
    the function caculate the normal function similary,from a sqlite database
    args: file1 file2 is two binary file
    """
    file0_class,file1_class=Path(file1),Path(file2)
    if not file0_class.exists() or not file1_class.exists():
        print ("the file do not exist")
        return 1

    #assert not run_bindiff(bin_export_filename1,bin_export_filename2)," run bindiff error"
    
    #read the result file of bindiff.exe
    
    bindiff_res_path=name_bindiff_res(file0_class,file1_class)
    
    sim_mean=caculate_normal_sim_api(bindiff_path=bindiff_res_path)
    return sim_mean


def del_binexport(file_path:str)->any:
    binexport_path=name_gen_binexport(file_path)
    try:
        os.remove(binexport_path)
        print(f"File '{binexport_path}' has been deleted.")
    except FileNotFoundError:
        print(f"Error: File '{binexport_path}' does not exist.")
    except PermissionError:
        print(f"Error: Permission denied to delete '{binexport_path}'.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def del_bindiff_res(file0:str,file1:str)->any:
    res_bindiff_path=name_bindiff_res(file0,file1)
    try:
        os.remove(res_bindiff_path)
        print(f"File '{res_bindiff_path}' has been deleted.")
    except FileNotFoundError:
        print(f"Error: File '{res_bindiff_path}' does not exist.")
    except PermissionError:
        print(f"Error: Permission denied to delete '{res_bindiff_path}'.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")

def del_bindiff_res(res_bindiff_path:str)->any:
    try:
        os.remove(res_bindiff_path)
        print(f"File '{res_bindiff_path}' has been deleted.")
    except FileNotFoundError:
        print(f"Error: File '{res_bindiff_path}' does not exist.")
    except PermissionError:
        print(f"Error: Permission denied to delete '{res_bindiff_path}'.")
    except Exception as e:
        print(f"An unexpected error occurred: {e}")


if __name__=="__main__":
    my_argparse=argparse.ArgumentParser("the bindiff test script need")
    my_argparse.add_argument("--first",dest="binary0",
                             default=r"E:\my_code\bindiffTest-v2.0\input\foo.out",help="binary path0")
    my_argparse.add_argument("--second",dest="binary1",
                             default=r"E:\my_code\bindiffTest-v2.0\input\liba.so",help="binary path1")
    my_args=my_argparse.parse_args(sys.argv[1:])
    binary0:str=my_args.binary0
    binary1:str=my_args.binary1

    ida_BinExport(binary0)
    ida_BinExport(binary1)
    
    bindiff_run(binary0,binary1)
    sim,confidence=bindiff_res_read(binary0,binary1)



    del_binexport(binary0)
    del_binexport(binary1)
    del_bindiff_res(binary0,binary1)
    ...