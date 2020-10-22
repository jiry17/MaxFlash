#ifndef L2S_PROGRAM_H
#define L2S_PROGRAM_H

#include "json/json.h"

#include "semantics.h"

// An AST of a program (possibly partial)
// "semantics = nullptr" represents a placeholder in a partial AST.
class Program {
    void initFromJsonNode(Json::Value program_root);
    Data run(GlobalInfo* inp);
public:
    std::vector<Program*> sub_list;
    Semantics* semantics;

    Program(const std::vector<Program*> _sub_list, Semantics* _semantics): sub_list(_sub_list), semantics(_semantics) {}
    Program(Json::Value program_root);
    Program(std::string file_name);
    Program(Program* program) {
        semantics = program->semantics;
        for (auto sub_program: program->sub_list) {
            sub_list.push_back(new Program(sub_program));
        }
    }
    ~Program() {
        for (auto sub_program: sub_list) {
            delete sub_program;
        }
    }
    Program(): semantics(nullptr) {}

    std::string toString();
    void print();
    Data run(const DataList& inp);
};


#endif //L2S_PROGRAM_H
