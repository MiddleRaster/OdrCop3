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

        if (str.find("(anonymous namespace)") != std::string::npos)
            return true;
        return false;
    }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, QualType qt                                              ) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) {                     qt.print      (os,                       policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::TemplateParameterList* templateParameterList) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) { templateParameterList->print      (os, contextItems.context, policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::Expr                 *                  expr) { return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) {                  expr->printPretty(os, nullptr             , policy); }); }
    inline bool NeedsManualSerialization(const ContextItems& contextItems, const clang::Decl                 *                  decl)
    {
        if (decl->isInAnonymousNamespace())
            return true; // print() often doesn't print "(anonymous namespace)"
        return NeedsManualSerialization(contextItems, [&](llvm::raw_ostream& os, const clang::PrintingPolicy& policy) { decl->print(os, policy); });
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
    inline std::string GetExceptionSpecifier(const ContextItems& contextItems, const FunctionProtoType* functionProtoType, const FunctionDecl* functionDecl)
    {
        switch (functionProtoType->getExceptionSpecType())
        {
        case EST_Dynamic:
        {
            std::string result = "throw(";
            bool first = true;
            for (QualType t : functionProtoType->exceptions())
            {
                if (first)
                    first = false;
                else
                    result += ", ";
                result += t.getAsString(contextItems.printPolicy);
            }
            return result + ") ";
        }
        case EST_DependentNoexcept: return "noexcept(" + IndentBlock(SerializeExpr(contextItems, functionProtoType->getNoexceptExpr()), 9) + ") ";
        case EST_BasicNoexcept:     return "noexcept ";
        case EST_NoexceptTrue:      return "noexcept(true) ";
        case EST_NoexceptFalse:     return "noexcept(false) ";
        case EST_DynamicNone:       return "throw() ";
        case EST_MSAny:             return "throw(...) ";
        case EST_None:
        default:
            break;
        }
        return "";
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class TemplateDeclBaseSerializer
    {
        const ContextItems& contextItems;

        static size_t GetIndentation(const std::string& prefix, const std::string& block)
        {
            // idea: examine block. 
            // If it is multi-line AND the second line starts with more than 4 spaces, then we know that some "(anonymous namespace)" type was serialized inline.
            // In that case, we need to indent by an extra prefix.size(); otherwise, we can just concatenate it or indent by 0.

            size_t indent = 0;
            auto pos = block.find("\n");
            if (pos != std::string::npos)
                if (block.size() > pos+1 + 5) // 1 to get past "\n" and 5 for 5 spaces
                    if (0 == block.compare(pos + 1, 5, "     "))
                        indent = prefix.size();

            return indent;
        }
    public:
        TemplateDeclBaseSerializer(const ContextItems& contextItems) : contextItems(contextItems) {}
        std::string Serialize(const clang::TemplateParameterList* templateParameterList, const clang::Decl* decl) const
        {
            std::string prefix = GetTemplateHeader<SerializeDecl, SerializeType, SerializeExpr>(contextItems, templateParameterList);
            std::string block  = SerializeDecl(contextItems, decl);
            return prefix + IndentBlock(block, GetIndentation(prefix, block)) + "\n";
        }
    };

    inline std::string SnugUpPointersAndReferences(const std::string& str)
    {
        if (!str.ends_with("*")) // e.g., "void *" gets no space
        if (!str.ends_with("&")) // e.g., ditto &
        if (!str.ends_with(" ")) // certainly don't want two spaces in a row
            return " ";          // e.g., "int" does
        return "";
    }

    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> inline std::string SerializeTemplateArgument(const ContextItems& contextItems, const TemplateArgument& arg, size_t indent)
    {
        switch (arg.getKind())
        {
        case clang::TemplateArgument::Type       : return TrimRightIf(IndentBlock(SerializeType(contextItems, arg.getAsType()), indent), ";");
        case clang::TemplateArgument::Expression : return             IndentBlock(SerializeExpr(contextItems, arg.getAsExpr()), indent);
        case clang::TemplateArgument::Declaration: return             IndentBlock(SerializeDecl(contextItems, arg.getAsDecl()), indent);
        default:
            break;
        }

        std::string argStr;
        llvm::raw_string_ostream os(argStr);
        arg.print(contextItems.printPolicy, os, true);
        os.flush();
        return argStr;
    }

    inline bool IsEventuallyArrayOrFunctionProtoType(QualType qt)
    {
        if (const auto* pointerType = qt->getAs<PointerType>())
            return IsEventuallyArrayOrFunctionProtoType(pointerType->getPointeeType());

        if (const auto* referenceType = qt->getAs<ReferenceType>())
            return IsEventuallyArrayOrFunctionProtoType(referenceType->getPointeeType());

        if (qt->isArrayType())
            return true;
        if (qt->isFunctionProtoType())
            return true;

        return false;
    }
}