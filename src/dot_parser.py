'''
Utils for read CDFG in .dot format and transfer it into networkx graph and numpy format
Author: Yunhao Zhou
'''

import networkx as nx
import pydot
import matplotlib.pyplot as plt
import os
from queue import Queue
from glob import glob
import numpy as np
import json
import re

def dot_to_nxgraph(dot_file):
    # read .dot file
    (graph,) = pydot.graph_from_dot_file(dot_file)
    # transform pydot into networkx graph
    nx_graph = nx.nx_pydot.from_pydot(graph)
    return nx_graph

def draw_graph(graph: nx.graph, output_file):
    pos = nx.nx_agraph.graphviz_layout(graph, prog='dot')  # set layout
    edge_labels = {(u, v): d['label'] for u, v, d in graph.edges(data=True) if 'label' in d}
    node_labels = {node: data.get('label', str(node)).strip('\"') for node, data in graph.nodes(data=True)}
    
    nx.draw(graph, pos, with_labels=True, labels=node_labels, node_size=70, node_color="skyblue", font_size=2, font_color="black", edge_color="gray")
    nx.draw_networkx_edge_labels(graph, pos, edge_labels=edge_labels, font_color='red', font_size=3)
    
    plt.savefig(output_file, dpi=500)
    plt.close()

def find_fanin_is_zero(nodes: list[str], edges: list[tuple], remove = False) -> list[str]:
    fanin_zero_nodes = []
    for node in nodes:
        fanin_is_zero = True
        for edge in edges:
            if edge[1] == node:
                fanin_is_zero = False
                break
        if fanin_is_zero:
            fanin_zero_nodes.append(node)

    # remove fanin_zero_node
    if remove:
        for fanin_zero_node in fanin_zero_nodes:
            # remove relevant nodes
            nodes.remove(fanin_zero_node)
            # collect relevant edges
            edges_to_remove = [edge for edge in edges if edge[0] == fanin_zero_node]
            # remove edges
            for edge_to_remove in edges_to_remove:
                edges.remove(edge_to_remove)
            
    return fanin_zero_nodes

# can't process graph with cycle
def get_level(nx_graph: nx.graph, remove = True):
    # get the level of each node with bf topology sort
    level_list = []
    nodes = list(set(nx_graph.nodes()))
    edges = list(set(nx_graph.edges()))
    while len(nodes) > 0:
        level_list.append(find_fanin_is_zero(nodes, edges, remove=remove))
    return level_list

def get_fanout_dict(nx_graph):
    nodes = list(set(nx_graph.nodes()))
    edges = list(set(nx_graph.edges()))
    fanout_dict = {node: [] for node in nodes}
    for edge in edges:
        fanout_dict[edge[0]].append(edge[1])
    return fanout_dict

def write_json(content, file_name, output_dir = './my_output'):
    output_path = os.path.join(output_dir, f'{file_name}.json')
    with open(output_path, 'w') as f:
        json.dump(content, f, indent=4)
        
def get_node_dict(nx_graph: nx.graph) -> dict:
    """_summary_

    Args:
        nx_graph (nx.graph): --

    Returns:
        dict: {
            'node_id': {
                'type': 'node_type',  # 'PI', 'PO', 'AND', 'HA', 'FA'
                'fanin': [fanin_node_id],
                'fanout': {'o': ['56', '54']}  # {'s': ['56', '54'], 'c': ['58', '59']}  if 'HA', 'FA'
            }
        }
    """
    node_dict = {}
    
    for node, data in nx_graph.nodes(data=True):
        # rm const
        if node == '0' or node == '1':
            continue
        
        node_dict[node] = {
            'type': data.get('label', str(node)).strip('\"'),
            'fanin': [],
            'fanout': {}
            }
        
    for u, v, d in nx_graph.edges(data=True):  # if 'label' in d
        node_dict[v]['fanin'].append(u)
        if 'label' in d:
            if not d['label'] in node_dict[u]['fanout']:
                node_dict[u]['fanout'][d['label']] = []
            node_dict[u]['fanout'][d['label']].append(v)
        else:
            if not 'o' in node_dict[u]['fanout']:
                node_dict[u]['fanout']['o'] = []
            node_dict[u]['fanout']['o'].append(v)
            
    return node_dict
    
    
        
if __name__ == '__main__':
    
    dot_file = "dot_file/test.dot"
    output_file = "my_output/test.png"
    nx_graph = dot_to_nxgraph(dot_file)
    # print(nx_graph)
    draw_graph(nx_graph, output_file)
    print(get_level(nx_graph))
    print(get_node_dict(nx_graph))
