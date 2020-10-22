#include "util.h"
#include <fstream>
#include <sstream>
#include "config.h"

std::string util::loadStringFromFile(std::string file_name) {
    std::ifstream inp(file_name, std::ios::out);
    std::stringstream buf;
    buf << inp.rdbuf();
    inp.close();
    return buf.str();
}

Type util::string2Type(std::string name) {
    if (name == "Int") return TINT;
    if (name == "String") return TSTRING;
    if (name == "Bool") return TBOOL;
    if (name == "Matrix") return TMATRIX;
    assert(0);
}

Data util::parseDataFromJson(Json::Value node) {
    auto value_type = string2Type(node["value_type"].asString());
    switch (value_type) {
        case TINT:
            return Data(new IntValue(node["value"].asInt()));
        case TBOOL:
            return Data(new BoolValue(node["value"].asString() == "True" || node["value"].asString() == "true"));
        case TSTRING:
            return Data(new StringValue(node["value"].asString()));
    }
}

Json::Value util::loadJsonFromFile(std::string file_name) {
    Json::Reader reader;
    Json::Value root;

    std::string json_string = util::loadStringFromFile(file_name);
    assert(reader.parse(json_string, root));
    return root;
}

std::string util::dataList2String(const DataList &data_list) {
    if (data_list.size() == 0) return "{}";
    std::string ans = "{";
    for (auto& data: data_list) {
        ans += data.toString() + ",";
    }
    ans[ans.length() - 1] = '}';
    return ans;
}

std::string util::type2String(Type type) {
    switch (type) {
        case TINT: return "Int";
        case TBOOL: return "Bool";
        case TSTRING: return "String";
        case TMATRIX: return "Matrix";
    }
}

std::string util::getStringConstType(std::string value) {
    if (global::string_info->const_cache.count(value)) return global::string_info->const_cache[value];
    int inp_num = 0;
    int oup_num = 0;
    int total = global::string_info->example_space.size();
    for (auto* example: global::string_info->example_space) {
        for (auto& param: example->inp) {
            if (param.getType() == TSTRING && param.getString().find(value) != std::string::npos) {
                ++inp_num;
                break;
            }
        }
        if (example->oup.getType() == TSTRING && example->oup.getString().find(value) != std::string::npos) {
            ++oup_num;
        }
    }
    std::string result;
    /*if (inp_num == total && oup_num == total) result = "AllInOutput";
    else if (inp_num == total) result = "AllInput";
    else if (oup_num == total) result = "AllOutput";
    else*/ if (inp_num > 0 && oup_num > 0) result = "SomeInOutput";
    else if (inp_num > 0) result = "SomeInput";
    else if (oup_num > 0) result = "SomeOutput";
    else result = "None";
    result = "Constant@" + result;
    return global::string_info->const_cache[value] = result;
}

bool util::checkInOupList(const Data &value, const DataList &oup) {
    if (oup.size() == 0) return true;
    if (value.getType() == TINT && oup.size() == 2) {
        int l = oup[0].getInt(), r = oup[1].getInt();
#ifdef DEBUG
        assert(l <= r);
#endif
        int now = value.getInt();
        return l <= now && now <= r;
    }
    for (auto& oup_value: oup) {
        if (value == oup_value) return true;
    }
    return false;
}