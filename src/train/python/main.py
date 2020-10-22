from sexp import sexp as sexpParser
import os
import json
import sys
import pickle as pkl
import pprint as pp
from enum import IntEnum

benchmark_type = None

# --------------------------------------------------------------------------
# Simple functions dealing with SyGuS benchmarks

def strip_comments(bmFile):
    no_comments = '('
    for line in bmFile:
        line = line.split(';', 1)[0]
        no_comments += line
    return no_comments + ")"

def sexp_from_string(string):
    return sexpParser.parseString(string, parseAll=True).asList()[0]

def load_cache():
    if not os.path.exists("parse_cache.pkl"):
        return {}
    with open("parse_cache.pkl", "rb") as inp:
        return pkl.load(inp)

def save_cache(result):
    with open("parse_cache.pkl", "wb") as oup:
        pkl.dump(result, oup)

def sexp_from_file(file_name):
    with open(file_name, "r") as inp:
        bm = strip_comments(inp)
        bm_expr = sexp_from_string(bm)
    return bm_expr

def filter_by_name(bm_expr, name):
    return list(filter(lambda x: x[0] == name, bm_expr))

def get_name_from_func_info(func_info):
    return func_info[1]

def get_params_from_func_info(func_info):
    return func_info[2]

def get_return_type_from_func_info(func_info):
    return func_info[3]

def get_prog_from_func_info(func_info):
    return func_info[4]

def check_in(constant, s):
    if s[0] != "String": return False
    return constant in s[1]

def get_example_from_spec(bm_expr):
    constraint_list = filter_by_name(bm_expr, "constraint")
    example_list = []
    for _, constraint in constraint_list:
        assert constraint[0] == "=" and len(constraint) == 3
        inp = constraint[1][1:]
        oup = constraint[2]
        example_list.append([inp, oup])
    return example_list

# --------------------------------------------------------------------------

# Abstract a constant according to its relationship with examples.
def get_abstracted_type(constant, example_list):
    inp_num = 0
    oup_num = 0
    total = len(example_list)
    for inp, oup in example_list:
        for inp_string in inp:
            if check_in(constant, inp_string):
                inp_num += 1
                break
        if check_in(constant, oup): oup_num += 1
    if inp_num > 0 and oup_num > 0: return "SomeInOutput"
    if inp_num > 0: return "SomeInput"
    if oup_num > 0: return "SomeOutput"
    return "None"

# Replace all constants and parameters in a program with abstract symbols.
def get_abstracted_program(prog, param_list, example_list):
    if type(prog) == str:
        for (param_name, param_type) in param_list:
            if param_name == prog:
                return "Param@" + param_type
        return prog
    if type(prog) == tuple:
        if example_list is None or prog[0] != "String":
            return "Constant@" + prog[0]
        else:
            return "Constant@" + get_abstracted_type(prog[1], example_list)
    if type(prog) == list:
        return list(map(lambda sublist: get_abstracted_program(sublist, param_list, example_list), prog))
    assert False

# Topdown context model
class Model:
    def __init__(self, prog, info_map):
        self.prog = prog
        self.info_map = {}
        for ctxt, info in info_map.items():
            probability_sum = 0
            for op, value in info.items():
                probability_sum += value
            new_info = {}
            for op, value in info.items():
                new_info[op] = value / probability_sum
            self.info_map[ctxt] = new_info

    def query(self, ctxt):
        ctxt_str = ",".join(ctxt)
        assert ctxt_str in self.info_map
        return self.info_map[ctxt_str]

model_map = {}

# Deal with matrix benchmarks
def get_matrix_data_info(file_name):
    with open(file_name, "r") as inp:
        info = inp.readlines()
    assert len(info) >= 5 and "// desired program" in info[3]
    code = info[4][:-1]

    def normalize(code):
        while code[0] == ' ': code = code[1:]
        while code[-1] == ' ': code = code[:-1]
        return code

    def parse_code(code):
        code = normalize(code)
        if "(" not in code:
            if code == "x":
                return "Param@Matrix"
            return "Constant@Int"
        assert code[-1] == ')'
        pos = code.find('(')
        operator = normalize(code[:pos])
        param = normalize(code[pos + 1: -1])
        pre_pos = 0
        left_num = 0
        param_list = []
        param = param + ","
        for i, ch in enumerate(param):
            if ch == '(': left_num += 1
            if ch == ')':
                left_num -= 1
            if ch == ',' and left_num == 0:
                param_list.append(normalize(param[pre_pos:i]))
                pre_pos = i + 1
        result = [operator]
        for param_code in param_list:
            result.append(parse_code(param_code))
        return result

    result = parse_code(code)
    return [result]

# Extract the correct program from benchmarks
def get_data_info(file_name):
    global parse_cache, benchmark_type
    if file_name in parse_cache:
        return parse_cache[file_name]
    if benchmark_type == "matrix":
        result = get_matrix_data_info(file_name)
        parse_cache[file_name] = result
        save_cache(parse_cache)
        return result
    bm_expr = sexp_from_file(file_name)
    def_func = filter_by_name(bm_expr, "define-fun")

    if len(def_func) == 0:
        result = []
    else:
        assert len(def_func) == 1
        func_info = def_func[0]
        prog = get_prog_from_func_info(func_info)
        param_list = get_params_from_func_info(func_info)
        example_list = get_example_from_spec(bm_expr)

        synth_info = filter_by_name(bm_expr, "synth-fun")
        assert len(synth_info) == 1
        result = [get_abstracted_program(prog, param_list, example_list)]
    parse_cache[file_name] = result
    save_cache(parse_cache)
    return result

def encode_context(context):
    return "{" + ",".join(list(map(str, context))) + "}"

def decode_context(context_string):
    return context_string[1:-1].split(",")

# Train an n-gram model from a set of benchmarks
def train_model(program_list, lookup_depth):
    model = {}
    context = [None for _ in range(lookup_depth)]

    def count_combine_in_program(program, path):
        if lookup_depth > 0:
            context_string = encode_context(context[-lookup_depth:])
        else:
            context_string = ""
        if context_string not in model:
            model[context_string] = {}
        if type(program) == str:
            current_name = program
        else:
            assert type(program) == list
            current_name = program[0]
            for i in range(1, len(program)):
                context.append(current_name + "@" + str(i))
                path.append(i - 1)
                count_combine_in_program(program[i], path)
                path.pop()
                context.pop()
        if current_name not in model[context_string]:
            model[context_string][current_name] = 0
        model[context_string][current_name] += 1

    for program in program_list:
        count_combine_in_program(program, [])

    for context in model:
        total = 0
        for _, value in model[context].items():
            total += value
        for possible_result in model[context]:
            model[context][possible_result] /= 1.0 * total
    return model

def print_model(model):
    for context in model:
        results = []
        for possible_result, probability in model[context].items():
            results.append([probability, possible_result])
        results = sorted(results, reverse=True)
        result_string = ", ".join(list(map(lambda x: x[1] + ": {0:2f}".format(x[0]), results)))
        print(context + " => " + result_string)

def save_model(model, file_name):
    json_dict = []
    for context_string in model:
        context = decode_context(context_string)
        current_info = {"context": context, "rule": []}
        for possible_term, probability in model[context_string].items():
            current_info["rule"].append({"term": possible_term, "p": probability})
        json_dict.append(current_info)
    with open(file_name, "w") as oup:
        json.dump(json_dict, fp=oup, indent=4)

def get_data_list(train_path):
    result = []
    file_name_list = os.listdir(train_path)
    for file_name in file_name_list:
        if ".sl" in file_name or benchmark_type == "matrix":
            result.append(train_path + file_name)
    return result

if __name__ == "__main__":
    #sys.argv = [None, "/Users/pro/Desktop/work/2020A/MaxFlash/benchmark/matrix", 1, "/Users/pro/Desktop/work/2020A/MaxFlash/", "Y", "matrix"]
    assert len(sys.argv) == 6
    # The path of the dataset
    data_path = sys.argv[1] + "/"
    # The number of ancestors in the context
    default_depth = int(sys.argv[2])
    # The path of the folder containing all models
    oup_dict = sys.argv[3]
    # Whether to clear the cache of benchmarks ('N' only for accelerating the cross validation)
    is_clear = (sys.argv[4] == "Y")
    # The type of the benchmark (string / matrix)
    benchmark_type = sys.argv[5]
    assert benchmark_type in ["string", "matrix"]

    if is_clear and os.path.exists("parse_cache.pkl"):
        os.system("rm parse_cache.pkl")

    parse_cache = load_cache()
    data_list = get_data_list(data_path)
    program_list = []
    for (i, file_name) in enumerate(data_list):
        prog = get_data_info(file_name)
        program_list += prog

    model = train_model(program_list, default_depth)
    save_model(model, oup_dict + "/model@depth" + str(default_depth) + ".json")

    if sys.argv[4] == 'C' and os.path.exists("parse_cache.pkl"):
        os.system("rm parse_cache.pkl")
