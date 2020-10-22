#ifndef L2S_UTIL_H
#define L2S_UTIL_H

#include "json/json.h"
#include "data.h"

namespace util {
    std::string loadStringFromFile(std::string file_name);
    Type string2Type(std::string name);
    Data parseDataFromJson(Json::Value node);
    Json::Value loadJsonFromFile(std::string file_name);
    std::string dataList2String(const DataList& data_list);
    std::string type2String(Type type);
    std::string getStringConstType(std::string s);
    bool checkInOupList(const Data& value, const DataList& oup);
}

#endif //L2S_UTIL_H
