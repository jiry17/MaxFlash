#ifndef L2S_SEMANTICS_FACTORY_H
#define L2S_SEMANTICS_FACTORY_H

#include "semantics.h"
#include "config.h"

#include <map>

// A map translating the name of an operator into an object of "Semantics".
// If a new operator is used, it must be registered in this function.
extern Semantics* string2Semantics(std::string name);

class StringAdd: public Semantics {
public:
    StringAdd(): Semantics({TSTRING, TSTRING}, TSTRING, "str.++") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new StringValue(input_list[0].getString() + input_list[1].getString()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringAt: public Semantics {
public:
    StringAt(): Semantics({TSTRING, TINT}, TSTRING, "str.at") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        int pos = input_list[1].getInt();
        std::string s = input_list[0].getString();
        if (pos < 0 || pos >= s.length()) return Data(new StringValue(""));
        char result[2] = {s[pos], 0};
        return Data(new StringValue(std::string(result)));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class IntToString: public Semantics {
public:
    IntToString(): Semantics({TINT}, TSTRING, "int.to.str") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new StringValue(std::to_string(input_list[0].getInt())));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringSubstr: public Semantics {
    void getAllChoice(std::string s, std::string t, WitnessList& result);
public:
    StringSubstr(): Semantics({TSTRING, TINT, TINT}, TSTRING, "str.substr") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        int pos = input_list[1].getInt();
        std::string s = input_list[0].getString();
        if (pos < 0 || pos >= s.length() || input_list[2].getInt() < 0) return Data(new StringValue(""));
        return Data(new StringValue(s.substr(pos, input_list[2].getInt())));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringReplace: public Semantics {
    bool isSubSequence(std::string s, std::string t);
    bool valid(std::string res, std::string s, std::string t, StringInfo* info);
    void searchForAllMaximam(int pos, std::string res, std::string s, std::string t, WitnessList& result, StringInfo* info);
public:
    StringReplace(): Semantics({TSTRING, TSTRING, TSTRING}, TSTRING, "str.replace") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        std::string result = input_list[0].getString();
        std::string s = input_list[1].getString();
        std::string t = input_list[2].getString();
        std::vector<int> occur;
        for (auto i = result.find(s, 0); i != std::string::npos; i = result.find(s, i + t.length())) {
            result.replace(i, s.length(), t);
        }
        return Data(new StringValue(result));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class IntAdd: public Semantics {
public:
    IntAdd(): Semantics({TINT, TINT}, TINT, "+") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new IntValue(input_list[0].getInt() + input_list[1].getInt()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class IntMinus: public Semantics {
public:
    IntMinus(): Semantics({TINT, TINT}, TINT, "-") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new IntValue(input_list[0].getInt() - input_list[1].getInt()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class IntEq: public Semantics {
public:
    IntEq(): Semantics({TINT, TINT}, TBOOL, "=") {}

    virtual Data run(const DataList& input_list, GlobalInfo* gloabl_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new BoolValue(input_list[0].getInt() == input_list[1].getInt()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringLen: public Semantics {
public:
    StringLen(): Semantics({TSTRING}, TINT, "str.len") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        return Data(new IntValue(input_list[0].getString().length()));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringToInt: public Semantics {
public:
    StringToInt(): Semantics({TSTRING}, TINT, "str.to.int") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        //TODO: The case when the input string is not a number.
        return Data(new IntValue(std::atoi(input_list[0].getString().c_str())));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringIndexOf: public Semantics {
public:
    StringIndexOf(): Semantics({TSTRING, TSTRING, TINT}, TINT, "str.indexof") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        auto result = input_list[0].getString().find(input_list[1].getString(), std::max(0, input_list[2].getInt()));
        if (result == std::string::npos) {
            return Data(new IntValue(-1));
        } else {
            return Data(new IntValue(result));
        }
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringPrefixOf: public Semantics {
public:
    StringPrefixOf(): Semantics({TSTRING, TSTRING}, TBOOL, "str.prefixof") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        std::string s = input_list[0].getString();
        std::string t = input_list[1].getString();
        bool result = true;
        if (t.length() < s.length()) result = false;
        else {
            for (int i = 0; i < s.length(); ++i) {
                if (s[i] != t[i]) {
                    result = false;
                    break;
                }
            }
        }
        return Data(new BoolValue(result));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringSuffixOf: public Semantics {
public:
    StringSuffixOf(): Semantics({TSTRING, TSTRING}, TBOOL, "str.suffixof") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        std::string s = input_list[0].getString();
        std::string t = input_list[1].getString();
        bool result = true;
        if (t.length() < s.length()) result = false;
        else {
            for (int i = 0, j = t.length() - s.length(); i < s.length(); ++i, ++j) {
                if (t[j] != s[i]) {
                    result = false;
                    break;
                }
            }
        }
        return Data(new BoolValue(result));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringContains: public Semantics {
public:
    StringContains(): Semantics({TSTRING, TSTRING}, TBOOL, "str.contains") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        std::string s = input_list[0].getString();
        std::string t = input_list[1].getString();
        return Data(new BoolValue(s.find(t) != std::string::npos));
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class IntIte: public Semantics {
public:
    IntIte(): Semantics({TBOOL, TINT, TINT}, TINT, "ite") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        if (input_list[0].getBool()) {
            return input_list[1];
        } else {
            return input_list[2];
        }
    }

    virtual WitnessList witnessFunction(const DataList& oup, GlobalInfo* global_info);
};

class StringIte: public Semantics {
public:
    StringIte(): Semantics({TBOOL, TSTRING, TSTRING}, TSTRING, "ite") {}

    virtual Data run(const DataList& input_list, GlobalInfo* global_info) {
#ifdef DEBUG
        check(input_list);
#endif
        if (input_list[0].getBool()) {
            return input_list[1];
        } else {
            return input_list[2];
        }
    }

    virtual WitnessList witnessFunction(const DataList &oup, GlobalInfo* global_info);
};
#endif //L2S_SEMANTICS_FACTORY_H
