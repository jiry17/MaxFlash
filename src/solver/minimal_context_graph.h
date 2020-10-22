#ifndef L2S_MINIMAL_CONTEXT_GRAPH_H
#define L2S_MINIMAL_CONTEXT_GRAPH_H

#include "context.h"
#include "context_maintainer.h"
#include "specification.h"

// A graph build from a topdown context model. It contains:
// (1) transitions between contexts, and (2) the heuristic value of each possible context.
// The heuristic value is the probability of the most probable program without considering the examples.
class MinimalContextGraph {
    void buildGraph();
    void addNewNode(NonTerminal* symbol, Context* context);
    void addNewNode(NonTerminal* symbol, Program* program, PathInfo& path_info);
public:
    struct Edge {
        std::vector<int> v;
        double w;
        int u, unfinished_num;
        Rule* rule;
        Edge(int _u, std::vector<int> _v, double _w, Rule* _rule): u(_u), v(_v), w(_w), unfinished_num(_v.size()), rule(_rule) {}
    };
    struct Node {
        Context* minimal_context;
        NonTerminal* symbol;
        std::vector<Edge*> edge_list, backward_edge_list;
        double upper_bound;
        Node(NonTerminal* _symbol, Context* _minimal_context): symbol(_symbol), minimal_context(_minimal_context) {}
    };
    ContextMaintainer* maintainer;
    ContextInfoMap* info_map;
    NonTerminal* start_symbol;

    std::vector<Node> minimal_context_list;
    std::unordered_map<std::string, int> minimal_context_map;
    void printUpperBound();

    static bool matchRuleWithName(std::string name, Rule* rule);
    static bool searchForValue(ContextInfo* context_info, Rule* rule, double& value);

    MinimalContextGraph(NonTerminal* _start_symbol, ContextMaintainer* _maintainer, ContextInfoMap* _info_map);
};


#endif //L2S_MINIMAL_CONTEXT_GRAPH_H
