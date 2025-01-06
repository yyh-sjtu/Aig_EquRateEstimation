import os 
import glob
import deepgate as dg
from torch_geometric.data import Data, InMemoryDataset
import torch
import numpy as np 
import random
import copy
import time
import argparse
import torch.nn.functional as F
import re
import time 

import sys
current_dir = os.path.dirname(os.path.abspath(__file__))
parent_dir = os.path.dirname(current_dir)
sys.path.append(parent_dir)

# print("Current working directory:", os.getcwd())

import utils.aiger_utils as aiger_utils
import utils.circuit_utils as circuit_utils

gate_to_index={'PI': 0, 'AND': 1, 'NOT': 2, 'DFF': 3}
NODE_CONNECT_SAMPLE_RATIO = 0.1
NO_NODE_PATH = 10
NO_NODE_HOP = 10
K_HOP = 4

NO_NODES = [30, 500000]

import sys
sys.setrecursionlimit(1000000)

def get_parse_args():
    parser = argparse.ArgumentParser()

    parser.add_argument('aig1', type=str, help='Input parameter 1')
    parser.add_argument('aig2', type=str, help='Input parameter 2')
    
    args = parser.parse_args()
    
    return args

class OrderedData(Data):
    def __init__(self): 
        super().__init__()
    
    def __inc__(self, key, value, *args, **kwargs):
        # if 'hop_forward_index' in key:
        #     return value.shape[0]
        # elif 'path_forward_index' in key:
        #     return value.shape[0]
        if key == 'ninp_node_index' or key == 'ninh_node_index':
            return self.num_nodes
        elif key == 'ninp_path_index':
            return args[0]['path_forward_index'].shape[0]
        elif key == 'ninh_hop_index':
            return args[0]['hop_forward_index'].shape[0]
        elif key == 'hop_pi' or key == 'hop_po' or key == 'hop_nodes': 
            return self.num_nodes
        elif key == 'winhop_po' or key == 'winhop_nodes':
            return self.num_nodes
        elif key == 'hop_pair_index' or key == 'hop_forward_index':
            return args[0]['hop_forward_index'].shape[0]
        elif key == 'path_forward_index':
            return args[0]['path_forward_index'].shape[0]
        elif key == 'paths' or key == 'hop_nodes':
            return self.num_nodes
        elif 'index' in key or 'face' in key:
            return self.num_nodes
        else:
            return 0

    def __cat_dim__(self, key, value, *args, **kwargs):
        if key == 'forward_index' or key == 'backward_index':
            return 0
        elif key == "edge_index" or key == 'tt_pair_index' or key == 'rc_pair_index':
            return 1
        elif key == "connect_pair_index" or key == 'hop_pair_index':
            return 1
        elif key == 'hop_pi' or key == 'hop_po' or key == 'hop_pi_stats' or key == 'hop_tt' or key == 'no_hops':
            return 0
        elif key == 'winhop_po' or key == 'winhop_nodes' or key == 'winhop_nodes_stats' or key == 'winhop_lev':
            return 0
        elif key == 'hop_nodes' or key == 'hop_nodes_stats':
            return 0
        elif key == 'paths':
            return 0
        else:
            return 0
        
def get_graph(aig_file):
    x_data, edge_index = aiger_utils.aig_to_xdata(aig_file)
    # fanin_list, fanout_list = circuit_utils.get_fanin_fanout(x_data, edge_index)
    
    # num_and = 0
    # for i in x_data:
    #     if i[1] == 1:
    #         num_and += 1
    # print(f"AND: {num_and}")
    
    x_data = np.array(x_data)
    
    x_one_hot = dg.construct_node_feature(x_data, 3)
    edge_index = torch.tensor(edge_index, dtype=torch.long).t().contiguous()
    forward_level, forward_index, backward_level, backward_index = dg.return_order_info(edge_index, x_one_hot.size(0))
    
    graph = OrderedData()
    graph.x = x_one_hot
    graph.edge_index = edge_index
    graph.name = os.path.basename(aig_file).replace('.aig', '')
    graph.gate = torch.tensor(x_data[:, 1], dtype=torch.long).unsqueeze(1)
    graph.forward_index = forward_index
    graph.backward_index = backward_index
    graph.forward_level = forward_level
    graph.backward_level = backward_level
    return graph

def get_equ_rate_from_output(text):
    match = re.search(r"Equivalence Rate:\s*(\d+\.\d+)", text)
    if match:
        return float(match.group(1))
    return None

if __name__ == '__main__': 
    args = get_parse_args()
    
    aig_file1 = args.aig1
    aig_file2 = args.aig2
    if aig_file1 == '' or aig_file2 == '':
        print("Please provide the path to the AIGER files.")
        exit(1)
        
    start_time = time.time()
    graph1, graph2 = get_graph(aig_file1), get_graph(aig_file2)
    # for i in range(graph1.edge_index.shape[1]):
    #     print(f"{graph1.edge_index[0][i]}->{graph1.edge_index[1][i]}")
    ################################################
    # DeepGate2 (node-level) labels
    ################################################
    result, exec_time = circuit_utils.get_equ_rate_cpp(graph1, graph2, 15000, fast=True)
    equ_rate = get_equ_rate_from_output(result)
    print(result)
    print(f"Equ_rate: {equ_rate}")
    print(f"Runtime: {exec_time}")
    
    print(f"Total runtime: {time.time() - start_time}")
