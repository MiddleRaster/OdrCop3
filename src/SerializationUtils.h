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
#include <set>

namespace OdrCop3
{
    struct ContextItems
    {
        ASTContext& context;
        const PrintingPolicy& printPolicy;
        std::unordered_set<const Decl*>& recursingDecls;
        ContextItems(ASTContext* context, const PrintingPolicy& policy, std::unordered_set<const Decl*>& recursingDecls)
            : context       (*context)
            , printPolicy   (policy)
            , recursingDecls(recursingDecls)
        {}
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

    inline std::string MakeUnnamedAndAnonymousConsistent(std::string input)
    {
        struct Replace
        {
            static std::string With(std::string str, const std::string& bad, const std::string& good)
            {
                auto pos = str.find(bad);
                if (pos != std::string::npos)
                    str.replace(pos, bad.size(), good);
                return str;
            }
        };

        input = Replace::With(input, "(unnamed at"         , "(anonymous type at");
        input = Replace::With(input, "(unnamed enum at"    , "(anonymous type at");
        input = Replace::With(input, "(unnamed union at"   , "(anonymous type at");
        input = Replace::With(input, "(anonymous struct at", "(anonymous type at");
        input = Replace::With(input, "(anonymous class at" , "(anonymous type at");
        input = Replace::With(input, "(anonymous union at" , "(anonymous type at");

        return input;
    }

    inline bool IsInAnonymousNamespace(const clang::Decl* decl)
    {
        for (const clang::DeclContext* declContext = decl->getDeclContext(); declContext != nullptr; declContext = declContext->getParent())
        {
            if (const auto* nameSpace = llvm::dyn_cast<clang::NamespaceDecl>(declContext))
                if (nameSpace->isAnonymousNamespace())
                    return true;
        }
        return false;
    }
}