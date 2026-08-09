#include "IROptimiser.hpp"

#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <queue>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace {

struct ConstantValue {
    Type type{Type::Unknown};
    std::int32_t intValue{};
    bool boolValue{};
};

using ConstantMap = std::unordered_map<std::string, ConstantValue>;

std::uint32_t rawBits(std::int32_t value) noexcept {
    return std::bit_cast<std::uint32_t>(value);
}

std::int32_t signedBits(std::uint32_t value) noexcept {
    return std::bit_cast<std::int32_t>(value);
}

std::int32_t wrappingAdd(std::int32_t lhs, std::int32_t rhs) noexcept {
    return signedBits(rawBits(lhs) + rawBits(rhs));
}

std::int32_t wrappingSubtract(std::int32_t lhs, std::int32_t rhs) noexcept {
    return signedBits(rawBits(lhs) - rawBits(rhs));
}

std::int32_t wrappingMultiply(std::int32_t lhs, std::int32_t rhs) noexcept {
    return signedBits(rawBits(lhs) * rawBits(rhs));
}

std::int32_t wrappingNegate(std::int32_t value) noexcept {
    return signedBits(0U - rawBits(value));
}

void clearNonConstantFields(IRInstruction& instruction) {
    instruction.args.clear();
    instruction.callee.clear();
    instruction.target.clear();
    instruction.trueTarget.clear();
    instruction.falseTarget.clear();
}

void replaceWithIntConstant(IRInstruction& instruction, std::int32_t value) {
    instruction.op = IROp::ConstInt;
    instruction.type = Type::Int;
    instruction.intValue = value;
    instruction.boolValue = false;
    clearNonConstantFields(instruction);
}

void replaceWithBoolConstant(IRInstruction& instruction, bool value) {
    instruction.op = IROp::ConstBool;
    instruction.type = Type::Bool;
    instruction.intValue = 0;
    instruction.boolValue = value;
    clearNonConstantFields(instruction);
}

const ConstantValue* findConstant(
    const ConstantMap& constants,
    const std::string& name
) {
    auto found = constants.find(name);
    return found == constants.end() ? nullptr : &found->second;
}

bool isBinaryFoldCandidate(IROp op) noexcept {
    switch (op) {
        case IROp::Add:
        case IROp::Subtract:
        case IROp::Multiply:
        case IROp::Divide:
        case IROp::Less:
        case IROp::LessEqual:
        case IROp::Greater:
        case IROp::GreaterEqual:
        case IROp::Equal:
        case IROp::NotEqual:
            return true;
        default:
            return false;
    }
}

}

void IROptimiser::optimise(IRProgram& program) const {
    for (IRFunction& function : program.functions) {
        constantFold(function);
        simplifyBranches(function);
        removeUnreachableBlocks(function);
        removeDeadInstructions(function);
    }
}

void IROptimiser::constantFold(IRFunction& function) const {
    ConstantMap constants;

    for (IRBlock& block : function.blocks) {
        for (IRInstruction& instruction : block.instructions) {
            if (instruction.op == IROp::ConstInt) {
                constants[instruction.dst] = ConstantValue{
                    Type::Int,
                    static_cast<std::int32_t>(instruction.intValue),
                    false
                };
                continue;
            }

            if (instruction.op == IROp::ConstBool) {
                constants[instruction.dst] = ConstantValue{
                    Type::Bool,
                    0,
                    instruction.boolValue
                };
                continue;
            }

            if (instruction.op == IROp::Negate ||
                instruction.op == IROp::Not) {
                if (instruction.args.size() != 1) {
                    continue;
                }

                const ConstantValue* operand =
                    findConstant(constants, instruction.args[0]);

                if (!operand) {
                    continue;
                }

                if (instruction.op == IROp::Negate &&
                    operand->type == Type::Int) {
                    const std::int32_t result =
                        wrappingNegate(operand->intValue);

                    replaceWithIntConstant(instruction, result);
                    constants[instruction.dst] = ConstantValue{
                        Type::Int, result, false
                    };
                }
                else if (instruction.op == IROp::Not &&
                         operand->type == Type::Bool) {
                    const bool result = !operand->boolValue;

                    replaceWithBoolConstant(instruction, result);
                    constants[instruction.dst] = ConstantValue{
                        Type::Bool, 0, result
                    };
                }

                continue;
            }

            if (!isBinaryFoldCandidate(instruction.op) ||
                instruction.args.size() != 2) {
                continue;
            }

            const ConstantValue* lhs =
                findConstant(constants, instruction.args[0]);
            const ConstantValue* rhs =
                findConstant(constants, instruction.args[1]);

            if (!lhs || !rhs) {
                continue;
            }

            switch (instruction.op) {
                case IROp::Add:
                case IROp::Subtract:
                case IROp::Multiply:
                case IROp::Divide: {
                    if (lhs->type != Type::Int || rhs->type != Type::Int) {
                        break;
                    }

                    if (instruction.op == IROp::Divide &&
                        (rhs->intValue == 0 ||
                         (lhs->intValue == std::numeric_limits<std::int32_t>::min() &&
                          rhs->intValue == -1))) {
                        break;
                    }

                    std::int32_t result{};

                    switch (instruction.op) {
                        case IROp::Add:
                            result = wrappingAdd(lhs->intValue, rhs->intValue);
                            break;
                        case IROp::Subtract:
                            result = wrappingSubtract(lhs->intValue, rhs->intValue);
                            break;
                        case IROp::Multiply:
                            result = wrappingMultiply(lhs->intValue, rhs->intValue);
                            break;
                        case IROp::Divide:
                            result = lhs->intValue / rhs->intValue;
                            break;
                        default:
                            break;
                    }

                    replaceWithIntConstant(instruction, result);
                    constants[instruction.dst] = ConstantValue{
                        Type::Int, result, false
                    };
                    break;
                }

                case IROp::Less:
                case IROp::LessEqual:
                case IROp::Greater:
                case IROp::GreaterEqual: {
                    if (lhs->type != Type::Int || rhs->type != Type::Int) {
                        break;
                    }

                    bool result{};

                    switch (instruction.op) {
                        case IROp::Less:
                            result = lhs->intValue < rhs->intValue;
                            break;
                        case IROp::LessEqual:
                            result = lhs->intValue <= rhs->intValue;
                            break;
                        case IROp::Greater:
                            result = lhs->intValue > rhs->intValue;
                            break;
                        case IROp::GreaterEqual:
                            result = lhs->intValue >= rhs->intValue;
                            break;
                        default:
                            break;
                    }

                    replaceWithBoolConstant(instruction, result);
                    constants[instruction.dst] = ConstantValue{
                        Type::Bool, 0, result
                    };
                    break;
                }

                case IROp::Equal:
                case IROp::NotEqual: {
                    if (lhs->type != rhs->type) {
                        break;
                    }

                    bool equal{};

                    if (lhs->type == Type::Int) {
                        equal = lhs->intValue == rhs->intValue;
                    }
                    else if (lhs->type == Type::Bool) {
                        equal = lhs->boolValue == rhs->boolValue;
                    }
                    else {
                        break;
                    }

                    const bool result =
                        instruction.op == IROp::Equal ? equal : !equal;

                    replaceWithBoolConstant(instruction, result);
                    constants[instruction.dst] = ConstantValue{
                        Type::Bool, 0, result
                    };
                    break;
                }

                default:
                    break;
            }
        }
    }
}

void IROptimiser::simplifyBranches(IRFunction& function) const {
    std::unordered_map<std::string, bool> booleanConstants;

    for (const IRBlock& block : function.blocks) {
        for (const IRInstruction& instruction : block.instructions) {
            if (instruction.op == IROp::ConstBool && !instruction.dst.empty()) {
                booleanConstants[instruction.dst] = instruction.boolValue;
            }
        }
    }

    for (IRBlock& block : function.blocks) {
        for (IRInstruction& instruction : block.instructions) {
            if (instruction.op != IROp::Branch ||
                instruction.args.size() != 1) {
                continue;
            }

            auto found = booleanConstants.find(instruction.args[0]);
            if (found == booleanConstants.end()) {
                continue;
            }

            const std::string target =
                found->second
                    ? instruction.trueTarget
                    : instruction.falseTarget;

            instruction.op = IROp::Jump;
            instruction.type = Type::Void;
            instruction.dst.clear();
            instruction.args.clear();
            instruction.callee.clear();
            instruction.target = target;
            instruction.trueTarget.clear();
            instruction.falseTarget.clear();
        }
    }
}

void IROptimiser::removeUnreachableBlocks(IRFunction& function) const {
    if (function.blocks.empty()) {
        return;
    }

    std::unordered_map<std::string, const IRBlock*> blocks;

    for (const IRBlock& block : function.blocks) {
        if (!blocks.emplace(block.name, &block).second) {
            throw std::runtime_error(
                "IR optimisation error: duplicate block '" + block.name + "'"
            );
        }
    }

    std::unordered_set<std::string> reachable;
    std::queue<std::string> work;

    work.push(function.blocks.front().name);

    while (!work.empty()) {
        std::string name = std::move(work.front());
        work.pop();

        if (!reachable.insert(name).second) {
            continue;
        }

        auto foundBlock = blocks.find(name);
        if (foundBlock == blocks.end()) {
            throw std::runtime_error(
                "IR optimisation error: missing block '" + name + "'"
            );
        }

        const IRBlock& block = *foundBlock->second;

        if (block.instructions.empty()) {
            throw std::runtime_error(
                "IR optimisation error: block '" + name + "' is empty"
            );
        }

        const IRInstruction& terminator = block.instructions.back();

        auto enqueueTarget = [&](const std::string& target) {
            if (!blocks.contains(target)) {
                throw std::runtime_error(
                    "IR optimisation error: block '" + name +
                    "' targets missing block '" + target + "'"
                );
            }

            work.push(target);
        };

        switch (terminator.op) {
            case IROp::Jump:
                enqueueTarget(terminator.target);
                break;

            case IROp::Branch:
                enqueueTarget(terminator.trueTarget);
                enqueueTarget(terminator.falseTarget);
                break;

            case IROp::Return:
                break;

            default:
                throw std::runtime_error(
                    "IR optimisation error: block '" + name +
                    "' does not end in a terminator"
                );
        }
    }

    std::erase_if(
        function.blocks,
        [&](const IRBlock& block) {
            return !reachable.contains(block.name);
        }
    );
}

bool IROptimiser::producesSideEffects(
    const IRInstruction& instruction
) noexcept {
    switch (instruction.op) {
        case IROp::Store:
        case IROp::Call:
        case IROp::Jump:
        case IROp::Branch:
        case IROp::Return:
            return true;

        // idiv can trap on division by zero or INT_MIN / -1.
        // Removing it without proving safety could change behaviour.
        case IROp::Divide:
            return true;

        default:
            return false;
    }
}

void IROptimiser::removeDeadInstructions(IRFunction& function) const {
    bool changed = false;

    do {
        std::unordered_set<std::string> usedValues;

        for (const IRBlock& block : function.blocks) {
            for (const IRInstruction& instruction : block.instructions) {
                for (const std::string& argument : instruction.args) {
                    usedValues.insert(argument);
                }
            }
        }

        changed = false;

        for (IRBlock& block : function.blocks) {
            const auto oldSize = block.instructions.size();

            std::erase_if(
                block.instructions,
                [&](const IRInstruction& instruction) {
                    if (producesSideEffects(instruction)) {
                        return false;
                    }

                    if (instruction.dst.empty()) {
                        return false;
                    }

                    return !usedValues.contains(instruction.dst);
                }
            );

            if (block.instructions.size() != oldSize) {
                changed = true;
            }
        }
    }
    while (changed);
}