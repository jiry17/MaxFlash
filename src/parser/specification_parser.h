#ifndef L2S_SPECIFICATION_PARSER_H
#define L2S_SPECIFICATION_PARSER_H

#include "specification.h"

// Initialize a specification from a file.
// If a new type of specification is required, its parser must be implemented in this function.
namespace parser {
    extern Specification* loadSpecification(std::string file_name, std::string benchmark_type);
}


#endif //L2S_SPECIFICATION_PARSER_H
