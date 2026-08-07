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

void requireContains(
    const std::string& text,
    const std::string& expected
) {
    require(
        text.find(expected) != std::string::npos,
        "Expected generated assembly to contain: " + expected
    );
}

void requireThrows(
    const std::function<void()>& action,
    const std::string& expectedMessage
) {
    try {
        action();
    } catch (const std::exception& error) {
        requireContains(error.what(), expectedMessage);
        return;
    }

    throw std::runtime_error("Expected an exception");
}

IRInstruction constInt(const std::string& dst, int value) {
    return IRInstruction{
        .op = IROp::ConstInt,
        .type = Type::Int,
        .dst = dst,
        .intValue = value
    };
}

IRInstruction comparison(
    IROp operation,
    const std::string& dst,
    const std::string& left,
    const std::string& right
) {
    return IRInstruction{
        .op = operation,
        .type = Type::Bool,
        .dst = dst,
        .args = {left, right}
    };
}

IRInstruction returnValue(const std::string& value) {
    return IRInstruction{
        .op = IROp::Return,
        .args = {value}
    };
}

IRProgram comparisonProgram() {
    IRFunction main;
    main.name = "main";
    main.returnType = Type::Int;

    IRBlock entry;
    entry.name = "entry";
    entry.instructions = {
        constInt("%left", -2),
        constInt("%right", 3),
        comparison(IROp::Equal, "%eq", "%left", "%right"),
        comparison(IROp::NotEqual, "%ne", "%left", "%right"),
        comparison(IROp::Less, "%lt", "%left", "%right"),
        comparison(IROp::LessEqual, "%le", "%left", "%right"),
        comparison(IROp::Greater, "%gt", "%left", "%right"),
        comparison(IROp::GreaterEqual, "%ge", "%left", "%right"),
        constInt("%result", 0),
        returnValue("%result")
    };

    main.blocks.push_back(std::move(entry));

    IRProgram program;
    program.functions.push_back(std::move(main));
    return program;
}

IRProgram branchProgram() {
    IRFunction main;
    main.name = "main";
    main.returnType = Type::Int;

    IRBlock entry;
    entry.name = "entry";
    entry.instructions = {
        IRInstruction{
            .op = IROp::ConstBool,
            .type = Type::Bool,
            .dst = "%condition",
            .boolValue = true
        },
        IRInstruction{
            .op = IROp::Branch,
            .args = {"%condition"},
            .trueTarget = "then",
            .falseTarget = "else"
        }
    };

    IRBlock thenBlock;
    thenBlock.name = "then";
    thenBlock.instructions = {
        IRInstruction{
            .op = IROp::Jump,
            .target = "end"
        }
    };

    IRBlock elseBlock;
    elseBlock.name = "else";
    elseBlock.instructions = {
        IRInstruction{
            .op = IROp::Jump,
            .target = "end"
        }
    };

    IRBlock end;
    end.name = "end";
    end.instructions = {
        constInt("%result", 0),
        returnValue("%result")
    };

    main.blocks.push_back(std::move(entry));
    main.blocks.push_back(std::move(thenBlock));
    main.blocks.push_back(std::move(elseBlock));
    main.blocks.push_back(std::move(end));

    IRProgram program;
    program.functions.push_back(std::move(main));
    return program;
}

IRProgram loopProgram() {
    IRFunction main;
    main.name = "main";
    main.returnType = Type::Int;

    IRBlock entry;
    entry.name = "entry";
    entry.instructions = {
        IRInstruction{
            .op = IROp::Jump,
            .target = "while_cond"
        }
    };

    IRBlock condition;
    condition.name = "while_cond";
    condition.instructions = {
        IRInstruction{
            .op = IROp::ConstBool,
            .type = Type::Bool,
            .dst = "%condition",
            .boolValue = true
        },
        IRInstruction{
            .op = IROp::Branch,
            .args = {"%condition"},
            .trueTarget = "while_body",
            .falseTarget = "while_end"
        }
    };

    IRBlock body;
    body.name = "while_body";
    body.instructions = {
        IRInstruction{
            .op = IROp::Jump,
            .target = "while_cond"
        }
    };

    IRBlock end;
    end.name = "while_end";
    end.instructions = {
        constInt("%result", 0),
        returnValue("%result")
    };

    main.blocks.push_back(std::move(entry));
    main.blocks.push_back(std::move(condition));
    main.blocks.push_back(std::move(body));
    main.blocks.push_back(std::move(end));

    IRProgram program;
    program.functions.push_back(std::move(main));
    return program;
}

std::string generate(IRProgram program) {
    AssemblyGenerator generator;
    std::ostringstream output;
    generator.generate(program, output);
    return output.str();
}

void testSignedComparisonConditionCodes() {
    const std::string assembly = generate(comparisonProgram());

    requireContains(assembly, "sete al");
    requireContains(assembly, "setne al");
    requireContains(assembly, "setl al");
    requireContains(assembly, "setle al");
    requireContains(assembly, "setg al");
    requireContains(assembly, "setge al");
    requireContains(assembly, "movzx eax, al");
}

void testBranchUsesFunctionScopedLabels() {
    const std::string assembly = generate(branchProgram());

    requireContains(assembly, ".L_main_then:");
    requireContains(assembly, ".L_main_else:");
    requireContains(assembly, ".L_main_end:");
    requireContains(assembly, "jne .L_main_then");
    requireContains(assembly, "jmp .L_main_else");
    requireContains(assembly, "jmp .L_main_end");
}

void testLoopContainsBackEdge() {
    const std::string assembly = generate(loopProgram());

    requireContains(assembly, ".L_main_while_cond:");
    requireContains(assembly, ".L_main_while_body:");
    requireContains(assembly, ".L_main_while_end:");
    requireContains(assembly, "jmp .L_main_while_cond");
}

void testUnknownJumpTargetIsRejected() {
    IRProgram program = branchProgram();
    program.functions[0].blocks[1].instructions[0].target = "missing";

    requireThrows(
        [&] { generate(std::move(program)); },
        "targets unknown block 'missing'"
    );
}

void testDuplicateBlockNameIsRejected() {
    IRProgram program = branchProgram();
    program.functions[0].blocks[1].name = "entry";

    requireThrows(
        [&] { generate(std::move(program)); },
        "duplicate block 'entry'"
    );
}

void testInstructionAfterTerminatorIsRejected() {
    IRProgram program = branchProgram();
    auto& entry = program.functions[0].blocks[0].instructions;
    entry.push_back(constInt("%late", 99));

    requireThrows(
        [&] { generate(std::move(program)); },
        "terminator in block 'entry' is not the final instruction"
    );
}

void testGeneratorCanBeReused() {
    AssemblyGenerator generator;

    std::ostringstream first;
    IRProgram branches = branchProgram();
    generator.generate(branches, first);

    std::ostringstream second;
    IRProgram comparisons = comparisonProgram();
    generator.generate(comparisons, second);

    requireContains(first.str(), "jne .L_main_then");
    requireContains(second.str(), "setl al");
}

} // namespace

int main() {
    using Test = std::pair<std::string, std::function<void()>>;

    const std::vector<Test> tests{
        {"signed comparison condition codes", testSignedComparisonConditionCodes},
        {"function-scoped branch labels", testBranchUsesFunctionScopedLabels},
        {"loop back edge", testLoopContainsBackEdge},
        {"reject unknown jump target", testUnknownJumpTargetIsRejected},
        {"reject duplicate block name", testDuplicateBlockNameIsRejected},
        {"reject instruction after terminator", testInstructionAfterTerminatorIsRejected},
        {"generator reuse", testGeneratorCanBeReused}
    };

    int failures = 0;

    for (const auto& [name, test] : tests) {
        try {
            test();
            std::cout << "[PASS] " << name << '\n';
        } catch (const std::exception& error) {
            ++failures;
            std::cerr << "[FAIL] " << name
                      << ": " << error.what() << '\n';
        }
    }

    std::cout << (tests.size() - failures) << "/"
              << tests.size() << " tests passed\n";

    return failures == 0 ? 0 : 1;
}