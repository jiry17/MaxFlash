#include "semantics_factory.h"
#include "config.h"
#include "matrix_operator.h"

#include <unordered_set>

namespace {
    std::map<std::string, Semantics *> semantics_map = {
            {"=", new IntEq()},
            {"str.++", new StringAdd()},
            {"str.replace", new StringReplace()},
            {"str.at", new StringAt()},
            {"int.to.str", new IntToString()},
            {"str.substr", new StringSubstr()},
            {"+", new IntAdd()},
            {"-", new IntMinus()},
            {"str.len", new StringLen()},
            {"str.to.int", new StringToInt()},
            {"str.indexof", new StringIndexOf()},
            {"str.prefixof", new StringPrefixOf()},
            {"str.suffixof", new StringSuffixOf()},
            {"str.contains", new StringContains()},
            {"ite", new StringIte()},
            {"Reshape", new ReshapeSemantics()},
            {"Permute", new PermuteSemantics()},
            {"Var", new MatrixIDSemantics()},
            {"Fliplr", new FliplrSemantics()},
            {"Flipud", new FlipudSemantics()},
            {"B", new VectorInitSemantics()},
            {"L", new VectorConcatSemantics()}
    };

    bool checkInList(const DataList& data_list, const Data& data) {
        for (auto& data_2: data_list) {
            if (data == data_2) return true;
        }
        return false;
    }

    std::pair<int, int> datalistToRange(const DataList& inp) {
        if (inp.size() == 0) return std::make_pair(global::KIntMin, global::KIntMax);
        if (inp.size() == 1) {
            int x = inp[0].getInt();
            return std::make_pair(x, x);
        }
        int l = inp[0].getInt(), r = inp[1].getInt();
#ifdef DEBUG
        assert(inp.size() == 2 && l <= r);
#endif
        return std::make_pair(l, r);
    }

    int getLastOccur(const std::string& s, const std::string& t, int r) {
        auto i = s.find(t);
        if (i == std::string::npos || i >= r) return global::KIntMin;
        while (1) {
            auto ne = s.find(t, i + 1);
            if (ne == std::string::npos || ne >= r) {
                return int(i) + 1;
            }
            i = ne;
        }
    }
}

Semantics* string2Semantics(std::string name) {
#ifdef DEBUG
    assert(semantics_map.count(name) > 0);
#endif
    return semantics_map[name];
}

WitnessList StringAdd::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    std::string s = oup[0].getString();
    int n = s.length();
    WitnessList result(n + 1);
    std::string now;
    for (int i = 0; i < n; ++i) {
        result[i].push_back({Data(new StringValue(now))});
        now += s[i];
    }
    result[n].push_back({Data(new StringValue(now))});
    now = "";
    result[n].push_back({Data(new StringValue(now))});
    for (int i = n - 1; i >= 0; --i) {
        now = s[i] + now;
        result[i].push_back({Data(new StringValue(now))});
    }
    return result;
}

bool StringReplace::isSubSequence(std::string s, std::string t) {
    int now = 0;
    for (int i = 0; i < t.length() && now < s.length(); ++i) {
        if (t[i] == s[now]) {
            now++;
        }
    }
    return now == s.length();
}

bool StringReplace::valid(std::string res, std::string s, std::string t, StringInfo *info) {
    bool is_contain = false;
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() != TSTRING) continue;
        if (isSubSequence(res, (*info)[i].getString())) {
            is_contain = true;
        }
    }
    if (!is_contain) return false;
    DataList inp;
    inp.push_back(Data(new StringValue(res)));
    inp.push_back(Data(new StringValue(s)));
    inp.push_back(Data(new StringValue("")));
    if (run(inp, info).getString() != t) return false;
    return true;
}

void StringReplace::searchForAllMaximam(int pos, std::string res, std::string s, std::string t, WitnessList &result, StringInfo *info) {
    bool is_end = true;
    for (int i = pos; i < res.length(); ++i) {
        std::string now = res.substr(0, i) + s + res.substr(i, res.length());
        if (valid(now, s, t, info)) {
            is_end = false;
            searchForAllMaximam(pos + s.length(), now, s, t, result, info);
        }
    }
    if (is_end) {
        result.push_back({{Data(new StringValue(res))}, {Data(new StringValue(s))}, {Data(new StringValue(""))}});
    }
}

WitnessList StringReplace::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.empty()) {
        return {{{}, {}, {}}};
    }
    auto* string_info = dynamic_cast<StringInfo*>(global_info);
#ifdef DEBUG
    assert(oup.size() == 1 && string_info != nullptr);
#endif
    std::string oup_value = oup[0].getString();
    DataList inp;
    inp.push_back(oup[0]);
    WitnessList result;
    bool is_contain_empty = false;
    for (auto& s_data: string_info->const_list) {
        if (s_data.getString().length() == 0) {
            is_contain_empty = true;
            continue;
        }
        inp.push_back(s_data);
        for (auto& t_data: string_info->const_list) {
            if (s_data.getString() == t_data.getString() || t_data.getString().length() == 0) continue;
            inp.push_back(t_data);
            Data previous = run(inp, global_info);
            DataList inp2({previous, t_data, s_data});
            if (run(inp2, global_info) == oup[0]) {
                result.push_back({{previous}, {t_data}, {s_data}});
            }
            inp.pop_back();
        }
        inp.pop_back();
    }
    if (is_contain_empty && oup[0].getString().length() > 5) {
        for (auto &s_data: string_info->const_list) {
            if (s_data.getString().length() == 0 ||
                oup[0].getString().find(s_data.getString()) != std::string::npos)
                continue;
            searchForAllMaximam(0, oup[0].getString(), s_data.getString(), oup[0].getString(), result, string_info);
        }
    }
    return result;
}

WitnessList StringAt::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.empty()) {
        return {{{}, {Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))}}};
    }
    auto* info = dynamic_cast<StringInfo*>(global_info);
#ifdef DEBUG
    assert(oup.size() == 1 && info != nullptr);
#endif
    if (oup[0].getString().length() > 1) return {};
    // Assume the first input can only be constants or params
    char t = oup[0].getString()[0];
    WitnessList result;
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() != TSTRING) continue;
        std::string s = (*info)[i].getString();
        for (int j = 0; j < s.length(); ++j) {
            if (s[j] == t) result.push_back({{Data(new StringValue(s))}, {Data(new IntValue(j))}});
        }
    }
    for (auto& const_data: info->const_list) {
        std::string s = const_data.getString();
        for (int j = 0; j < s.length(); ++j) {
            if (s[j] == t) result.push_back({{const_data}, {Data(new IntValue(j))}});
        }
    }
    return result;
}

WitnessList IntToString::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.empty()) {
        return {{{Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))}}};
    }
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    std::string result = oup[0].getString();
    if (result.length() == 0 || result.length() >= 8) return {};
    if (result.length() > 0 && result[0] == '0') return {};
    for (char c: result) {
        //TODO complete it
        if (!isdigit(c)) return {};
    }
    int int_value = std::stoi(result);
    return {{{Data(new IntValue(int_value))}}};
}

void StringSubstr::getAllChoice(std::string s, std::string t, WitnessList &result) {
    if (t.length() > s.length()) return;
    if (t.length() == 0) {
        result.push_back({{Data(new StringValue(s))},
                          {Data(new IntValue(global::KIntMin)), Data(new IntValue(-1))}, {}});
        result.push_back({{Data(new StringValue(s))}, {},
                          {Data(new IntValue(global::KIntMin)), Data(new IntValue(0))}});
        if (s.length() <= global::KIntMax) {
            result.push_back({{Data(new StringValue(s))},
                              {Data(new IntValue(s.length())), Data(new IntValue(global::KIntMax))}, {}});
        }
        return;
    }
    int n = s.length();
    int m = t.length();
    for (auto i = s.find(t); i != std::string::npos; i = s.find(t, i + 1)) {
        if (i != n - m) {
            result.push_back({{Data(new StringValue(s))}, {Data(new IntValue(i))}, {Data(new IntValue(m))}});
        } else {
            result.push_back({{Data(new StringValue(s))}, {Data(new IntValue(i))}, {Data(new IntValue(m)), Data(new IntValue(global::KIntMax))}});
        }
    }
}

WitnessList StringSubstr::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))},
                 {Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))}}};
    }
    auto* info = dynamic_cast<StringInfo*>(global_info);
#ifdef DEBUG
    assert(oup.size() == 1 && info != nullptr);
#endif
    std::string oup_value = oup[0].getString();
    WitnessList result;
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() != TSTRING) continue;
        std::string param_value = (*info)[i].getString();
        getAllChoice(param_value, oup_value, result);
    }
    return result;
}

WitnessList IntAdd::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    if (oup.size() == 1) {
        int oup_value = oup[0].getInt();
        WitnessList result;
        for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
            int j = oup_value - i;
            if (j >= global::KIntMin && j <= i) {
                result.push_back({{Data(new IntValue(i))},
                                  {Data(new IntValue(j))}});
            }
        }
        return result;
    }
    int l = oup[0].getInt(), r = oup[1].getInt();
#ifdef DEBUG
    assert(oup.size() == 2 && l <= r);
#endif
    WitnessList result;
    for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
        int new_l = l - i, new_r = r - i;
        new_l = std::max(new_l, global::KIntMin);
        new_r = std::min(new_r, global::KIntMax);
        if (new_l <= new_r) {
            result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(new_l)), Data(new IntValue(new_r))}});
        }
    }
    return result;
}

WitnessList IntMinus::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    if (oup.size() == 1) {
        int oup_value = oup[0].getInt();
        WitnessList result;
        for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
            int j = i - oup_value;
            if (j >= global::KIntMin && j <= global::KIntMax) {
                result.push_back({{Data(new IntValue(i))},
                                  {Data(new IntValue(j))}});
            }
        }
        return result;
    }
    int l = oup[0].getInt();
    int r = oup[1].getInt();
#ifdef DEBUG
    assert(l <= r && oup.size() == 2);
#endif
    WitnessList result;
    for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
        int new_l = i - r, new_r = i - l;
        new_l = std::max(new_l, global::KIntMin);
        new_r = std::min(new_r, global::KIntMax);
        if (new_l <= new_r) {
            result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(new_l)), Data(new IntValue(new_r))}});
        }
    }
    return result;
}

WitnessList StringLen::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    auto* param_info = dynamic_cast<ParamInfo*>(global_info);
    std::pair<int, int> res = datalistToRange(oup);
    int l = res.first, r = res.second;
    //TODO: be more generalize
    WitnessList result;
    for (int i = 0; i < param_info->size(); ++i) {
        if ((*param_info)[i].getType() != TSTRING) continue;
        int current_len = (*param_info)[i].getString().length();
        if (current_len >= l && current_len <= r) {
            result.push_back({{(*param_info)[i]}});
        }
    }
    return result;
}

WitnessList StringToInt::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    auto res = datalistToRange(oup);
    int l = res.first, r = res.second;
    l = std::max(l, 0);
    WitnessList result;
    for (int i = l; i <= r; ++i) {
        result.push_back({{Data(new StringValue(std::to_string(i)))}});
    }
    return result;
}

WitnessList StringIndexOf::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    auto* string_info = dynamic_cast<StringInfo*>(global_info);
#ifdef DEBUG
    assert(string_info != nullptr);
#endif
    auto res = datalistToRange(oup);
    WitnessList result;
    int l = std::max(-1, res.first);
    int r = res.second;
    if (l > r) return {};
    for (int i = 0; i < string_info->size(); ++i) {
        if ((*string_info)[i].getType() != TSTRING) continue;
        std::string s = (*string_info)[i].getString();
        if (l == -1) {
            for (auto const_str: string_info->const_set) {
                int l = getLastOccur(s, const_str, s.length());
                if (l <= global::KIntMin) {
                    result.push_back({{Data(new StringValue(s))},
                                      {Data(new StringValue(const_str))},
                                      {Data(new IntValue(l)), Data(new IntValue(global::KIntMax))}});
                }
            }
        }
        for (int pos = std::max(0, l); pos <= std::min(r, int(s.length())); ++pos) {
            std::string now;
            now += s[pos];
            result.push_back({{Data(new StringValue(s))}, {Data(new StringValue(now))},
                              {Data(new IntValue(getLastOccur(s, now, pos))), Data(new IntValue(pos))}});
            for (int j = pos + 1; j < s.length(); ++j) {
                now += s[j];
                if (string_info->const_set.find(now) != string_info->const_set.end()) {
                    result.push_back({{Data(new StringValue(s))}, {Data(new StringValue(now))},
                                      {Data(new IntValue(getLastOccur(s, now, pos))), Data(new IntValue(pos))}});
                }
            }
        }
    }
    return result;
}

WitnessList StringPrefixOf::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    auto* info = dynamic_cast<StringInfo*>(global_info);
    std::vector<Data> possible_list;
    for (auto& s: info->const_list) {
        if (s.getType() == TSTRING) possible_list.push_back(s);
    }
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() == TSTRING) possible_list.push_back((*info)[i]);
    }
    WitnessList result;
    bool target = oup[0].getBool();
    for (int i = 0; i < possible_list.size(); ++i) {
        for (int j = 0; j < possible_list.size(); ++j) {
            std::string s = possible_list[i].getString();
            std::string t = possible_list[j].getString();
            if (target == (s.length() <= t.length() && t.substr(0, s.length()) == s)) {
                result.push_back({{possible_list[i]}, {possible_list[j]}});
            }
        }
    }
    return result;
}

WitnessList StringSuffixOf::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    auto* info = dynamic_cast<StringInfo*>(global_info);
    std::vector<Data> possible_list;
    for (auto& s: info->const_list) {
        if (s.getType() == TSTRING) possible_list.push_back(s);
    }
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() == TSTRING) possible_list.push_back((*info)[i]);
    }
    WitnessList result;
    bool target = oup[0].getBool();
    for (int i = 0; i < possible_list.size(); ++i) {
        for (int j = 0; j < possible_list.size(); ++j) {
            std::string s = possible_list[i].getString();
            std::string t = possible_list[j].getString();
            if (target == (s.length() <= t.length() && t.substr(t.length() - s.length(), s.length()) == s)) {
                result.push_back({{possible_list[i]}, {possible_list[j]}});
            }
        }
    }
    return result;
}

WitnessList StringContains::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
#ifdef DEBUG
    assert(oup.size() == 1);
#endif
    auto* info = dynamic_cast<StringInfo*>(global_info);
    std::vector<Data> possible_list;
    for (auto& s: info->const_list) {
        if (s.getType() == TSTRING) possible_list.push_back(s);
    }
    for (int i = 0; i < info->size(); ++i) {
        if ((*info)[i].getType() == TSTRING) possible_list.push_back((*info)[i]);
    }
    WitnessList result;
    bool target = oup[0].getBool();
    for (int i = 0; i < possible_list.size(); ++i) {
        for (int j = 0; j < possible_list.size(); ++j) {
            std::string s = possible_list[i].getString();
            std::string t = possible_list[j].getString();
            if (target == (s.find(t) != std::string::npos)) {
                result.push_back({{possible_list[i]}, {possible_list[j]}});
            }
        }
    }
    return result;
}

WitnessList IntEq::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}}};
    }
    WitnessList result;
    if (oup[0].getBool()) {
        for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
            result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(i))}});
        }
    } else {
        for (int i = global::KIntMin; i <= global::KIntMax; ++i) {
            if (i > global::KIntMin) {
                if (i == global::KIntMin + 1) {
                    result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(global::KIntMin))}});
                } else {
                    result.push_back({{Data(new IntValue(i))},
                                      {Data(new IntValue(global::KIntMin)), Data(new IntValue(i - 1))}});
                }
            }
            if (i < global::KIntMax) {
                if (i == global::KIntMax - 1) {
                    result.push_back({{Data(new IntValue(i))}, {Data(new IntValue(global::KIntMax))}});
                } else {
                    result.push_back({{Data(new IntValue(i))},
                                      {Data(new IntValue(i + 1)), Data(new IntValue(global::KIntMax))}});
                }
            }
        }
    }
    return result;
}

WitnessList IntIte::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{Data(new BoolValue(true))}, {}, {}}, {{Data(new BoolValue(false))}, {}, {}}};
    }
    return {{{Data(new BoolValue(true))}, oup, {Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))}},
            {{Data(new BoolValue(false))}, {Data(new IntValue(global::KIntMin)), Data(new IntValue(global::KIntMax))}, oup}};
}

WitnessList StringIte::witnessFunction(const DataList &oup, GlobalInfo *global_info) {
    if (oup.size() == 0) {
        return {{{}, {}, {}}};
    }
    return {{{Data(new BoolValue(true))}, oup, {}}, {{Data(new BoolValue(false))}, {}, oup}};
}