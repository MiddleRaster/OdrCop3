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
#include <algorithm>

namespace OdrCop3
{
    struct ContextItems
    {
        ASTContext& context;
        const PrintingPolicy& printPolicy;
        const std::string& TU;
        std::unordered_set<const Decl*>& recursingDecls;
        std::string aux;
        bool wantFunctionBody = true;
        bool needsFriend      = false;
        ContextItems(ASTContext* context, const PrintingPolicy& policy, const std::string& TU, std::unordered_set<const Decl*>& recursingDecls, const std::string& aux="")
            : context       (*context)
            , printPolicy   (policy)
            , TU            (TU)
            , recursingDecls(recursingDecls)
            , aux           (aux)
        {}
    };

	struct UnhandledException : public std::exception
	{
		const std::string message;
        UnhandledException(const std::string& msg) : message(msg) {}
		const char* what() const noexcept override { return message.c_str(); }
	};

    inline std::string TrimRightIf(std::string out, const std::string& what)
    {
        if (out.ends_with(what))
            out = out.substr(0, out.size()-what.size());
        return out;
    }
    inline size_t LengthOfLastLine(const std::string& out) { return out.size() - (out.rfind('\n')+1); }
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
        return TrimRightIf(out, "\n");
    }

    inline std::string MakeUnnamedAndAnonymousConsistent(std::string input)
    {
        auto Replace = [](std::string& str, std::string_view bad, std::string_view good) { if (auto pos = str.find(bad); pos != std::string::npos) str.replace(pos, bad.size(), good); };
        Replace(input, "(unnamed at",          "(anonymous type at");
        Replace(input, "(unnamed enum at",     "(anonymous type at");
        Replace(input, "(unnamed union at",    "(anonymous type at");
        Replace(input, "(anonymous struct at", "(anonymous type at");
        Replace(input, "(anonymous class at",  "(anonymous type at");
        Replace(input, "(anonymous union at",  "(anonymous type at");
        return input;
    }


    template<typename PrintLambda> inline bool NeedsManualSerialization(const ContextItems& contextItems, PrintLambda print)
    {
        clang::PrintingPolicy policy = contextItems.printPolicy;
        policy.FullyQualifiedName = true;

        std::string str;
        llvm::raw_string_ostream os(str);
        print(os, policy);
        os.flush();

        return str.find("(anonymous namespace)") != std::string::npos;
    }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, QualType qt                                              ) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) {                     qt.print(os,                       policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::TemplateParameterList* templateParameterList) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) { templateParameterList->print(os, contextItems.context, policy); }); }
    
    inline std::string PostProcessBody(const std::string& originalBody)
    {
        std::string body(originalBody);

        // collapse to "{}" if there's no real content
        bool allWhitespace = true;
        for (char c : body) {
            if ((c == '{') ||
                (c == '}'))
                continue;
            if (!std::isspace(static_cast<unsigned char>(c))) {
                allWhitespace = false;
                break;
            }
        }
        if (allWhitespace)
            return "{}";

        // if one-liner (by counting semicolons)
        if (std::count(body.begin(), body.end(), ';') < 2) {
            size_t pos = 0;
            while ((pos = body.find("\n", pos)) != std::string::npos)
                body.replace(pos, 1, " ");

            pos = 0;
            while ((pos = body.find("  ", pos)) != std::string::npos)
                body.replace(pos, 2, " ");

            while (body.ends_with(' '))
                body = body.substr(0, body.size() - 1); // strip off last ' '
        }
        return TrimRightIf(body, "\n");
    }

    inline std::string GetExceptionSpecifier(const ContextItems& contextItems, const FunctionProtoType* functionProtoType, const FunctionDecl* functionDecl)
    {
        switch (functionProtoType->getExceptionSpecType())
        {
        case EST_BasicNoexcept:
            if (functionDecl == nullptr)
                return "noexcept ";
            if (functionDecl->getExceptionSpecSourceRange().isValid()) // only if the user actually wrote this (i.e., not "inferred" by the compiler)
                return "noexcept ";
            break;
        case EST_DependentNoexcept:
        {
            std::string exprStr;
            llvm::raw_string_ostream os(exprStr);
            functionProtoType->getNoexceptExpr()->printPretty(os, nullptr, contextItems.printPolicy);
            os.flush();
            return "noexcept(" + exprStr + ") ";
        }
        case EST_Dynamic:
        {
            std::string result = "throw(";
            bool        first = true;
            for (QualType t : functionProtoType->exceptions())
            {
                if (!first)
                    result += ", ";
                result += t.getAsString(contextItems.printPolicy);
                first = false;
            }
            return result + ") ";
        }
        case EST_NoexceptTrue:  return "noexcept(true) ";
        case EST_NoexceptFalse: return "noexcept(false) ";
        case EST_DynamicNone:   return "throw() ";
        case EST_MSAny:         return "throw(...) ";
        case EST_None:
        default:
            break;
        }
        return "";
    }
}