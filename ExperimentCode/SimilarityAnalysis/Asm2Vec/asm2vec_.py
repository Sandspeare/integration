import torch
import torch.nn as nn
import click
from asm2vec import utils,model ,datatype

def cosine_similarity(v1, v2):
    '''
    caculate tow vector cosin distance
    '''
    return (v1 @ v2 / (v1.norm() * v2.norm())).item()


@click.command()
@click.option('-i1', '--input1', 'ipath1', default='/home/test/MyLib/asm2vec/output/bin1/a', help='target function 1', required=True)
#@click.option('-i2', '--input2', 'ipath2', default='/home/test/MyLib/asm2vec/output/bin2/b', help='target function 2', required=True)
@click.option('-m', '--model', 'mpath', default='model.pt', help='model path', required=True)
@click.option('-e', '--epochs', default=10, help='training epochs', show_default=True)
@click.option('-c', '--device', default='auto', help='hardware device to be used: cpu / cuda / auto', show_default=True)
@click.option('-lr', '--learning-rate', 'lr', default=0.02, help="learning rate", show_default=True)
def cli(ipath1, mpath, epochs, device, lr):
    if device == 'auto':
        device = 'cuda' if torch.cuda.is_available() else 'cpu'

    # load model, tokens
    model, tokens = utils.load_model(mpath, device=device)
    functions, tokens_new = utils.load_data([ipath1])
    tokens.update(tokens_new)
    model.update(function_size_new=2, vocab_size_new=tokens.size())
    model = model.to(device)
    
    # train function embedding
    model = utils.train(
        functions,
        tokens,
        model=model,
        epochs=epochs,
        device=device,
        mode='test',
        learning_rate=lr
    )

    # compare 2 function vectors
    v1 = model.to('cpu').embeddings_f(torch.tensor([0]))

    #print(f'cosine similarity : {cosine_similarity(v1, v2):.6f}')

if __name__ == '__main__':
    cli()
