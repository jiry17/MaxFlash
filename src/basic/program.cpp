#include <iostream>

#include "program.h"
#include "util.h"
#include "config.h"
#include "semantics_factory.h"

Data Program::run(GlobalInfo *inp) {
    DataList sub_expr;
    for (auto* sub_program: sub_list) {
        sub_expr.push_back(sub_program->run(inp));
    }
    return semantics->run(sub_expr, inp);
}

Data Program::run(const DataList &inp) {
    if (global::spec_type == S_ORACLE) {
        auto *param_info = new ParamInfo(inp);
        Data result = run(param_info);
        delete param_info;
        return result;
    } else if (global::spec_type == S_PBE) {
        global::string_info->setInp(inp);
        Data result = run(global::string_info);
        return result;
    } else assert(0);
}

void Program::initFromJsonNode(Json::Value program_root) {
    std::string node_type = program_root["type"].asString();
    if (node_type == "var") {
        int id = program_root["id"].asInt();
        Type type = util::string2Type(program_root["var_type"].asString());
        semantics = new ParamSemantics(id, type);
    } else if (node_type == "const") {
        semantics = new ConstSemantics(util::parseDataFromJson(program_root));
    } else if (node_type == "expr") {
        semantics = string2Semantics(program_root["operator"].asString());
        auto sub_root = program_root["params"];
        for (auto sub_node: sub_root) {
            auto* sub_program = new Program(sub_node);
            sub_list.push_back(sub_program);
        }
    }
}

Program::Program(Json::Value program_root) {
    initFromJsonNode(program_root);
}

Program::Program(std::string file_name) {
    Json::Value root = util::loadJsonFromFile(file_name);
    initFromJsonNode(root);
}

std::string Program::toString() {
    if (sub_list.size() == 0) return semantics->name;
    std::string result = "(" + semantics->name;
    for (auto* sub_expr: sub_list) {
        result += " " + sub_expr->toString();
    }
    return result + ")";
}

void Program::print() {
    std::cout << toString() << std::endl;
}