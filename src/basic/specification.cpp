#include "specification.h"
#include "semantics_factory.h"
#include "util.h"
#include "config.h"

#include <fstream>
#include <sstream>
#include <iostream>

using util::string2Type;

Specification::Specification(std::string file_name) {
    Json::Value root = util::loadJsonFromFile(file_name);
    auto function_root = root["spec"];
    auto global_var_root = root["global_var"];
    constraint_root = root["constraint"];
    extra_root = root["extra"];
    for (auto& var_node: global_var_root) {
        global_variable_map[var_node["name"].asString()] = string2Type(var_node["type"].asString());
    }
    auto param_root = function_root["param"];
    int count = 0;
    for (auto& param_node: param_root) {
        std::string name = param_node["name"].asString();
        Type type = string2Type(param_node["type"].asString());
        param_map[name] = count++;
        param_list.push_back(Specification::ParamInfo(name, type));
    }
    return_type = string2Type(function_root["return_type"].asString());
    auto non_terminal_root = function_root["non_terminal"];
    for (auto& non_terminal_node: non_terminal_root) {
        std::string name = non_terminal_node["name"].asString();
        Type type = string2Type(non_terminal_node["type"].asString());
        non_terminal_map[name] = new NonTerminal(name, type);
    }
    for (auto& non_terminal_node: non_terminal_root) {
        NonTerminal* non_terminal = non_terminal_map[non_terminal_node["name"].asString()];
        auto& rule_root = non_terminal_node["rule"];
        for (auto& rule_node: rule_root) {
            auto rule_type = rule_node["type"].asString();
            std::vector<NonTerminal*> sub_expr;
            Semantics* semantics = nullptr;
            if (rule_type == "expr") {
                semantics = string2Semantics(rule_node["operator"].asString());
                auto& param_list_node = rule_node["param"];
                for (auto& param: param_list_node) {
                    std::string param_name = param.asString();
                    sub_expr.push_back(non_terminal_map[param_name]);
                }
            } else if (rule_type == "var") {
                auto param_type = string2Type(rule_node["var_type"].asString());
                std::string param_name = rule_node["var_name"].asString();
#ifdef DEBUG
                assert(param_map.count(param_name));
#endif
                int param_id = param_map[param_name];
#ifdef DEBUG
                assert(param_type == param_list[param_id].type);
#endif
                semantics = new ParamSemantics(param_id, param_type);
            } else if (rule_type == "const") {
                semantics = new ConstSemantics(util::parseDataFromJson(rule_node));
            }
            (*non_terminal).rule_list.push_back(new Rule(semantics, sub_expr));
        }
    }
    start_terminal = nullptr;
    for (auto& symbol_pair: non_terminal_map) {
        std::string name = symbol_pair.first;
        auto* symbol = symbol_pair.second;
        if (name.find("Start") != std::string::npos && symbol->type == return_type) {
            assert(start_terminal == nullptr);
            start_terminal = symbol;
        }
    }
    recognizeSpecType();
    if (global::spec_type == S_PBE) {
        initGlobalInfoForPBE();
    }
    assert(start_terminal != nullptr);
}

bool Specification::verify(Program *program, Example*& counter_example) {
    assert(global::spec_type == S_PBE);
    if (global::spec_type == S_PBE) {
        for (auto* example: example_space) {
            if (program->run(example->inp) != example->oup) {
                counter_example = new Example(example->inp, example->oup);
                return false;
            }
        }
        return true;
    }
}

void Specification::recognizeSpecType() {
    if (checkOracle()) {
        global::spec_type = S_ORACLE;
    } else if (checkPBE()) {
        global::spec_type = S_PBE;
    } else assert(false);
}

void Specification::initGlobalInfoForPBE() {
    global::string_info->clear();
    global::string_info->example_space = example_space;
    global::string_info->spec = this;
    for (const auto& symbol_pair: non_terminal_map) {
        auto& symbol = symbol_pair.second;
        for (auto* rule: symbol->rule_list) {
            auto* const_semantics = dynamic_cast<ConstSemantics*>(rule->semantics);
            if (const_semantics != nullptr) {
                switch (const_semantics->value.getType()) {
                    case TINT:
                        global::string_info->const_list.push_back(
                                Data(new StringValue(std::to_string(const_semantics->value.getInt()))));
                        break;
                    case TSTRING:
                        global::string_info->const_list.push_back(const_semantics->value);
                        break;
                    default:
                        break;
                }
            }
        }
    }
    global::KIntMax = 0;
    for (auto s: global::string_info->const_list) {
        global::KIntMax = std::max(global::KIntMax, int(s.getString().length()));
    }
    for (auto* example: global::string_info->example_space) {
        for (auto& data: example->inp) {
            if (data.getType() == TSTRING) {
                global::KIntMax = std::max(global::KIntMax, int(data.getString().length()));
            }
        }
        if (example->oup.getType() == TSTRING) {
            global::KIntMax = std::max(global::KIntMax, int(example->oup.getString().length()));
        }
    }
    // std::cout << "ConstList" << util::dataList2String(global::string_info->const_list) << std::endl;
}

bool Specification::checkPBE() {
    if (extra_root.size() > 0) return false;
    example_space.clear();
    for (auto& constraint: constraint_root) {
        if (constraint["type"].asString() != "expr") return false;
        auto param_root = constraint["params"];
        if (param_root.size() != 2) return false;
        auto l_expr = param_root[0], r_expr = param_root[1];
        if (l_expr["type"] != "function") std::swap(l_expr, r_expr);
        if (l_expr["type"] != "function" || r_expr["type"] != "const") return false;
        Data oup = util::parseDataFromJson(r_expr);
        DataList inp;
        for (auto l_param: l_expr["params"]) {
            if (l_param["type"] != "const") return false;
            inp.push_back(util::parseDataFromJson(l_param));
        }
        example_space.push_back(new Example(inp, oup));
    }
    return true;
}

bool Specification::checkOracle() {
    if (constraint_root.size() != 1) return false;
    auto constraint = constraint_root[0];
    if (constraint["type"].asString() != "expr") return false;
    if (constraint["operator"].asString() != "=") return false;
    auto params_root = constraint["params"];
    if (params_root.size() != 2) return false;
    auto l_expr = params_root[0];
    auto r_expr = params_root[1];
    if (l_expr["type"].asString() != "var") {
        std::swap(l_expr, r_expr);
    }
    if (l_expr["type"].asString() != "var") return false;
    if (r_expr["type"].asString() != "function") return false;
    params_root = r_expr["params"];
    for (auto& param_node: params_root) {
        if (param_node["type"].asString() != "var") return false;
    }
    return true;
}

std::string Example::toString() const {
    return util::dataList2String(inp) + " => " + oup.toString();
}

void Specification::print() {
    for (auto& symbol_pair: non_terminal_map) {
        auto* symbol = symbol_pair.second;
        std::cout << "Symbol " << symbol->name << std::endl;
        for (auto* rule: symbol->rule_list) std::cout << rule->semantics->name << " "; std::cout << std::endl;
    }
}