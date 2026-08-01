#pragma once

#include <ostream>

//--------------------------------------
// Language Types
//--------------------------------------
enum class Type {
    Int,
    Bool,
    Void,
    Unknown
};

//--------------------------------------
// Helpers
//--------------------------------------
[[nodiscard]] inline const char* toString(Type type) noexcept {
    switch (type) {
        case Type::Int:     return "int";
        case Type::Bool:    return "bool";
        case Type::Void:    return "void";
        case Type::Unknown: return "unknown";
    }
    return "unknown";
}

inline std::ostream& operator<<(std::ostream& os, Type type) {
    return os << toString(type);
}