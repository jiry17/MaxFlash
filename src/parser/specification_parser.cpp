#include "specification_parser.h"
#include "semantics_factory.h"
#include "config.h"
#include <cstring>
#include <iostream>

namespace {
    std::vector<int> extractAllInt(std::string s) {
        std::vector<int> result;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] >= '0' && s[i] <= '9') {
                int value = 0;
                while (i < s.length() && s[i] >= '0' && s[i] <= '9') {
                    value = value * 10 + int(s[i] - '0');
                    ++i;
                }
                result.push_back(value);
            }
        }
        return result;
    }

    std::string normalize(std::string s) {
        int l = 0, r = s.length() - 1;
        while (s[l] == ' ' && l < r) ++l;
        while (s[r] == ' ' && l < r) --r;
        return s.substr(l, r - l + 1);
    }

    Program* parseProgram(std::string s) {
        s = normalize(s);
        if (s == "x") {
            return new Program({}, new ParamSemantics(0, TMATRIX));
        }
        bool is_number = 1;
        for (int i = 0; i < s.length(); ++i) {
            if (s[i] < '0' || s[i] > '9') {
                is_number = 0;
                break;
            }
        }
        if (is_number) {
            return new Program({}, new ConstSemantics(Data(new IntValue(std::stoi(s)))));
        }
        assert(s[s.length() - 1] == ')');
        int start = 0;
        while (s[start] != '(') ++start;
        std::string op = s.substr(0, start);
        auto* semantics = string2Semantics(op);
        s[s.length() - 1] = ',';
        int pre = start + 1;
        int left_num = 0;
        std::vector<Program*> sub_list;
        for (int i = start + 1; i < s.length(); ++i) {
            if (s[i] == '(') ++left_num;
            if (s[i] == ')') --left_num;
            if (s[i] == ',' && left_num == 0) {
                std::string param_string = s.substr(pre, i - pre);
                pre = i + 1;
                sub_list.push_back(parseProgram(normalize(param_string)));
            }
        }
        return new Program(sub_list, semantics);
    }

    Data parseMatrix(std::string s) {
        std::vector<int> shape = extractAllInt(s);
        int size = 1;
        for (int i = 0; i < shape.size(); ++i) {
            size *= shape[i];
        }
        std::vector<int> contents;
        for (int i = 1; i <= size; ++i) {
            contents.push_back(i);
        }
        return Data(new MatrixValue(contents, shape));
    }

    Specification *loadMatrixSpecification(std::string file_name) {
        static char ch[5100];
        std::vector<std::string> info;
        auto* file = fopen(file_name.c_str(), "r");
        while (fgets(ch, 1000, file) != NULL) {
            ch[strlen(ch) - 1] = '\0';
            info.emplace_back(ch);
        }
        fclose(file);

        auto* start = new NonTerminal("start", TMATRIX);
        auto* vec = new NonTerminal("vec", TMATRIX);
        auto* param = new NonTerminal("param", TMATRIX);
        auto* int_const = new NonTerminal("const", TINT);
        for (int i = 0; i <= 3; ++i) {
            global::string_info->int_const.push_back(i);
            int_const->rule_list.push_back(new Rule(new ConstSemantics(Data(new IntValue(i))), {}));
        }

        param->rule_list.push_back(new Rule(new ParamSemantics(0, TMATRIX), {}));
        vec->rule_list.push_back(new Rule(string2Semantics("L"), {int_const, vec}));
        vec->rule_list.push_back(new Rule(string2Semantics("B"), {int_const, int_const}));
        start->rule_list.push_back(new Rule(string2Semantics("Reshape"),{start, vec}));
        start->rule_list.push_back(new Rule(string2Semantics("Permute"), {start, vec}));
        start->rule_list.push_back(new Rule(string2Semantics("Var"), {param}));
        start->rule_list.push_back(new Rule(string2Semantics("Fliplr"), {start}));
        start->rule_list.push_back(new Rule(string2Semantics("Flipud"), {start}));

        Specification* specification = new Specification();
        specification->start_terminal = start;
        specification->return_type = TMATRIX;
        global::spec_type = S_PBE;

        std::vector<int> size_list = extractAllInt(info[1]);
        int tot = 1;
        for (int size: size_list) tot *= size;
        auto& int_list = global::string_info->int_const;
        std::vector<int> extra = size_list;
        for (int i = 2; i <= 10; ++i) if (tot % i == 0) extra.push_back(i);
        for (int i = 7; i < info.size(); ++i) {
            std::vector<int> extra_int_const = extractAllInt(info[i]);
            for (int ex: extra_int_const) extra.push_back(ex);
        }
        for (int i: extra) {
            if (i && tot % i != 0) continue;
            if (std::find(int_list.begin(), int_list.end(), i) == int_list.end()) {
                int_list.push_back(i);
                int_const->rule_list.push_back(new Rule(new ConstSemantics(Data(new IntValue(i))), {}));
            }
        }

        auto inp = parseMatrix(info[1]);
        Program* program = parseProgram(info[4]);
        auto oup = program->run({inp});
        global::KMaxDim = std::max(global::KMaxDim, int(std::max(inp.getMatrix().shape.size(), oup.getMatrix().shape.size())));
        specification->oracle = program;

        specification->example_space.push_back(new Example({inp}, oup));
        return specification;
    }
}

Specification* parser::loadSpecification(std::string file_name, std::string benchmark_type) {
    if (benchmark_type == "matrix") {
        return loadMatrixSpecification(file_name);
    }
    srand(time(0));
    std::string temp_file = std::to_string(rand()) + ".json";
    std::string command = "python3 " + config::KParserMainPath + " " + file_name + " " + benchmark_type + " " + temp_file;
    system(command.c_str());
    auto* spec = new Specification(temp_file);
    system(("rm " + temp_file).c_str());
    return spec;
}

