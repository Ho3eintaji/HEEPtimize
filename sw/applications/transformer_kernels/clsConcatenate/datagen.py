import argparse
import os
import numpy as np

from c_gen import CFileGen

class TokenPosEmbedding:
    def __init__(self, seq_len, input_dim, pos_matrix_dim, cls_token_vector, pos_matrix):
        self.cls_token_vector_ = cls_token_vector
        self.pos_matrix_ = pos_matrix
        self.seq_len_ = seq_len
        self.input_dim_ = input_dim
        self.pos_matrix_dim_ = pos_matrix_dim

def create_token_pos_embedding(seq_len, input_dim, pos_matrix_dim, cls_token_vector, pos_matrix):
    return TokenPosEmbedding(seq_len, input_dim, pos_matrix_dim, cls_token_vector, pos_matrix)

def cls_concatenate(tpe, input_data):
    # Create the concatenated input (CLS token first, followed by the input data)
    concatenated_input = np.zeros((tpe.seq_len_ + 1, tpe.input_dim_), dtype=np.int16)
    
    # Insert CLS token vector at the beginning
    concatenated_input[0, :] = tpe.cls_token_vector_
    
    # Insert the input data (sequences) starting from index 1
    concatenated_input[1:, :] = input_data
    
    return concatenated_input

def pos_embedding(tpe, input_data):
    # Add positional embeddings to the input data
    for i in range(tpe.seq_len_ + 1):  # Including the CLS token
        input_data[i, :] += tpe.pos_matrix_[i, :]
    
    return input_data

def generate_data(seq_len, input_dim):
    cls_token_vector = np.random.randint(-32768, 32767, size=(1, input_dim), dtype=np.int16)
    pos_matrix = np.random.randint(-32768, 32767, size=(seq_len + 1, input_dim), dtype=np.int16)
    input = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
    pos_matrix_dim = input_dim

    tpe = create_token_pos_embedding(seq_len, input_dim, pos_matrix_dim, cls_token_vector, pos_matrix)

    # Step 1: Concatenate CLS token with input data
    concatenated_input = cls_concatenate(tpe, input)

    # Step 2: Apply positional embeddings
    final_input = pos_embedding(tpe, concatenated_input)

    return input, cls_token_vector, pos_matrix, concatenated_input, final_input

# def generate_data(seq_len, input_dim):
#     input = np.random.randint(-32768, 32767, size=(seq_len, input_dim), dtype=np.int16)
#     cls_token_vector = np.random.randint(-32768, 32767, size=(1, input_dim), dtype=np.int16)
#     return input, cls_token_vector

def main():
    parser = argparse.ArgumentParser(description='Generate input data for clsConcatenate kernel.')
    parser.add_argument('--seq_len', type=int, default=32, help='Sequence length')
    parser.add_argument('--input_dim', type=int, default=64, help='Input dimension')
    parser.add_argument('--outdir', '-o', type=str, default=os.path.dirname(__file__), help='Output directory')
    args = parser.parse_args()

    input, cls_token_vector, pos_matrix, concatenated_input, final_input = generate_data(args.seq_len, args.input_dim)

    out_dir = args.outdir
    data_header = 'data.h'
    header_gen = CFileGen()
    header_gen.add_macro('SEQ_LEN', args.seq_len, "Sequence length")
    header_gen.add_macro('INPUT_DIM', args.input_dim, "Input dimension")
    header_gen.add_input_matrix('input', input)
    header_gen.add_input_matrix('cls_token_vector', cls_token_vector)
    header_gen.add_attribute('section(".xheep_data_interleaved")')
    header_gen.write_header(out_dir, data_header)
    print(f'- generated header file in \'{out_dir}/{data_header}\'.')

if __name__ == '__main__':
    main()
