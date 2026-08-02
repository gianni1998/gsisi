#pragma once

#include <cstddef>
#include <ostream>
#include <string>
#include <unordered_map>

#include "IR.hpp"

class AssemblyGenerator {
public:
    void generate(
        const IRProgram& program,
        std::ostream& output
    );

private:
    struct FunctionContext {
        std::unordered_map<std::string, int> stackSlots;
        int nextOffset{0};
        std::string returnLabel;
    };

    FunctionContext _context;

    //----------------------------------
    // Validation
    //----------------------------------
    void validateProgram(const IRProgram& program) const;
    void validateFunction(const IRFunction& function) const;

    static void validateInstruction(
        const IRInstruction& instruction
    );

    //----------------------------------
    // Program and functions
    //----------------------------------
    void emitProgram(
        const IRProgram& program,
        std::ostream& output
    );

    void emitFunction(
        const IRFunction& function,
        std::ostream& output
    );

    //----------------------------------
    // Stack slots
    //----------------------------------
    void allocateStackSlots(const IRFunction& function);
    void allocateSlot(const std::string& name);

    [[nodiscard]] int slotOffset(
        const std::string& name
    ) const;

    [[nodiscard]] int stackSize() const noexcept;

    [[nodiscard]] std::string stackOperand(
        const std::string& name
    ) const;

    //----------------------------------
    // Instructions
    //----------------------------------
    void emitInstruction(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitConstant(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitLoad(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitStore(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitBinary(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitUnary(
        const IRInstruction& instruction,
        std::ostream& output
    );

    void emitReturn(
        const IRInstruction& instruction,
        std::ostream& output
    );

    //----------------------------------
    // Helpers
    //----------------------------------
    static void requireDestination(
        const IRInstruction& instruction
    );

    static void requireArgumentCount(
        const IRInstruction& instruction,
        std::size_t expected
    );

    [[nodiscard]] static bool isSupported(
        IROp operation
    ) noexcept;

    [[nodiscard]] static std::string functionLabel(
        const std::string& name
    );

    [[nodiscard]] static std::string blockLabel(
        const std::string& functionName,
        const std::string& blockName
    );
};