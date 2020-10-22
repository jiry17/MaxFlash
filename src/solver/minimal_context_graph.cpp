#include "minimal_context_graph.h"
#include "semantics_factory.h"

#include <queue>
#include <cmath>
#include <config.h>
#include <iostream>

namespace {
    std::map<std::string, Semantics*> temp_semantics_map = {
            {"Param@Int", new ParamSemantics(0, TINT)},
            {"Param@Bool", new ParamSemantics(0, TBOOL)},
            {"Param@String", new ParamSemantics(0, TSTRING)},
            {"Constant@Int", new ConstSemantics(Data(new IntValue(0)))},
            {"Constant@Bool", new ConstSemantics(Data(new BoolValue(false)))},
            {"Constant@String", new ConstSemantics(Data(new StringValue("")))}
    };

    bool checkFinished(std::string name) {
        return name.find("Param@") != std::string::npos || name.find("Constant@") != std::string::npos;
    }

    Semantics* getTempSemantics(std::string name) {
        if (checkFinished(name)) return temp_semantics_map[name];
        return string2Semantics(name);
    }

    struct TempNodeInfo {
        Program* program;
        PathInfo path;
        TempNodeInfo(Program* _program, PathInfo _path): program(_program), path(_path) {}
    };

    std::string encodeState(NonTerminal* symbol, Context* context) {
        return symbol->name + "@" + context->encodeContext();
    }
}

bool MinimalContextGraph::matchRuleWithName(std::string name, Rule* rule) {
    if (name.find("Param@") != std::string::npos) {
        auto* semantics = dynamic_cast<ParamSemantics*>(rule->semantics);
        return semantics != nullptr && name.find(util::type2String(semantics->oup_type)) != std::string::npos;
    }
    if (name.find("Constant@") != std::string::npos) {
        auto* semantics = dynamic_cast<ConstSemantics*>(rule->semantics);
        if (semantics == nullptr) return false;
        if (global::spec_type == S_PBE && semantics->oup_type == TSTRING) {
            return name == util::getStringConstType(semantics->value.getString());
        }
        return semantics != nullptr && name.find(util::type2String(semantics->oup_type)) != std::string::npos;
    }
    auto* semantics = string2Semantics(name);
    return rule->semantics->name == semantics->name;
}

bool MinimalContextGraph::searchForValue(ContextInfo* context_info, Rule* rule, double& value) {
    for (auto& rule_info: (*context_info)) {
        std::string name = rule_info.first;
        if (matchRuleWithName(name, rule)) {
            value = rule_info.second;
            return true;
        }
    }
    return false;
}

void MinimalContextGraph::addNewNode(NonTerminal* symbol, Context *current_context) {
    int pos = minimal_context_list.size();
    minimal_context_list.emplace_back(Node(symbol, current_context));
    std::string context_name = encodeState(symbol, current_context);
#ifdef DEBUG
    assert(minimal_context_map.count(context_name) == 0);
#endif
    minimal_context_map[context_name] = pos;
}

void MinimalContextGraph::addNewNode(NonTerminal* symbol, Program *program, PathInfo &path_info) {
    maintainer->partial_program = program;
    Context* current_context = maintainer->getMinimalContext(path_info);
    addNewNode(symbol, current_context);
}

void MinimalContextGraph::buildGraph() {
    std::vector<TempNodeInfo> temp_info;
    temp_info.push_back(TempNodeInfo(new Program(), {}));
    addNewNode(start_symbol, temp_info[0].program, temp_info[0].path);
    std::queue<int> Q;
    std::vector<Node>& node_list = minimal_context_list;
    Q.push(0);
    while (!Q.empty()) {
        int current_pos = Q.front(); Q.pop();
        PathInfo current_path = temp_info[current_pos].path;
        Program* current_program = temp_info[current_pos].program;
        maintainer->partial_program = current_program;
        Context* abstracted_context = maintainer->getAbstractedContext(current_path);

        auto* context_info = info_map->getContextInfo(abstracted_context);
        auto* symbol = node_list[current_pos].symbol;
        double sum = 0.0;
        for (auto* rule: symbol->rule_list) {
            double value = 0;
            if (searchForValue(context_info, rule, value)) sum += std::max(value, config::KDefaultP);
            else sum += config::KDefaultP;
        }
        if (global::isMatrix) sum = 1.0;
        for (auto* rule: symbol->rule_list) {
            double value = 0;
            if (!searchForValue(context_info, rule, value)) {
                value = config::KDefaultP;
            }
            /*if (dynamic_cast<ConstSemantics*>(rule->semantics)) {
                std::cout << "now " << abstracted_context->encodeContext() << std::endl;
                std::cout << "const " << value << std::endl;
                for (auto info: *context_info) {
                    std::cout << info.first << " " << info.second << std::endl;
                }
            }*/
            if (value == 0) {
                continue;
            }
            value = std::max(value, config::KDefaultP);
            value = std::min(0.0, std::log(value / sum));
            Semantics* semantics = rule->semantics;
            maintainer->partial_program = new Program(current_program);
            maintainer->update(current_path, semantics);
            std::vector<int> v;
            for (int i = 0; i < semantics->inp_type_list.size(); ++i) {
                current_path.push_back(i);
                Context* sub_context = maintainer->getMinimalContext(current_path);
                NonTerminal* sub_symbol = rule->param_list[i];
                std::string feature = encodeState(sub_symbol, sub_context);
                int id = 0;
                if (minimal_context_map.count(feature) == 0) {
                    id = minimal_context_list.size();
                    temp_info.push_back(TempNodeInfo(new Program(maintainer->partial_program), current_path));
                    addNewNode(sub_symbol, sub_context);
                    Q.push(id);
                } else id = minimal_context_map[feature];
                v.push_back(id);
                current_path.pop_back();
            }
            maintainer->clear();
            Edge* edge = new Edge(current_pos, v, value, rule);
            minimal_context_list[current_pos].edge_list.push_back(edge);
            for (auto p: v) {
                minimal_context_list[p].backward_edge_list.push_back(edge);
            }
        }
    }
}

MinimalContextGraph::MinimalContextGraph(NonTerminal* _start_symbol, ContextMaintainer *_maintainer, ContextInfoMap *_info_map):
    start_symbol(_start_symbol), maintainer(_maintainer), info_map(_info_map) {
    buildGraph();
    auto& node_list = minimal_context_list;
    std::vector<bool> is_finished;
    std::priority_queue<std::pair<double, int> > Q;

    for (int node_id = 0; node_id < node_list.size(); ++node_id) {
        Node& node = node_list[node_id];
        is_finished.push_back(false);
        node.upper_bound = -1e100;
        bool is_changed = false;
        for (auto* edge: node.edge_list) {
            if (edge->unfinished_num == 0) {
                node.upper_bound = std::max(node.upper_bound, edge->w);
                is_changed = true;
            }
        }
        if (is_changed) Q.push(std::make_pair(node.upper_bound, node_id));
    }

    while (!Q.empty()) {
        int pos = Q.top().second;
        double w = Q.top().first;
        Node& node = node_list[pos];
        Q.pop();
        if (std::fabs(w - node.upper_bound) > 1e-6) continue;
        is_finished[pos] = true;
        for (auto* edge: node.backward_edge_list) {
            --edge->unfinished_num;
            if (edge->unfinished_num == 0) {
                int u = edge->u;
                if (is_finished[u]) continue;
                double current_w = edge->w;
                for (int v_id: edge->v) {
                    current_w += node_list[v_id].upper_bound;
                }
                if (current_w > node_list[u].upper_bound + 1e-6) {
                    node_list[u].upper_bound = current_w;
                    Q.push(std::make_pair(current_w, u));
                }
            }
        }
    }
}

void MinimalContextGraph::printUpperBound() {
    for (auto& node: minimal_context_list) {
        printf("%s : %.3lf\n", encodeState(node.symbol, node.minimal_context).c_str(), node.upper_bound);
        printf("Edge:\n");
        for (auto* edge: node.edge_list) {
            std::cout << edge->rule->semantics->name << " " << edge->w << std::endl;
        }
        /*printf("Previous Node:\n");
        for (auto* edge: node.backward_edge_list) {
            printf("%s\n", minimal_context_list[edge->u].minimal_context->encodeContext().c_str());
        }
        puts("==================");*/
    }
}