#ifndef L2S_SPECIFICATION_H
#define L2S_SPECIFICATION_H

#include "semantics.h"
#include "program.h"
#include "json/json.h"

#include <map>

class Specification;
class Rule;

// A non-terminal symbol in a DSL.
class NonTerminal {
public:
    std::string name;
    Type type;
    std::vector<Rule*> rule_list;
    NonTerminal(std::string _name, Type _type): name(_name), type(_type) {}
};

// An input-output examples.
class Example {
public:
    DataList inp;
    Data oup;
    Example(const DataList& _inp, Data& _oup): inp(_inp), oup(_oup) {}
    std::string toString() const;
};

// A specification.
// The grammar expanded from "start_terminal" represents the syntax constraint.
// Program "oracle" represents the semantics constraint for S_ORACLE: A program is valid if and only if it is
// semantically equivalent with "oracle".
// Example space "example_space" represents the semantics constraint for S_PBE. A program is valid if and only
// if it is consistent with all examples in "example_space".
class Specification {
    void recognizeSpecType();
    void initGlobalInfoForPBE();
    bool checkOracle(); // Only valid for S_ORACLE
    bool checkPBE(); // Only valid for S_PBE
public:
    Json::Value constraint_root, extra_root;
    std::map<std::string, int> param_map;
    struct ParamInfo {
        std::string name;
        Type type;
        ParamInfo(std::string _name, Type _type): name(_name), type(_type) {}
    };
    std::vector<ParamInfo> param_list;
    Type return_type;
    std::map<std::string, NonTerminal*> non_terminal_map;
    Program* oracle;
    NonTerminal* start_terminal;
    std::map<std::string, Type> global_variable_map;
    std::vector<Example*> example_space;
    Specification() {};
    Specification(std::string file_name);
    void print();
    bool verify(Program* program, Example*& counter_example);
};

// A grammar rule in a DSL.
class Rule {
    void check() {
        assert(semantics->inp_type_list.size() == param_list.size());
        for (int i = 0; i < semantics->inp_type_list.size(); ++i) {
            assert(semantics->inp_type_list[i] == param_list[i]->type);
        }
    }
public:
    Semantics* semantics;
    std::vector<NonTerminal*> param_list;
    Rule(Semantics* _semantics, const std::vector<NonTerminal*> _param_list): semantics(_semantics), param_list(_param_list) {
#ifdef DEBUG
        check();
#endif
    }
};


#endif //L2S_SPECIFICATION_H
