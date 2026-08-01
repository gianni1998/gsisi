# gsisi: The Si Compiler — Building a Compiler From Scratch

This repository accompanies [From Source to Machine Code](https://www.biosconfessions.com/posts/from-source-to-machine-code/), a blog post series in which we build **gsisi**, a compiler for the custom **Si programming language**, from scratch.

Each blog post has its own Git branch containing the complete source code as it exists at the end of that post. Branch names match the post slugs, making it easy to follow the project step by step or jump directly to a specific compiler stage.

## 📚 Chapters

| #  | Title | Branch |
| -- | ----- | ------ |
| 01 | [How Compilers Work: Source Code to Machine Code]((https://www.biosconfessions.com/posts/from-source-to-machine-code/1-the-black-box/) | - |
| 02 | [Design a Programming Language and Compiler in C++]((https://www.biosconfessions.com/posts/from-source-to-machine-code/2-language-design/) | [2-language-design](https://github.com/gianni1998/gsisi/tree/2-language-design) |
| 03 | [Build a Lexer in C++: Tokens and Lexical Analysis]((https://www.biosconfessions.com/posts/from-source-to-machine-code/3-building-a-lexer/) | [3-building-a-lexer](https://github.com/gianni1998/gsisi/tree/3-building-a-lexer) |
| 04 | [Build a Recursive Descent Parser and AST in C++]((https://www.biosconfessions.com/posts/from-source-to-machine-code/4-recursive-descent-parser/) | [4-recursive-descent-parser](https://github.com/gianni1998/gsisi/tree/4-recursive-descent-parser) |
| 05 | [Semantic Analysis in C++: Symbols, Scopes and Types]((https://www.biosconfessions.com/posts/from-source-to-machine-code/5-semantic-analysis/) | [5-semantic-analysis](https://github.com/gianni1998/gsisi/tree/5-semantic-analysis) |
| 06 | [Build a Compiler IR in C++: Three-Address Code]((https://www.biosconfessions.com/posts/from-source-to-machine-code/6-intermediate-representation/) | [6-intermediate-representation](https://github.com/gianni1998/gsisi/tree/6-intermediate-representation) |
| 07 | [Build a Compiler Backend: Generate x86-64 Assembly]((https://www.biosconfessions.com/posts/from-source-to-machine-code/7-backend-begins/) | [7-backend-begins](https://github.com/gianni1998/gsisi/tree/7-backend-begins) |
| 08 | [Compiler Function Calls: System V ABI and Runtime]((https://www.biosconfessions.com/posts/from-source-to-machine-code/8-runtime/) | [8-runtime](https://github.com/gianni1998/gsisi/tree/8-runtime) |
| 09 | [Compiler Control Flow: x86-64 Branches and Loops]((https://www.biosconfessions.com/posts/from-source-to-machine-code/9-control-flow/) | [9-control-flow](https://github.com/gianni1998/gsisi/tree/9-control-flow) |
| 10 | [Compiler Optimisation in C++: Folding and Dead Code]((https://www.biosconfessions.com/posts/from-source-to-machine-code/10-optimisation/) | [10-optimisation](https://github.com/gianni1998/gsisi/tree/10-optimisation) |

> Posts 1 and 2 introduce the compiler pipeline, language design, and project setup, so they do not yet produce a compiler executable.

## ✅ Prerequisites

To follow along, you need:

- Git
- Docker
- A text editor such as [Visual Studio Code](https://code.visualstudio.com/)

## 🌿 Choose a Branch

Clone the repository and switch to the branch matching the blog post you are following:

```bash
git clone <repository-url>
cd gsisi
git branch -a
git switch 3-building-a-lexer
```

## 🔨 Build

Start the Docker development environment:

```bash
./scripts/dev-shell.sh
```

Then build gsisi from inside the Docker shell:

```bash
./scripts/build.sh
```

## ▶️ Run

The command changes as the compiler develops throughout the series.

Posts 3–6 analyse a Si source file:

```bash
./build/si_compiler path/to/program.si
```

Post 7 generates an x86-64 assembly file:

```bash
./build/si_compiler path/to/program.si build/program.asm
```

From post 8 onward, gsisi can build and run a native executable:

```bash
./build/si_compiler examples/sum-to-ten.si build/sum-to-ten
./build/sum-to-ten
```

## 🎓 Purpose

The **Si language** and **gsisi compiler** are educational projects created to explain how source code is transformed into a native executable without relying on LLVM or a parser generator.
