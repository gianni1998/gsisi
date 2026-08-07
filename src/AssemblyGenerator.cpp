#include "AssemblyGenerator.hpp"

#include <array>
#include <stdexcept>
#include <unordered_set>
#include <utility>

void AssemblyGenerator::generate(
    const IRProgram& program,
    std::ostream& output
) {
    reset();
    collectSignatures(program);
    collectRuntimeImports(program);
    validateProgram(program);
    emitProgram(program, output);

    if (!output) {
        throw std::runtime_error("Assembly error: failed while writing output");
    }
}

void AssemblyGenerator::reset() {
    _context = FunctionContext{};
    _signatures.clear();
    _runtimeImports.clear();

    _signatures.emplace(
        "out",
        FunctionSignature{
            .parameterCount = 1,
            .returnType = Type::Void,
            .runtimeFunction = true
        }
    );
}

void AssemblyGenerator::collectSignatures(const IRProgram& program) {
    for (const IRFunction& function : program.functions) {
        if (function.name.empty()) {
            throw std::runtime_error("Assembly error: function name cannot be empty");
        }

        if (_signatures.contains(function.name)) {
            throw std::runtime_error(
                "Assembly error: duplicate or reserved function name '" +
                function.name + "'"
            );
        }

        _signatures.emplace(
            function.name,
            FunctionSignature{
                .parameterCount = function.parameters.size(),
                .returnType = function.returnType,
                .runtimeFunction = false
            }
        );
    }
}

void AssemblyGenerator::collectRuntimeImports(const IRProgram& program) {
    for (const IRFunction& function : program.functions) {
        for (const IRBlock& block : function.blocks) {
            for (const IRInstruction& instruction : block.instructions) {
                if (instruction.op != IROp::Call) {
                    continue;
                }

                const auto found = _signatures.find(instruction.callee);

                if (found != _signatures.end() && found->second.runtimeFunction) {
                    _runtimeImports.insert(instruction.callee);
                }
            }
        }
    }
}

void AssemblyGenerator::validateProgram(const IRProgram& program) const {
    const auto mainFunction = _signatures.find("main");

    if (mainFunction == _signatures.end()) {
        throw std::runtime_error("Assembly error: IR program has no main function");
    }

    if (mainFunction->second.parameterCount != 0 ||
        mainFunction->second.returnType != Type::Int) {
        throw std::runtime_error(
            "Assembly error: main must have signature fn main(): int"
        );
    }

    for (const IRFunction& function : program.functions) {
        validateFunction(function);
    }
}

void AssemblyGenerator::validateFunction(
    const IRFunction& function
) const {
    if (function.parameters.size() > 6) {
        throw std::runtime_error(
            "Assembly error: function '" + function.name +
            "' has more than 6 parameters"
        );
    }

    if (function.blocks.empty()) {
        throw std::runtime_error(
            "Assembly error: function '" + function.name +
            "' has no basic blocks"
        );
    }

    std::unordered_set<std::string> storageNames;

    for (const IRParameter& parameter : function.parameters) {
        if (parameter.name.empty() || parameter.type == Type::Void) {
            throw std::runtime_error(
                "Assembly error: invalid parameter in function '" +
                function.name + "'"
            );
        }

        if (!storageNames.insert(parameter.name).second) {
            throw std::runtime_error(
                "Assembly error: duplicate storage name '" +
                parameter.name + "' in function '" + function.name + "'"
            );
        }
    }

    for (const IRLocal& local : function.locals) {
        if (local.name.empty() || local.type == Type::Void) {
            throw std::runtime_error(
                "Assembly error: invalid local in function '" +
                function.name + "'"
            );
        }

        if (!storageNames.insert(local.name).second) {
            throw std::runtime_error(
                "Assembly error: duplicate storage name '" +
                local.name + "' in function '" + function.name + "'"
            );
        }
    }

    std::unordered_set<std::string> blockNames;

    for (const IRBlock& block : function.blocks) {
        if (block.name.empty()) {
            throw std::runtime_error(
                "Assembly error: function '" + function.name +
                "' contains an unnamed block"
            );
        }

        if (!blockNames.insert(block.name).second) {
            throw std::runtime_error(
                "Assembly error: duplicate block '" + block.name +
                "' in function '" + function.name + "'"
            );
        }

        if (block.instructions.empty()) {
            throw std::runtime_error(
                "Assembly error: block '" + block.name +
                "' is empty"
            );
        }

        for (std::size_t index = 0;
             index < block.instructions.size();
             ++index) {
            const IRInstruction& instruction = block.instructions[index];
            validateInstruction(instruction, function);

            if (isTerminator(instruction.op) &&
                index + 1 != block.instructions.size()) {
                throw std::runtime_error(
                    "Assembly error: terminator in block '" + block.name +
                    "' is not the final instruction"
                );
            }
        }

        if (!isTerminator(block.instructions.back().op)) {
            throw std::runtime_error(
                "Assembly error: block '" + block.name +
                "' has no terminator"
            );
        }
    }

    for (const IRBlock& block : function.blocks) {
        const IRInstruction& terminator = block.instructions.back();

        if (terminator.op == IROp::Jump) {
            if (!blockNames.contains(terminator.target)) {
                throw std::runtime_error(
                    "Assembly error: jump from block '" + block.name +
                    "' targets unknown block '" + terminator.target + "'"
                );
            }
        }

        if (terminator.op == IROp::Branch) {
            if (!blockNames.contains(terminator.trueTarget)) {
                throw std::runtime_error(
                    "Assembly error: branch from block '" + block.name +
                    "' targets unknown true block '" +
                    terminator.trueTarget + "'"
                );
            }

            if (!blockNames.contains(terminator.falseTarget)) {
                throw std::runtime_error(
                    "Assembly error: branch from block '" + block.name +
                    "' targets unknown false block '" +
                    terminator.falseTarget + "'"
                );
            }
        }
    }
}

void AssemblyGenerator::validateInstruction(
    const IRInstruction& instruction,
    const IRFunction& function
) const {
    if (!isSupported(instruction.op)) {
        throw std::runtime_error(
            "Assembly error: instruction '" +
            std::string(toString(instruction.op)) +
            "' is not supported in Chapter 8"
        );
    }

    switch (instruction.op) {
        case IROp::ConstInt:
        case IROp::ConstBool:
            requireDestination(instruction);
            requireArgumentCount(instruction, 0);
            return;

        case IROp::Load:
            requireDestination(instruction);
            requireArgumentCount(instruction, 1);
            return;

        case IROp::Store:
            requireArgumentCount(instruction, 2);
            return;

        case IROp::Add:
        case IROp::Subtract:
        case IROp::Multiply:
        case IROp::Divide:
            requireDestination(instruction);
            requireArgumentCount(instruction, 2);
            return;

        case IROp::Negate:
        case IROp::Not:
            requireDestination(instruction);
            requireArgumentCount(instruction, 1);
            return;

        case IROp::Call: {
            if (instruction.callee.empty()) {
                throw std::runtime_error("Assembly error: call has no callee");
            }

            const auto found = _signatures.find(instruction.callee);

            if (found == _signatures.end()) {
                throw std::runtime_error(
                    "Assembly error: call to unknown function '" +
                    instruction.callee + "'"
                );
            }

            const FunctionSignature& signature = found->second;

            if (instruction.args.size() != signature.parameterCount) {
                throw std::runtime_error(
                    "Assembly error: function '" + instruction.callee +
                    "' expects " + std::to_string(signature.parameterCount) +
                    " argument(s), got " +
                    std::to_string(instruction.args.size())
                );
            }

            if (instruction.args.size() > 6) {
                throw std::runtime_error(
                    "Assembly error: calls with more than 6 arguments are not supported"
                );
            }

            if (instruction.type != signature.returnType) {
                throw std::runtime_error(
                    "Assembly error: call result type does not match function '" +
                    instruction.callee + "'"
                );
            }

            if (signature.returnType == Type::Void) {
                if (!instruction.dst.empty()) {
                    throw std::runtime_error(
                        "Assembly error: void call cannot have a destination"
                    );
                }
            } else {
                requireDestination(instruction);
            }

            return;
        }

        case IROp::Return:
            if (function.returnType == Type::Void) {
                requireArgumentCount(instruction, 0);
            } else {
                requireArgumentCount(instruction, 1);
            }
            return;

        case IROp::Less:
        case IROp::LessEqual:
        case IROp::Greater:
        case IROp::GreaterEqual:
        case IROp::Equal:
        case IROp::NotEqual:
            requireDestination(instruction);
            requireArgumentCount(instruction, 2);

            if (instruction.type != Type::Bool) {
                throw std::runtime_error(
                    "Assembly error: comparison result must have type bool"
                );
            }
            return;

        case IROp::Jump:
            requireNoDestination(instruction);
            requireArgumentCount(instruction, 0);

            if (instruction.target.empty()) {
                throw std::runtime_error(
                    "Assembly error: jump has no target"
                );
            }
            return;

        case IROp::Branch:
            requireNoDestination(instruction);
            requireArgumentCount(instruction, 1);

            if (instruction.trueTarget.empty() ||
                instruction.falseTarget.empty()) {
                throw std::runtime_error(
                    "Assembly error: branch requires true and false targets"
                );
            }
            return;

        default:
            break;
    }

    throw std::runtime_error("Assembly internal error: unvalidated instruction");
}

void AssemblyGenerator::emitProgram(
    const IRProgram& program,
    std::ostream& output
) {
    output << "global _start\n";

    for (const std::string& runtimeFunction : _runtimeImports) {
        output << "extern " << functionLabel(runtimeFunction) << '\n';
    }

    output << "\nsection .text\n\n";
    output << "_start:\n";
    output << "    call si_main\n";
    output << "    mov edi, eax\n";
    output << "    mov eax, 60\n";
    output << "    syscall\n\n";

    for (const IRFunction& function : program.functions) {
        emitFunction(function, output);
    }
}

void AssemblyGenerator::emitFunction(
    const IRFunction& function,
    std::ostream& output
) {
    allocateStackSlots(function);

    output << functionLabel(function.name) << ":\n";
    output << "    push rbp\n";
    output << "    mov rbp, rsp\n";

    const int frameSize = stackSize();

    if (frameSize > 0) {
        output << "    sub rsp, " << frameSize << '\n';
    }

    for (std::size_t index = 0; index < function.parameters.size(); ++index) {
        output
            << "    mov " << stackOperand(function.parameters[index].name)
            << ", " << parameterRegister(index) << '\n';
    }

    for (const IRBlock& block : function.blocks) {
        output << blockLabel(function.name, block.name) << ":\n";

        for (const IRInstruction& instruction : block.instructions) {
            emitInstruction(instruction, output);
        }
    }

    output << _context.returnLabel << ":\n";
    output << "    mov rsp, rbp\n";
    output << "    pop rbp\n";
    output << "    ret\n\n";
}

void AssemblyGenerator::allocateStackSlots(
    const IRFunction& function
) {
    _context = FunctionContext{};
    _context.functionName = function.name;
    _context.returnLabel = ".L_" + function.name + "_return";

    for (const IRParameter& parameter : function.parameters) {
        allocateSlot(parameter.name);
    }

    for (const IRLocal& local : function.locals) {
        allocateSlot(local.name);
    }

    for (const IRBlock& block : function.blocks) {
        for (const IRInstruction& instruction : block.instructions) {
            if (!instruction.dst.empty()) {
                allocateSlot(instruction.dst);
            }
        }
    }
}

void AssemblyGenerator::allocateSlot(const std::string& name) {
    if (_context.stackSlots.contains(name)) {
        return;
    }

    _context.nextOffset += 4;
    _context.stackSlots.emplace(name, _context.nextOffset);
}

int AssemblyGenerator::slotOffset(const std::string& name) const {
    const auto found = _context.stackSlots.find(name);

    if (found == _context.stackSlots.end()) {
        throw std::runtime_error(
            "Assembly error: no stack slot for IR value '" + name + "'"
        );
    }

    return found->second;
}

int AssemblyGenerator::stackSize() const noexcept {
    constexpr int alignment = 16;
    const int size = _context.nextOffset;
    return ((size + alignment - 1) / alignment) * alignment;
}

std::string AssemblyGenerator::stackOperand(const std::string& name) const {
    return "dword [rbp - " + std::to_string(slotOffset(name)) + "]";
}

void AssemblyGenerator::emitInstruction(
    const IRInstruction& instruction,
    std::ostream& output
) {
    switch (instruction.op) {
        case IROp::ConstInt:
        case IROp::ConstBool:
            emitConstant(instruction, output);
            return;

        case IROp::Load:
            emitLoad(instruction, output);
            return;

        case IROp::Store:
            emitStore(instruction, output);
            return;

        case IROp::Add:
        case IROp::Subtract:
        case IROp::Multiply:
        case IROp::Divide:
            emitBinary(instruction, output);
            return;

        case IROp::Less:
        case IROp::LessEqual:
        case IROp::Greater:
        case IROp::GreaterEqual:
        case IROp::Equal:
        case IROp::NotEqual:
            emitComparison(instruction, output);
            return;

        case IROp::Negate:
        case IROp::Not:
            emitUnary(instruction, output);
            return;

        case IROp::Call:
            emitCall(instruction, output);
            return;

        case IROp::Jump:
            emitJump(instruction, output);
            return;

        case IROp::Branch:
            emitBranch(instruction, output);
            return;

        case IROp::Return:
            emitReturn(instruction, output);
            return;
    }

    throw std::runtime_error(
        "Assembly internal error: unsupported instruction reached emission"
    );
}

void AssemblyGenerator::emitConstant(
    const IRInstruction& instruction,
    std::ostream& output
) {
    const int value = instruction.op == IROp::ConstBool
        ? (instruction.boolValue ? 1 : 0)
        : instruction.intValue;

    output
        << "    mov " << stackOperand(instruction.dst)
        << ", " << value << '\n';
}

void AssemblyGenerator::emitLoad(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output
        << "    mov eax, " << stackOperand(instruction.args[0]) << '\n'
        << "    mov " << stackOperand(instruction.dst) << ", eax\n";
}

void AssemblyGenerator::emitStore(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output
        << "    mov eax, " << stackOperand(instruction.args[0]) << '\n'
        << "    mov " << stackOperand(instruction.args[1]) << ", eax\n";
}

void AssemblyGenerator::emitBinary(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output << "    mov eax, " << stackOperand(instruction.args[0]) << '\n';

    const std::string right = stackOperand(instruction.args[1]);

    switch (instruction.op) {
        case IROp::Add:
            output << "    add eax, " << right << '\n';
            break;

        case IROp::Subtract:
            output << "    sub eax, " << right << '\n';
            break;

        case IROp::Multiply:
            output << "    imul eax, " << right << '\n';
            break;

        case IROp::Divide:
            output << "    cdq\n";
            output << "    idiv " << right << '\n';
            break;

        default:
            throw std::runtime_error(
                "Assembly internal error: invalid binary instruction"
            );
    }

    output
        << "    mov " << stackOperand(instruction.dst)
        << ", eax\n";
}

void AssemblyGenerator::emitUnary(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output << "    mov eax, " << stackOperand(instruction.args[0]) << '\n';

    switch (instruction.op) {
        case IROp::Negate:
            output << "    neg eax\n";
            break;

        case IROp::Not:
            output << "    cmp eax, 0\n";
            output << "    sete al\n";
            output << "    movzx eax, al\n";
            break;

        default:
            throw std::runtime_error(
                "Assembly internal error: invalid unary instruction"
            );
    }

    output
        << "    mov " << stackOperand(instruction.dst)
        << ", eax\n";
}

void AssemblyGenerator::emitCall(
    const IRInstruction& instruction,
    std::ostream& output
) {
    for (std::size_t index = 0; index < instruction.args.size(); ++index) {
        output
            << "    mov " << parameterRegister(index) << ", "
            << stackOperand(instruction.args[index]) << '\n';
    }

    output << "    call " << functionLabel(instruction.callee) << '\n';

    if (!instruction.dst.empty()) {
        output
            << "    mov " << stackOperand(instruction.dst)
            << ", eax\n";
    }
}

void AssemblyGenerator::emitReturn(
    const IRInstruction& instruction,
    std::ostream& output
) {
    if (!instruction.args.empty()) {
        output
            << "    mov eax, "
            << stackOperand(instruction.args[0])
            << '\n';
    }

    output << "    jmp " << _context.returnLabel << '\n';
}

void AssemblyGenerator::requireDestination(
    const IRInstruction& instruction
) {
    if (instruction.dst.empty()) {
        throw std::runtime_error(
            "Assembly error: instruction '" +
            std::string(toString(instruction.op)) +
            "' requires a destination"
        );
    }
}

void AssemblyGenerator::requireArgumentCount(
    const IRInstruction& instruction,
    std::size_t expected
) {
    if (instruction.args.size() != expected) {
        throw std::runtime_error(
            "Assembly error: instruction '" +
            std::string(toString(instruction.op)) +
            "' expects " + std::to_string(expected) +
            " argument(s), got " +
            std::to_string(instruction.args.size())
        );
    }
}

bool AssemblyGenerator::isSupported(IROp operation) noexcept {
    switch (operation) {
        case IROp::ConstInt:
        case IROp::ConstBool:
        case IROp::Load:
        case IROp::Store:
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
        case IROp::Negate:
        case IROp::Not:
        case IROp::Call:
        case IROp::Jump:
        case IROp::Branch:
        case IROp::Return:
            return true;
    }

    return false;
}

const char* AssemblyGenerator::parameterRegister(std::size_t index) {
    static constexpr std::array<const char*, 6> registers{
        "edi", "esi", "edx", "ecx", "r8d", "r9d"
    };

    if (index >= registers.size()) {
        throw std::runtime_error(
            "Assembly error: more than 6 register arguments are not supported"
        );
    }

    return registers[index];
}

std::string AssemblyGenerator::functionLabel(const std::string& name) {
    return "si_" + name;
}

std::string AssemblyGenerator::blockLabel(
    const std::string& functionName,
    const std::string& blockName
) {
    return ".L_" + functionName + "_" + blockName;
}

void AssemblyGenerator::requireNoDestination(
    const IRInstruction& instruction
) {
    if (!instruction.dst.empty()) {
        throw std::runtime_error(
            "Assembly error: instruction '" +
            std::string(toString(instruction.op)) +
            "' cannot have a destination"
        );
    }
}

void AssemblyGenerator::emitComparison(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output
        << "    mov eax, "
        << stackOperand(instruction.args[0])
        << '\n';

    output
        << "    cmp eax, "
        << stackOperand(instruction.args[1])
        << '\n';

    switch (instruction.op) {
        case IROp::Equal:
            output << "    sete al\n";
            break;

        case IROp::NotEqual:
            output << "    setne al\n";
            break;

        case IROp::Less:
            output << "    setl al\n";
            break;

        case IROp::LessEqual:
            output << "    setle al\n";
            break;

        case IROp::Greater:
            output << "    setg al\n";
            break;

        case IROp::GreaterEqual:
            output << "    setge al\n";
            break;

        default:
            throw std::runtime_error(
                "Assembly internal error: invalid comparison instruction"
            );
    }

    output << "    movzx eax, al\n";
    output
        << "    mov " << stackOperand(instruction.dst)
        << ", eax\n";
}

void AssemblyGenerator::emitJump(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output
        << "    jmp "
        << blockLabel(_context.functionName, instruction.target)
        << '\n';
}

void AssemblyGenerator::emitBranch(
    const IRInstruction& instruction,
    std::ostream& output
) {
    output
        << "    cmp "
        << stackOperand(instruction.args[0])
        << ", 0\n";

    output
        << "    jne "
        << blockLabel(
            _context.functionName,
            instruction.trueTarget
        )
        << '\n';

    output
        << "    jmp "
        << blockLabel(
            _context.functionName,
            instruction.falseTarget
        )
        << '\n';
}