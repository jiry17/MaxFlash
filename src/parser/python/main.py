# Convert an SyGuS benchmark into a Json file.

import sys
from sexp import sexp as sexpParser
from enum import Enum
import json

# ---------------------------------------------------------------------------
# Util functions for parsing SyGuS benchmarks

def stripComments(bmFile):
    noComments = '('
    for line in bmFile:
        line = line.split(';', 1)[0]
        noComments += line
    return noComments + ')'

def sexpFromString(value):
    return sexpParser.parseString(value, parseAll = True).asList()[0]

def sexpFromFile(benchmarkFileName):
    try:
        benchmarkFile = open(benchmarkFileName)
    except:
        return None

    bm = stripComments(benchmarkFile)
    bmExpr = sexpFromString(bm)
    benchmarkFile.close()
    return bmExpr

def getSublist(bmExpr, name):
    return list(filter(lambda x: x[0] == name, bmExpr))

def getName(func_info): return func_info[1]
def getParamType(func_info): return list(map(lambda x: x[1], func_info[2]))
def getReturnType(func_info): return func_info[3]
def getSpec(func_info): return func_info[4]

def substituteParam(func, param_list):
    if type(func) == str:
        for (i, (name, _)) in enumerate(param_list):
            if name == func:
                return "Param", i
        return func
    elif type(func) == list:
        return list(map(lambda x: substituteParam(x, param_list), func))
    else:
        return func

def lookup_global_var(var_list, var_name):
    for var_info in var_list:
        if var_info["name"] == var_name:
            return var_info["type"]
    return None

def get_temp_name(global_var):
    for var_id in range(0, 1000):
        current_name = "temp" + str(var_id)
        if lookup_global_var(global_var, current_name) is None:
            return current_name
    assert False

def replace_params(expr, param_list, param_value):
    if type(expr) == str:
        for (i, (param_name, param_type)) in enumerate(param_list):
            if param_name == expr:
                return param_value[i]
        return expr
    if type(expr) == tuple:
        return expr
    return list(map(lambda x: replace_params(x, param_list, param_value), expr))
# ------------------------------------------------------------------------------------

def functionInfo2Json(function_info):
    param_list = function_info[2]
    function_info_json = {}
    param_json = []
    for (param_name, param_type) in param_list:
        param_json.append({"name": param_name, "type": param_type})
    function_info_json["param"] = param_json
    function_info_json["return_type"] = function_info[3]
    non_terminal_json_list = []
    for (non_terminal, non_terminal_type, rule_list) in function_info[4]:
        non_terminal_json = {"name": non_terminal, "type": non_terminal_type}
        rule_root = []
        for rule in rule_list:
            if type(rule) == list:
                rule_root.append({"type": "expr", "operator": rule[0], "param": rule[1:]})
            elif type(rule) == tuple:
                rule_root.append({"type": "const", "value": rule[1], "value_type": rule[0]})
            elif type(rule) == str:
                var_type = None
                for var_info in param_json:
                    if var_info["name"] == rule:
                        var_type = var_info["type"]
                assert var_type is not None
                rule_root.append({"type": "var", "var_name": rule, "var_type": var_type})
        non_terminal_json["rule"] = rule_root
        non_terminal_json_list.append(non_terminal_json)
    function_info_json["non_terminal"] = non_terminal_json_list
    return function_info_json

def replace_function(expr, defined_functions, spec_info, global_var, result):
    if type(expr) == str:
        var_type = lookup_global_var(global_var, expr)
        assert var_type is not None
        return {"type": "var", "var_name": expr, "var_type": var_type}
    if type(expr) == tuple:
        return {"type": "const", "value": expr[1], "value_type": expr[0]}
    '''
    (define-fun Spec ((k1 Bool) (k2 Bool) (k3 Bool) (r1 Bool) (r2 Bool) (r3 Bool) (r4 Bool)) Bool
    )
    '''
    if type(expr) == list:
        function_call = expr[0]
        for function_info in defined_functions:
            if function_info[1] == function_call:
                var_name = get_temp_name(global_var)
                var_type = function_info[3]
                global_var.append({"name": var_name, "type": var_type})
                new_expr = ["=", var_name,
                            replace_params(function_info[4], function_info[2], expr[1:])]
                result.append(replace_function(new_expr, defined_functions, spec_info, global_var, result))
                return {"type": "var", "var_name": var_name, "var_type": var_type}
        sub_expr = list(map(lambda x: replace_function(x, defined_functions, spec_info,
                                                       global_var, result), expr[1:]))
        if function_call == getName(spec_info):
            formalized_sub_expr = []
            for (i, param_expr) in enumerate(sub_expr):
                if param_expr["type"] != "expr":
                    formalized_sub_expr.append(param_expr)
                else:
                    var_name = get_temp_name(global_var)
                    var_type = getParamType(spec_info)[i]
                    global_var.append({"name": var_name, "type": var_type})
                    new_expr = ["=", var_name, param_expr]
                    formalized_sub_expr.append({"type": "var", "var_name": var_name, "var_type": var_type})
                    result.append(replace_function(new_expr, defined_functions, spec_info, global_var, result))
            return {"type": "function", "params": formalized_sub_expr}
        return {"type": "expr", "operator": function_call, "params": sub_expr}
    assert False

def parseBenchmark(bmExpr):
    '''
    (define-fun Spec ((k1 Bool) (k2 Bool) (k3 Bool) (r1 Bool) (r2 Bool) (r3 Bool) (r4 Bool)) Bool
    )
    '''
    defined_functions = getSublist(bmExpr, "define-fun")
    function_info = getSublist(bmExpr, "synth-fun")
    assert len(function_info) == 1
    function_info = function_info[0]
    result = {"spec": functionInfo2Json(function_info)}
    declared_variables = getSublist(bmExpr, "declare-var")
    global_var = []
    for (_, var_name, var_type) in declared_variables:
        global_var.append({"name": var_name, "type": var_type})
    constraint_list = getSublist(bmExpr, "constraint")
    formalized_constraint = []
    extra_constraint = []
    for constraint in constraint_list:
        replaced_constraint = replace_function(constraint[1], defined_functions,
                                               function_info, global_var, extra_constraint)
        formalized_constraint.append(replaced_constraint)
    result["global_var"] = global_var
    result["constraint"] = formalized_constraint
    result["extra"] = extra_constraint
    return result

def check_valid(rule):
    if type(rule) == list:
        if rule[0] == "ite" and rule[1] == "ntInt": return False
    if type(rule) == tuple:
        if rule[0] == "Int" and abs(rule[1]) > 1: return False
    return True

def formalize_bmExpr(bm_expr):
    function_info = getSublist(bm_expr, "synth-fun")[0]
    result = []
    for sub_list in bm_expr:
        if sub_list[0] != "synth-fun":
            result.append(sub_list)
    new_function_info = function_info[:4]
    rule_list = function_info[4]
    start_rule = rule_list[0]
    true_start_symbol = start_rule[2][0]
    assert start_rule == ["Start", true_start_symbol[2:], [true_start_symbol]]
    string_const = []
    def replace_start(rule_list):
        if type(rule_list) == list:
            result = []
            for sub_list in rule_list:
                if check_valid(sub_list):
                    result.append(replace_start(sub_list))
            return result
        if type(rule_list) == str:
            assert rule_list != "Start"
            if rule_list == true_start_symbol:
                return "Start"
        if type(rule_list) == tuple and rule_list[0] == "String":
            string_const.append(rule_list[1])
        return rule_list
    rule_list = replace_start(rule_list[1:])
    new_rule_list = []
    for symbol_list in rule_list:
        rule_info_list = symbol_list[2]
        new_symbol_list = symbol_list[:2]
        new_symbol_list.append([])
        for rule_info in rule_info_list:
            if rule_info not in new_symbol_list[2]:
                new_symbol_list[2].append(rule_info)
        new_rule_list.append(new_symbol_list)
    new_function_info.append(new_rule_list)
    result.append(new_function_info)

    return result

if __name__ == "__main__":
    assert len(sys.argv) == 4
    file_name = sys.argv[1]
    benchmark_name = sys.argv[2]
    output_file = sys.argv[3]
    result = sexpFromFile(file_name)

    result = formalize_bmExpr(result)
    parse_result = parseBenchmark(result)

    with open(output_file, "w") as oup:
        json.dump(parse_result, fp=oup, indent=4)