#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "AssemblyGenerator.hpp"
#include "IR.hpp"
#include "IRGenerator.hpp"
#include "IRPrinter.hpp"
#include "Lexer.hpp"
#include "Parser.hpp"
#include "SemanticAnalyser.hpp"
#include "Token.hpp"

/**
 * Usage:
 * ./si_compiler <source_file.si> <output_file.asm>
 */
int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr
            << "Usage: " << argv[0]
            << " <source_file.si> <output_file.asm>\n";

        return 1;
    }

    std::ifstream inputFile(
        argv[1],
        std::ios::binary
    );

    if (!inputFile) {
        std::cerr
            << "Error: could not open source file '"
            << argv[1]
            << "'\n";

        return 1;
    }

    std::ostringstream sourceBuffer;
    sourceBuffer << inputFile.rdbuf();

    if (inputFile.bad()) {
        std::cerr
            << "Error: failed while reading source file '"
            << argv[1]
            << "'\n";

        return 1;
    }

    try {
        Lexer lexer(sourceBuffer.str());
        std::vector<Token> tokens = lexer.tokenise();

        Parser parser(std::move(tokens));
        std::unique_ptr<ProgramNode> ast =
            parser.parseProgram();

        SemanticAnalyser analyser;
        analyser.analyze(*ast);

        IRGenerator irGenerator;
        IRProgram ir = irGenerator.generate(*ast);

        std::cout
            << "--- Intermediate Representation ---\n";
        dumpIR(std::cout, ir);
        std::cout
            << "-----------------------------------\n";

        std::ostringstream assemblyBuffer;

        AssemblyGenerator assemblyGenerator;
        assemblyGenerator.generate(ir, assemblyBuffer);

        std::ofstream assemblyFile(
            argv[2],
            std::ios::binary | std::ios::trunc
        );

        if (!assemblyFile) {
            std::cerr
                << "Error: could not open output file '"
                << argv[2]
                << "'\n";

            return 1;
        }

        assemblyFile << assemblyBuffer.str();
        assemblyFile.close();

        if (!assemblyFile) {
            std::cerr
                << "Error: failed while writing output file '"
                << argv[2]
                << "'\n";

            return 1;
        }

        std::cout
            << "Assembly written to "
            << argv[2]
            << '\n';
    }
    catch (const std::exception& error) {
        std::cerr
            << "Compilation error: "
            << error.what()
            << '\n';

        return 1;
    }

    return 0;
}