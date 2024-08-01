#the windows and linux pythonpath load order is not same. window from end to start ,but linux from start to end
from scripts_ import bin2asm
import argparse
from pathlib import Path
import os
import shutil
import torch
import asm2vec
import numpy as np
import statistics
def clean_folder(bin_folder)->int:
    if os.path.exists(bin_folder):
        for file_path in os.listdir(bin_folder):
            try:
               if os.path.isfile(bin_folder/file_path):
                    os.remove(bin_folder/file_path)
               elif os.path.isdir(bin_folder/file_path):
                   shutil.rmtree(bin_folder/file_path)
            except Exception as error:
                print(error)
                return 1
        pass
    else:
        try:
            os.mkdir(bin_folder)
        except OSError as error:
            print(error)
            return 1
    return 0
def gen_vector_func(bin1,bin2,ipath1, ipath2, mpath, epochs, device, lr):
    if device == 'auto':
        device = 'cuda' if torch.cuda.is_available() else 'cpu'

    # load model, tokens
    model, tokens = asm2vec.utils.load_model(mpath, device=device)
    functions, tokens_new = asm2vec.utils.load_data([ipath1, ipath2])
    
    tokens.update(tokens_new)
    model.update(function_size_new=functions.__len__(), vocab_size_new=tokens.size())
    model = model.to(device)
    
    # train function embedding
    model = asm2vec.utils.train(
        functions,
        tokens,
        model=model,
        epochs=epochs,
        device=device,
        mode='test',
        callback=None,
        learning_rate=lr
    )

    # compare X function vectors
    vector_func:torch.tensor = model.embeddings_f(torch.arange(len(functions),device=device))
    #[(func_meta,func_embedd),]
    bin1_func_embedd,bin2_func_embedd=[],[]
    for i ,function in enumerate(functions):
        if function.meta["file"] ==bin1.name:
            bin1_func_embedd.append([function.meta,vector_func[i,:]])
        elif function.meta["file"]==bin2.name:
            bin2_func_embedd.append([function.meta,vector_func[i,:]])
        else:
            print("find an except function")
            pass
    return bin1_func_embedd,bin2_func_embedd

def cosine_similarity(v1, v2):
    return (v1 @ v2 / (v1.norm() * v2.norm())).item()
    #print(f'cosine similarity : {cosine_similarity(v1, v2):.6f}')
def cosine_simi_matrix(matrix1:torch.tensor,matrix2:torch.tensor):
    matrix1_norm=matrix1/matrix1.norm(p=2,dim=1,keepdim=True)
    matrix2_norm=matrix2/matrix2.norm(p=2,dim=1,keepdim=True)
    #matrix1,matrix2=[x/x.norm(p=2,dim=1,keepdim=True) for x in [matrix1,matrix2]]
    bin1vsbin2_embed=torch.matmul(matrix1_norm,matrix2_norm.T)
    return bin1vsbin2_embed
def caculate_sim(matrix:np.ndarray)->float:
    '''
    caculate the matrix max value ,under the condition of axis=1
    return sim (float)
    '''
    res_list=list()
    for i in range(matrix.shape[0]):
        max_val=np.max(matrix,axis=1)
        max_idx=np.argmax(matrix,axis=1)
        max_val2=np.max(max_val,axis=0)
        max_idx_row=np.argmax(max_val,axis=0)
        max_idx_column=max_idx[max_idx_row]
        res_list.append( (max_val2,max_idx_row,max_idx_column))
        matrix[max_idx_row,:]=np.zeros((matrix.shape[1],))
        matrix[:,max_idx_column]=np.zeros((matrix.shape[0],))
        pass
    return res_list
def caculate_bin_similarity(bin1_embedding:torch.tensor,bin2_embedding:torch.tensor)->any:
    #caculate the cos_sim matrix
    bin1Vsbin2_cosinedistance=cosine_simi_matrix(bin1_embedding,bin2_embedding)
    
    #calute the binary similarity
    bin1Vsbin2_cosinedistance:torch.tensor(device="cuda")= bin1Vsbin2_cosinedistance.to("cpu")
    cosin_matrix:np.ndarray=bin1Vsbin2_cosinedistance.detach().numpy()
    cosin_matrix_copy=np.copy(cosin_matrix)
    cosin_matrix_T=np.transpose(cosin_matrix_copy)
    res_list_a=caculate_sim(cosin_matrix)
    sim_a=sum([i[0] for i in res_list_a])/len(res_list_a)
    res_list_b=caculate_sim(cosin_matrix_T)
    sim_b=sum([i[0] for i in res_list_b])/len(res_list_b)
    sim_=statistics.mean((sim_a,sim_b))
    #cosine distance is from -1 to +1 ,so I map it
    sim_=(sim_+1)/2
    return sim_

if __name__=="__main__":
    parse=argparse.ArgumentParser(description="asm2vec model arguments")
    parse.add_argument("--bin1",dest="bin1",type=str,required=True,help="the first binary")
    parse.add_argument("--bin2",dest="bin2",type=str,required=True,help="the seconde binary")
    parse.add_argument("--output_dir",dest="output_dir",default="./output",help="the temp data to store")
    #define the hy parameters
    model_path=Path(r"./output/model_dir/model_train_test.pt")
    epochs=50
    device=torch.device("cuda" if torch.cuda.is_available() else "cpu")
    lr=0.02
    
    args=parse.parse_args()
    output_dir=Path(args.output_dir)
    bin1=Path(args.bin1)
    bin2=Path(args.bin2)
    
    #if the dir is not exits ,create it 
    bin1_folder=output_dir/"bin1"
    bin2_folder=output_dir/"bin2"
    clean_folder(bin1_folder)
    clean_folder(bin2_folder)
    
    if os.path.isfile(bin1) and os.path.isfile(bin2):
        ##bin2asm.bin2asm(bin1,bin1_folder)
        list(map(bin2asm.bin2asm,[bin1,bin2],[bin1_folder,bin2_folder],[4,4]))
        
    #[(func_meta,func_embedd),]
    bin1_func_embed,bin2_func_embed=gen_vector_func(bin1,bin2,bin1_folder,bin2_folder,model_path,epochs,device,lr)
    bin1_embedding,bin2_embedding=None,None
    for i  in bin1_func_embed:
        if bin1_embedding is not None:
            bin1_embedding=torch.cat([bin1_embedding,i[1].unsqueeze(0)],dim=0)
        else:
            bin1_embedding=i[1].unsqueeze(0)
    for i in bin2_func_embed:
        if bin2_embedding is not None:
            bin2_embedding=torch.cat([bin2_embedding,i[1].unsqueeze(0)],dim=0)
        else:
            bin2_embedding=i[1].unsqueeze(0)
    simi:torch.tensor=caculate_bin_similarity(bin1_embedding,bin2_embedding)
    
    print("*"*20)
    #print("first file is :{}".format(bin1_abspath))
    #print("seconde file is :{}".format(bin2_abspath))
    
    print("the similary of files :{:.6f}".format(simi))
    
    pass
