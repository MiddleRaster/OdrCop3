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

#include <vector>

//#include <windows.h>

#include "SerializationUtils.h"
#include "magic_enum.h"

namespace OdrCop3
{
    namespace Serialize
    {
        struct Attr
        {
            static std::string Serialize(const ContextItems& contextItems, const clang::AlignedAttr* alignedAttr)
            {
                std::string out;
                if (alignedAttr->isAlignmentExpr())
                {
                    const clang::Expr* expr = alignedAttr->getAlignmentExpr();
                    if (expr && expr->isIntegerConstantExpr(contextItems.context))
                    {
                        auto optInt = expr->getIntegerConstantExpr(contextItems.context);
                        if (optInt.has_value())
                            return "alignas(" + std::to_string(optInt.value().getExtValue()) + ") ";
                    }
                }
                // alignment specified as a type: alignas(SomeType)
                return "alignas(" + alignedAttr->getAlignmentType()->getType().getAsString() + ") ";
            }
            static std::string Serialize(const ContextItems& /*contextItems*/, const clang::WarnUnusedResultAttr* nodiscard)
            {
                const llvm::StringRef msg = nodiscard->getMessage();
                return msg.empty() ? "[[nodiscard]] " : ("[[nodiscard(\"" + msg.str() + "\")]] ");
            }
            static std::string Serialize(const ContextItems& /*contextItems*/, const clang::DeprecatedAttr* deprecated)
            {
                const llvm::StringRef msg = deprecated->getMessage();
                return msg.empty() ? "[[deprecated]] " : ("[[deprecated(\"" + msg.str() + "\")]] ");
            }
            static std::string Serialize(const ContextItems& /*contextItems*/, const clang::FinalAttr* /*Final*/)
            {
                return "final ";
            }
            static std::string Serialize(const ContextItems& contextItems, const clang::Attr* attr)
            {
                std::string raw;
                llvm::raw_string_ostream os(raw);
                attr->printPretty(os, contextItems.printPolicy);
                os.flush();

                // strip off ("")
                constexpr std::string_view empty_parens = "(\"\")";
                if (auto pos = raw.find(empty_parens); pos != std::string::npos)
                    raw.replace(pos, empty_parens.size(), "");
                return raw + " ";
            }
        };

        template<auto SerializeDecl, auto SerializeType>
        static std::string Attrs(const ContextItems& contextItems, const clang::Attr* attr)
        {
            const clang::Decl* decl = nullptr;
            if (decl != nullptr)
                return SerializeDecl(contextItems, decl); // just checking that recursion will compile

            std::string out;
            switch (attr->getKind())
            {
            case clang::attr::Aligned:          if (const auto* alignedAttr = clang::dyn_cast<clang::AlignedAttr         >(attr)) return Attr::Serialize(contextItems, alignedAttr); break;
            case clang::attr::WarnUnusedResult: if (const auto*   nodiscard = clang::dyn_cast<clang::WarnUnusedResultAttr>(attr)) return Attr::Serialize(contextItems, nodiscard);   break;
            case clang::attr::Deprecated:       if (const auto*  deprecated = clang::dyn_cast<clang::DeprecatedAttr      >(attr)) return Attr::Serialize(contextItems, deprecated);  break;
            case clang::attr::Final:            if (const auto*       Final = clang::dyn_cast<clang::FinalAttr           >(attr)) return Attr::Serialize(contextItems, Final);       break;
            case clang::attr::Override:         if (const auto*    override = clang::dyn_cast<clang::OverrideAttr        >(attr)) return Attr::Serialize(contextItems, override);    break;
            case clang::attr::Unused:           if (const auto*      unused = clang::dyn_cast<clang::UnusedAttr          >(attr)) return Attr::Serialize(contextItems, unused);      break;
            case clang::attr::MSInheritance: /* always skip these */                                                              return "";
            default:                         // don't dump everything                                                             return Attr::Serialize(contextItems, attr);
                break;
            };
            throw OdrCop3::UnhandledException(std::string("unhandled attr::getKind: ") + enum_name<clang::attr::Kind,0, 512>(attr->getKind()));
        }
    }
}