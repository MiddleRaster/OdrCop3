#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
#include <clang\Tooling\CompilationDatabase.h>
#include <clang\AST\Mangle.h>

#include <clang\AST\Decl.h>
#include <clang\AST\GlobalDecl.h>
#include <clang\AST\RecordLayout.h>
#include <llvm\Support\raw_ostream.h>

#include <exception>
#include <string>
#include <sstream>

namespace OdrCop3
{
    struct ContextItems
    {
        ASTContext& context;
        const PrintingPolicy& printPolicy;
        ContextItems(ASTContext* context, const PrintingPolicy& policy) : context(*context), printPolicy(policy) {}
    };

	struct UnhandledException : public std::exception
	{
		const std::string message;
        UnhandledException(const std::string& msg) : message(msg) {}
		const char* what() const noexcept override { return message.c_str(); }
	};

    inline std::string IndentBlock(const std::string& block, size_t indentWidth, const std::string& firstLinePrefix = "")
    {
        std::istringstream iss(block);
        std::string indentation(indentWidth, ' ');
        std::string out;
        bool first = true;
        for (std::string line; std::getline(iss, line);)
        {
            if (first) {
                first = false;
                out  += firstLinePrefix + line + "\n";
            } else
                out  += indentation + line + "\n";
        }
        return out;
    }
}