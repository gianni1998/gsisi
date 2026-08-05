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

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void requireContains(const std::string& text, const std::string& expected) {
    require(
        text.find(expected) != std::string::npos,
        "missing assembly fragment: " + expected
    );
}

std::string generateAssembly(const IRProgram& program) {
    std::ostringstream output;
    AssemblyGenerator generator;
    generator.generate(program, output);
    return output.str();
}

IRInstruction constInt(std::string destination, int value) {
    return IRInstruction{
        .op = IROp::ConstInt,
        .type = Type::Int,
        .dst = std::move(destination),
        .intValue = value
    };
}

IRInstruction returnValue(std::string value) {
    return IRInstruction{
        .op = IROp::Return,
        .args = {std::move(value)}
    };
}

IRFunction makeMain(std::vector<IRInstruction> instructions) {
    IRFunction function;
    function.name = "main";
    function.returnType = Type::Int;
    function.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = std::move(instructions)
    });
    return function;
}

void testParametersAreCopiedFromAbiRegisters() {
    IRFunction add;
    add.name = "add";
    add.parameters = {
        IRParameter{"a", Type::Int},
        IRParameter{"b", Type::Int}
    };
    add.returnType = Type::Int;
    add.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = {
            IRInstruction{.op = IROp::Load, .type = Type::Int, .dst = "%t0", .args = {"a"}},
            IRInstruction{.op = IROp::Return, .args = {"%t0"}}
        }
    });

    IRProgram program;
    program.functions.push_back(std::move(add));
    program.functions.push_back(makeMain({
        constInt("%t0", 0),
        returnValue("%t0")
    }));

    const std::string assembly = generateAssembly(program);
    requireContains(assembly, "mov dword [rbp - 4], edi");
    requireContains(assembly, "mov dword [rbp - 8], esi");
}

void testUserFunctionCallUsesRegistersAndCapturesEax() {
    IRFunction add;
    add.name = "add";
    add.parameters = {
        IRParameter{"a", Type::Int},
        IRParameter{"b", Type::Int}
    };
    add.returnType = Type::Int;
    add.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = {
            IRInstruction{.op = IROp::Load, .type = Type::Int, .dst = "%t0", .args = {"a"}},
            IRInstruction{.op = IROp::Load, .type = Type::Int, .dst = "%t1", .args = {"b"}},
            IRInstruction{.op = IROp::Add, .type = Type::Int, .dst = "%t2", .args = {"%t0", "%t1"}},
            IRInstruction{.op = IROp::Return, .args = {"%t2"}}
        }
    });

    IRFunction main = makeMain({
        constInt("%t0", 20),
        constInt("%t1", 22),
        IRInstruction{
            .op = IROp::Call,
            .type = Type::Int,
            .dst = "%t2",
            .args = {"%t0", "%t1"},
            .callee = "add"
        },
        returnValue("%t2")
    });

    IRProgram program;
    program.functions.push_back(std::move(add));
    program.functions.push_back(std::move(main));

    const std::string assembly = generateAssembly(program);
    requireContains(assembly, "mov edi, dword [rbp - 4]");
    requireContains(assembly, "mov esi, dword [rbp - 8]");
    requireContains(assembly, "call si_add");
    requireContains(assembly, "mov dword [rbp - 12], eax");
}

void testRuntimeCallDeclaresExternalSymbol() {
    IRProgram program;
    program.functions.push_back(makeMain({
        constInt("%t0", 42),
        IRInstruction{
            .op = IROp::Call,
            .type = Type::Void,
            .args = {"%t0"},
            .callee = "out"
        },
        constInt("%t1", 0),
        returnValue("%t1")
    }));

    const std::string assembly = generateAssembly(program);
    requireContains(assembly, "extern si_out");
    requireContains(assembly, "call si_out");
}

void testSixArgumentsUseAllRegisterArguments() {
    IRFunction function;
    function.name = "six";
    function.returnType = Type::Int;

    for (int index = 0; index < 6; ++index) {
        function.parameters.push_back(IRParameter{
            "p" + std::to_string(index),
            Type::Int
        });
    }

    function.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = {
            IRInstruction{.op = IROp::Load, .type = Type::Int, .dst = "%t0", .args = {"p0"}},
            IRInstruction{.op = IROp::Return, .args = {"%t0"}}
        }
    });

    std::vector<IRInstruction> mainInstructions;
    std::vector<std::string> arguments;

    for (int index = 0; index < 6; ++index) {
        const std::string temporary = "%t" + std::to_string(index);
        mainInstructions.push_back(constInt(temporary, index + 1));
        arguments.push_back(temporary);
    }

    mainInstructions.push_back(IRInstruction{
        .op = IROp::Call,
        .type = Type::Int,
        .dst = "%t6",
        .args = arguments,
        .callee = "six"
    });
    mainInstructions.push_back(returnValue("%t6"));

    IRProgram program;
    program.functions.push_back(std::move(function));
    program.functions.push_back(makeMain(std::move(mainInstructions)));

    const std::string assembly = generateAssembly(program);
    requireContains(assembly, "mov r8d,");
    requireContains(assembly, "mov r9d,");
}

void testMoreThanSixParametersAreRejected() {
    IRFunction function;
    function.name = "too_many";
    function.returnType = Type::Int;

    for (int index = 0; index < 7; ++index) {
        function.parameters.push_back(IRParameter{
            "p" + std::to_string(index),
            Type::Int
        });
    }

    function.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = {
            constInt("%t0", 0),
            returnValue("%t0")
        }
    });

    IRProgram program;
    program.functions.push_back(std::move(function));
    program.functions.push_back(makeMain({
        constInt("%t0", 0),
        returnValue("%t0")
    }));

    bool rejected = false;

    try {
        generateAssembly(program);
    } catch (const std::runtime_error& error) {
        rejected = std::string(error.what()).find("more than 6 parameters") != std::string::npos;
    }

    require(rejected, "expected seven-parameter function to be rejected");
}

void testInvalidCallSignatureIsRejected() {
    IRFunction identity;
    identity.name = "identity";
    identity.parameters = {IRParameter{"value", Type::Int}};
    identity.returnType = Type::Int;
    identity.blocks.push_back(IRBlock{
        .name = "entry",
        .instructions = {
            IRInstruction{.op = IROp::Load, .type = Type::Int, .dst = "%t0", .args = {"value"}},
            IRInstruction{.op = IROp::Return, .args = {"%t0"}}
        }
    });

    IRProgram program;
    program.functions.push_back(std::move(identity));
    program.functions.push_back(makeMain({
        IRInstruction{
            .op = IROp::Call,
            .type = Type::Int,
            .dst = "%t0",
            .callee = "identity"
        },
        returnValue("%t0")
    }));

    bool rejected = false;

    try {
        generateAssembly(program);
    } catch (const std::runtime_error& error) {
        rejected = std::string(error.what()).find("expects 1 argument") != std::string::npos;
    }

    require(rejected, "expected wrong argument count to be rejected");
}

void testGeneratorCanBeReused() {
    AssemblyGenerator generator;

    for (int value : {1, 2}) {
        IRProgram program;
        program.functions.push_back(makeMain({
            constInt("%t0", value),
            returnValue("%t0")
        }));

        std::ostringstream output;
        generator.generate(program, output);
        requireContains(output.str(), ", " + std::to_string(value));
    }
}

} // namespace

int main() {
    const std::vector<std::pair<std::string, std::function<void()>>> tests{
        {"parameters use ABI registers", testParametersAreCopiedFromAbiRegisters},
        {"user call and return value", testUserFunctionCallUsesRegistersAndCapturesEax},
        {"runtime call import", testRuntimeCallDeclaresExternalSymbol},
        {"six register arguments", testSixArgumentsUseAllRegisterArguments},
        {"reject seven parameters", testMoreThanSixParametersAreRejected},
        {"reject invalid call signature", testInvalidCallSignatureIsRejected},
        {"generator reuse", testGeneratorCanBeReused}
    };

    int failures = 0;

    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
        }
    }

    std::cout << (tests.size() - failures) << "/" << tests.size()
              << " tests passed\n";

    return failures == 0 ? 0 : 1;
}