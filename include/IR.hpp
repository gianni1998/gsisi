#pragma once

#include <string>
#include <vector>

#include "Type.hpp"

//--------------------------------------
// Operations
//--------------------------------------
enum class IROp {
    ConstInt,
    ConstBool,

    Load,
    Store,

    Add,
    Subtract,
    Multiply,
    Divide,

    Less,
    LessEqual,
    Greater,
    GreaterEqual,
    Equal,
    NotEqual,

    Negate,
    Not,

    Call,

    Jump,
    Branch,
    Return
};

//--------------------------------------
// Function data
//--------------------------------------
struct IRParameter {
    std::string name;
    Type type{Type::Unknown};
};

struct IRLocal {
    std::string name;
    Type type{Type::Unknown};
};

//--------------------------------------
// Instructions and blocks
//--------------------------------------
struct IRInstruction {
    IROp op{IROp::Return};
    Type type{Type::Void};

    // Result produced by the instruction, such as %t0.
    std::string dst;

    // Input values or variable storage names.
    std::vector<std::string> args;

    // Call data.
    std::string callee;

    // Control-flow targets.
    std::string target;
    std::string trueTarget;
    std::string falseTarget;

    // Literal payloads.
    int intValue{};
    bool boolValue{};
};

struct IRBlock {
    std::string name;
    std::vector<IRInstruction> instructions;
};

struct IRFunction {
    std::string name;
    std::vector<IRParameter> parameters;
    std::vector<IRLocal> locals;
    Type returnType{Type::Void};
    std::vector<IRBlock> blocks;
};

struct IRProgram {
    std::vector<IRFunction> functions;
};

//--------------------------------------
// Helpers
//--------------------------------------
[[nodiscard]] inline const char* toString(IROp op) noexcept {
    switch (op) {
        case IROp::ConstInt:     return "const_int";
        case IROp::ConstBool:    return "const_bool";

        case IROp::Load:         return "load";
        case IROp::Store:        return "store";

        case IROp::Add:          return "add";
        case IROp::Subtract:     return "sub";
        case IROp::Multiply:     return "mul";
        case IROp::Divide:       return "div";

        case IROp::Less:         return "less";
        case IROp::LessEqual:    return "less_equal";
        case IROp::Greater:      return "greater";
        case IROp::GreaterEqual: return "greater_equal";
        case IROp::Equal:        return "equal";
        case IROp::NotEqual:     return "not_equal";

        case IROp::Negate:       return "negate";
        case IROp::Not:          return "not";

        case IROp::Call:         return "call";

        case IROp::Jump:         return "jump";
        case IROp::Branch:       return "branch";
        case IROp::Return:       return "return";
    }

    return "unknown";
}

[[nodiscard]] inline bool isTerminator(IROp op) noexcept {
    return op == IROp::Jump ||
           op == IROp::Branch ||
           op == IROp::Return;
}