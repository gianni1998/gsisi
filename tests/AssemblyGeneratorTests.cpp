#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "AssemblyGenerator.hpp"
#include "IR.hpp"

namespace {

void require(
    bool condition,
    const std::string& message
) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireContains(
    const std::string& text,
    const std::string& expected
) {
    require(
        text.find(expected) != std::string::npos,
        "missing assembly fragment: " + expected
    );
}

std::string generateAssembly(
    const IRProgram& program
) {
    std::ostringstream output;

    AssemblyGenerator generator;
    generator.generate(program, output);

    return output.str();
}

IRProgram makeMainProgram(
    std::vector<IRInstruction> instructions,
    std::vector<IRLocal> locals = {}
) {
    IRFunction main;
    main.name = "main";
    main.returnType = Type::Int;
    main.locals = std::move(locals);
    main.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = std::move(instructions)
    });

    IRProgram program;
    program.functions.push_back(std::move(main));

    return program;
}

void testEntryPointAndReturn() {
    const IRProgram program = makeMainProgram({
        IRInstruction{
            .op = IROp::ConstInt,
            .type = Type::Int,
            .dst = "%t0",
            .intValue = 42
        },
        IRInstruction{
            .op = IROp::Return,
            .args = {"%t0"}
        }
    });

    const std::string assembly =
        generateAssembly(program);

    requireContains(assembly, "global _start");
    requireContains(assembly, "call si_main");
    requireContains(assembly, "si_main:");
    requireContains(
        assembly,
        "mov dword [rbp - 4], 42"
    );
    requireContains(
        assembly,
        "jmp .L_main_return"
    );
}

void testStackAlignmentAndMemoryOperations() {
    const IRProgram program = makeMainProgram(
        {
            IRInstruction{
                .op = IROp::ConstInt,
                .type = Type::Int,
                .dst = "%t0",
                .intValue = 7
            },
            IRInstruction{
                .op = IROp::Store,
                .args = {"%t0", "x"}
            },
            IRInstruction{
                .op = IROp::Load,
                .type = Type::Int,
                .dst = "%t1",
                .args = {"x"}
            },
            IRInstruction{
                .op = IROp::Return,
                .args = {"%t1"}
            }
        },
        {
            IRLocal{
                .name = "x",
                .type = Type::Int
            }
        }
    );

    const std::string assembly =
        generateAssembly(program);

    requireContains(assembly, "sub rsp, 16");
    requireContains(
        assembly,
        "mov dword [rbp - 4], eax"
    );
    requireContains(
        assembly,
        "mov eax, dword [rbp - 4]"
    );
}

void testArithmeticOperations() {
    const IRProgram program = makeMainProgram({
        IRInstruction{
            .op = IROp::ConstInt,
            .type = Type::Int,
            .dst = "%t0",
            .intValue = 20
        },
        IRInstruction{
            .op = IROp::ConstInt,
            .type = Type::Int,
            .dst = "%t1",
            .intValue = 6
        },
        IRInstruction{
            .op = IROp::Subtract,
            .type = Type::Int,
            .dst = "%t2",
            .args = {"%t0", "%t1"}
        },
        IRInstruction{
            .op = IROp::Multiply,
            .type = Type::Int,
            .dst = "%t3",
            .args = {"%t2", "%t1"}
        },
        IRInstruction{
            .op = IROp::Divide,
            .type = Type::Int,
            .dst = "%t4",
            .args = {"%t3", "%t1"}
        },
        IRInstruction{
            .op = IROp::Return,
            .args = {"%t4"}
        }
    });

    const std::string assembly =
        generateAssembly(program);

    requireContains(assembly, "sub eax,");
    requireContains(assembly, "imul eax,");
    requireContains(assembly, "cdq");
    requireContains(assembly, "idiv dword");
}

void testUnaryOperations() {
    const IRProgram program = makeMainProgram({
        IRInstruction{
            .op = IROp::ConstInt,
            .type = Type::Int,
            .dst = "%t0",
            .intValue = 5
        },
        IRInstruction{
            .op = IROp::Negate,
            .type = Type::Int,
            .dst = "%t1",
            .args = {"%t0"}
        },
        IRInstruction{
            .op = IROp::ConstBool,
            .type = Type::Bool,
            .dst = "%t2",
            .boolValue = false
        },
        IRInstruction{
            .op = IROp::Not,
            .type = Type::Bool,
            .dst = "%t3",
            .args = {"%t2"}
        },
        IRInstruction{
            .op = IROp::Return,
            .args = {"%t1"}
        }
    });

    const std::string assembly =
        generateAssembly(program);

    requireContains(assembly, "neg eax");
    requireContains(assembly, "cmp eax, 0");
    requireContains(assembly, "sete al");
    requireContains(assembly, "movzx eax, al");
}

void testUnsupportedOperationIsRejectedBeforeOutput() {
    const IRProgram program = makeMainProgram({
        IRInstruction{
            .op = IROp::Call,
            .callee = "out"
        },
        IRInstruction{
            .op = IROp::Return
        }
    });

    std::ostringstream output;
    AssemblyGenerator generator;

    bool rejected = false;

    try {
        generator.generate(program, output);
    }
    catch (const std::runtime_error& error) {
        rejected =
            std::string(error.what()).find(
                "not supported"
            ) != std::string::npos;
    }

    require(rejected, "call instruction should be rejected");
    require(
        output.str().empty(),
        "validation should happen before assembly output"
    );
}

void testParametersAreRejected() {
    IRProgram program = makeMainProgram({
        IRInstruction{
            .op = IROp::Return
        }
    });

    program.functions[0].parameters.push_back(
        IRParameter{
            .name = "value",
            .type = Type::Int
        }
    );

    bool rejected = false;

    try {
        static_cast<void>(generateAssembly(program));
    }
    catch (const std::runtime_error& error) {
        rejected =
            std::string(error.what()).find(
                "parameters"
            ) != std::string::npos;
    }

    require(rejected, "parameters should be rejected");
}

} // namespace

int main() {
    const std::vector<
        std::pair<std::string, std::function<void()>>
    > tests = {
        {
            "entry point and return",
            testEntryPointAndReturn
        },
        {
            "stack alignment and memory operations",
            testStackAlignmentAndMemoryOperations
        },
        {
            "arithmetic operations",
            testArithmeticOperations
        },
        {
            "unary operations",
            testUnaryOperations
        },
        {
            "unsupported operations",
            testUnsupportedOperationIsRejectedBeforeOutput
        },
        {
            "function parameters",
            testParametersAreRejected
        }
    };

    int passed = 0;

    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
            ++passed;
        }
        catch (const std::exception& error) {
            std::cerr
                << "[FAIL] "
                << name
                << ": "
                << error.what()
                << '\n';
        }
    }

    std::cout
        << passed
        << '/'
        << tests.size()
        << " tests passed\n";

    return passed == static_cast<int>(tests.size())
        ? 0
        : 1;
}