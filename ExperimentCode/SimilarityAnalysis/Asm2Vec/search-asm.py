import pickle
import torch
import numpy
import asm2vec
import sys
import os
from pathlib import Path
from typing import List
import tqdm

from asm2vec.datatype import *

DATA_HOME = 'd:/gu/dataset/'

device = 'cuda' if torch.cuda.is_available() else 'cpu'

#mpath='H:/src/phd/dataset/asm2vec/output/model_all_ep31.pt'

def find_all_files(path):
    filenames = []
    if os.path.isdir(path):
        for filename in sorted(os.listdir(path)):
            if os.path.isfile(Path(path) / filename):
                filenames.append(Path(path) / filename)
            else:
                filenames += find_all_files(Path(path) / filename)
    else:
        filenames.append(path)
    return filenames
    
def cosine_similarity(v1, v2):
    return (v1 @ v2 / (v1.norm() * v2.norm())).item()

    
def calc_sim(f1:Function, f2:Function):
    #functions, tokens_new = asm2vec.utils.load_data([ipath1, ipath2])
    global model
    global tokens

    functions = [f1, f2]
    tokens_new = Tokens()
    tokens_new.add(f1.tokens())
    tokens_new.add(f2.tokens())
    tokens.update(tokens_new)
    model.update(2, tokens.size())
    model = model.to(device)
    
    # train function embedding
    model = asm2vec.utils.train(
        functions,
        tokens,
        model=model,
        epochs=10,
        device=device,
        mode='test',
        learning_rate=0.02
    )

    # compare 2 function vectors
    v1, v2 = model.to('cpu').embeddings_f(torch.tensor([0, 1]))
    return cosine_similarity(v1, v2)

# calc top-K
# search funcs1 in funcs2
def calc_topk(embs1, embs2):
    similars = []
    sim_sorted = []
    for emb1 in embs1:     
        sub_similars = []   
        sub_sim_sorted = []
        for emb2 in embs2:
            similar = cosine_similarity(emb1, emb2)
            sub_similars.append(similar)
        sub_sim_sorted = numpy.argsort(sub_similars)

        similars.append(sub_similars)
        sim_sorted.append(sub_sim_sorted)

    return similars, sim_sorted

def calc_topk_funcs(funcs1, funcs2):
    similars = []
    sim_sorted = []
    for f1 in tqdm.tqdm(funcs1):     
        sub_similars = []   
        sub_sim_sorted = []
        for f2 in funcs2:
            similar_main = cosine_similarity(f1.embedding, f2.embedding)
            sub_similars.append(similar_main)

            # if similar_main < 0.5:
            #     sub_similars.append(max(0, min(1, similar_main)))
            #     continue
            # 
            # similar_miner = [0]
            # for c1 in f1.calls:
            #     s = [0]
            #     for c2 in f2.calls:
            #         s.append(cosine_similarity(c1.embedding, c2.embedding))
            #     similar_miner.append((max(s) - 0.5) * 0.5)                
            
            #similar_main += numpy.max(similar_miner)
            #similar_main += numpy.min(similar_miner)
            #sub_similars.append(max(0, min(1, similar_main)))

        sub_sim_sorted = numpy.argsort(sub_similars)
        similars.append(sub_similars)
        sim_sorted.append(sub_sim_sorted)

    return similars, sim_sorted

def calc_topk_pretrain(funcs1, funcs2, k):
    hit_count_top = []
    similars = []
    differs = []
    sim_sorted = []
    for i in tqdm.tqdm(range(len(funcs1)), 'calc top-{}'.format(k)):  
        sub_similars = []   
        for f2 in funcs2:
            similar = cosine_similarity(funcs1[i].embedding, f2.embedding)
            sub_similars.append(similar)
        sub_sim_sorted = numpy.argsort(sub_similars)

        similars.append(sub_similars)
        sim_sorted.append(sub_sim_sorted)

        sub_hit_count_top = [0] * (k + 1)
        for j in range(1, min(len(sub_sim_sorted) + 1, k + 1)):
            id = sub_sim_sorted[-j]

            names2 = funcs2[id].meta['name'].split('.')
            names1 = funcs1[i].meta['name'].split('.')
            name1 = names1[-1]
            name2 = names2[-1]
            if name1 == name2:
                if sub_similars[id] > 0.9999: # if the similarity equal to 1, then we find it @1
                    sub_hit_count_top[1] += 1
                else:
                    sub_hit_count_top[j] += 1
                break
        hit_count_top.append(sub_hit_count_top)

    return hit_count_top, sim_sorted, similars, differs

def load_functions(func_file) -> List[Function]:
    #print("Load functions from {}".format(func_file))
    functions:List[Function] = []
    with open(func_file, "rb") as f:
        functions = pickle.load(f)

    return functions

def create_functions(filenames, func_file = None) -> List[Function]:
    # prepare data    
    functions:List[Function] = []

    if func_file and os.path.exists(func_file):
        print("File already exist: {}".format(func_file))
        return load_functions(func_file)
    
    if not filenames:
        print('need asm files to load')
        return functions

    #print("Loading raw asm files")
    for filename in filenames:#tqdm.tqdm(filenames):
        with open(filename) as f:
            fn = Function.load(f.read())
            functions.append(fn)

    if func_file:
        #print("Dump functions to {}".format(func_file))
        try:
            with open(func_file, "wb") as f:            
                pickle.dump(functions, f)
        except Exception as e:
            #print('Failed')
            os.remove(func_file)
            pass

    return functions

def ext_functions(funcs:List[Function]):
    for i in range(len(funcs)):
        for addr in funcs[i].calls_addrs:
            if addr.startswith('0x'):
                addr = addr.split('0x')[-1]
            for j in range(len(funcs)):
                if funcs[j].meta['offset'].strip().endswith(addr):
                    funcs[i].calls.append(funcs[j])
                    break

# funcs: functions
# outfile: embeddings pickle file
def gen_embeddings(funcs, outfile = None):
    funcs_emb = []
    if outfile and os.path.lexists(outfile):
        with open(outfile, "rb") as f:
            funcs_emb = pickle.load(f)

        # set embedding
        for i in range(len(funcs)):
            funcs[i].embedding = funcs_emb[i]
    else:
        global model
        global tokens

        tokens_new = Tokens()
        for f in funcs:
            tokens_new.add(f.tokens())
        tokens.update(tokens_new)
        model.update(len(funcs), tokens.size())
        model = model.to(device)
        
        # train function embedding
        model = asm2vec.utils.train(
            funcs,
            tokens,
            embedding_size=100,
            model=model,
            epochs=10,
            device=device,
            mode='test',
            learning_rate=0.02
        )

        # compare 2 function vectors
        funcs_emb = model.to('cpu').embeddings_f(torch.tensor(range(len(funcs))))
        
        # set embedding
        for i in range(len(funcs)):
            funcs[i].embedding = funcs_emb[i]

        if outfile:
            with open(outfile, "wb") as f:
                pickle.dump(funcs_emb, f)
    return funcs_emb

select = -1
if len(sys.argv) <= 2:
    print("usage: search.py xxx.pt")

n = 1

mpath = 'd:/gu/output/test.pt'
#if len(sys.argv) > n:
#    mpath = sys.argv[n]
#    n+=1

targets1 = ['libpcap']
if len(sys.argv) > n:
    targets1 = [sys.argv[n]]
    n+=1
compilers1 = ['gcc-o3']
if len(sys.argv) > n:
    compilers1 = [sys.argv[n]]
    n+=1
targets2 = targets1
if len(sys.argv) > n:
    targets2 = [sys.argv[n]]
    n+=1
compilers2 = ['gcc-o3']
if len(sys.argv) > n:
    compilers2 = [sys.argv[n]]
    n+=1

model, tokens = asm2vec.utils.load_model(mpath, device=device)

search_funcname1 = ''
if len(sys.argv) > n:
    search_funcname1 = sys.argv[n]
    n+=1
search_funcname2 = ''
if len(sys.argv) > n:
    search_funcname2 = sys.argv[n]
    n+=1

#asm_file = DATA_HOME + 'test.asm'
asm_file = ''
topk = 5
min_fsize = 0
max_fsize = 100000

if asm_file != '': 
    funcs1_target = ['test']
    funcs1_comp = ['test']
    with open(asm_file) as f:
        funcs1 = [Function.load(f.read())]

    ext_functions(funcs1)
    gen_embeddings(funcs1)


#print("Load src functions ...")
# input
funcs1 = []
for t in targets1:
    for c in compilers1:
        func_file = DATA_HOME + 'train-asm/out-{}-{}.pkl'.format(t, c)
        asm_files = find_all_files(DATA_HOME + 'train-asm/{}/{}/'.format(t, c))
        funcs = create_functions(asm_files, func_file)
        #check_vocab(VOCAB_FILE, funcs)
        for f in funcs:
            f.target = t
            f.compiler = c
        funcs1 += funcs
#print("Total: {}".format(len(funcs1)))

#print("Load dst functions ...")
funcs2 = []
for t in targets2:
    for c in compilers2:
        func_file = DATA_HOME + 'train-asm/out-{}-{}.pkl'.format(t, c)
        asm_files = find_all_files(DATA_HOME + 'train-asm/{}/{}/'.format(t, c))
        funcs = create_functions(asm_files, func_file)
        #check_vocab(VOCAB_FILE, funcs)
        for f in funcs:
            f.target = t
            f.compiler = c
        funcs2 += funcs
#print("Total: {}".format(len(funcs2)))

# filter functions with same name in Targets2
funcs1_sim = funcs1
funcs2_sim = funcs2
if search_funcname1 != '':
    funcs1_sim = []
    for i in range(len(funcs1)):
        if funcs1[i].meta['name'].endswith(search_funcname1):
            funcs1_sim.append(funcs1[i])    
if search_funcname2 != '':
    funcs2_sim = []
    for i in range(len(funcs2)):
        if funcs2[i].meta['name'].endswith(search_funcname2):
            funcs2_sim.append(funcs2[i])     
if search_funcname1 == '' and search_funcname2 == '':    
    funcs1_set = set()
    funcs2_set = set()
    for i in tqdm.tqdm(range(len(funcs1)), desc='Find similar'):
        # filter functions with instructions > max_fsize
        if len(funcs1[i].insts) > max_fsize or len(funcs1[i].insts) < min_fsize:
            continue
        for j in range(len(funcs2)):        
            if len(funcs2[j].insts) > max_fsize or len(funcs1[i].insts) < min_fsize:
                continue
            n1 = funcs1[i].meta['name'].split('.')[-1]
            n2 = funcs2[j].meta['name'].split('.')[-1]
            if n1 == n2:
                funcs1_set.add(funcs1[i])  
                funcs2_set.add(funcs2[j])
                break
    funcs1_sim = list(funcs1_set)
    funcs2_sim = list(funcs2_set)                         
if len(funcs1_sim) == 0 or len(funcs2_sim) == 0:
    raise('not found')
else:
    print("Select for compare: {} vs {}".format(len(funcs1_sim), len(funcs2_sim)))

#print('generate embedding ...')
gen_embeddings(funcs1_sim)
gen_embeddings(funcs2_sim)

#print('calculate similarity ...')
# calc similarities
#similars, sim_sorted = calc_topk_funcs(funcs1_sim, funcs2_sim)
#similars, sim_sorted = calc_topk(funcs1_emb, funcs2_emb)

hit_count_top, sim_sorted, similars, differs = calc_topk_pretrain(funcs1_sim, funcs2_sim, topk)
print_count = 0
for i in range(len(funcs1_sim)):
    sub_similars = similars[i]
    sub_sim_sorted = sim_sorted[i]
    if select == -1 and len(funcs1_sim) > 20:
        if hit_count_top[i][1] != 1:
            continue
    if print_count >= 1:
        continue
    print_count+=1

    print("INPUT_{}: {}-{} {}".format(i, funcs1_sim[i].target, funcs1_sim[i].compiler, funcs1_sim[i].meta['name']))
    for j in range(1, min(len(sub_sim_sorted) + 1, topk + 1)):
        id = sub_sim_sorted[-j]
        print(" -FIND_{}: [{:.4f}] {}-{} {} ".format(j, sub_similars[id], funcs2_sim[id].target, funcs2_sim[id].compiler, funcs2_sim[id].meta['name']))
    top1_id = sub_sim_sorted[-1]
    top1_file = '{}train-asm/{}/{}/{}'.format(DATA_HOME, funcs2_sim[top1_id].target, funcs2_sim[top1_id].compiler, funcs2_sim[top1_id].meta['name'])
    print(top1_file)
    
print('SRC funcs: {}, DST funcs: {}, sim-funcs: {}'.format(len(funcs1), len(funcs2), len(funcs1_sim)))
print('Top-1/3/{} recall:'.format(topk)) # 
hittotal = 0
for i in range(1, topk + 1):
    for hit in hit_count_top:
        hittotal += hit[i]
    if i == 1:
        print('{:.2f}/'.format(hittotal/len(funcs1_sim)), end="")
    if i == 3:
        print('{:.2f}/'.format(hittotal/len(funcs1_sim)), end="")
    if i == 5:
        print('{:.2f}'.format(hittotal/len(funcs1_sim)), end="")