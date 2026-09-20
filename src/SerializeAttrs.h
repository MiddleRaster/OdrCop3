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

#include "SerializationUtils.h"
#include "magic_enum.h"

namespace OdrCop3
{
    namespace Serialize
    {
        inline static std::string Print(const ContextItems& contextItems, const clang::Attr* attr)
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
        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> struct Attr
        {
            static std::string Serialize(const ContextItems& contextItems, const clang::AlignedAttr* alignedAttr)
            {
                std::string out = "alignas(";
                if (alignedAttr->isAlignmentExpr())
                    out += IndentBlock(SerializeExpr(contextItems, alignedAttr->getAlignmentExpr()           ), LengthOfLastLine(out));
                else
                    out += IndentBlock(SerializeType(contextItems, alignedAttr->getAlignmentType()->getType()), LengthOfLastLine(out));
                return out + ") ";
            }
            //static std::string Serialize(const ContextItems& /*contextItems*/, const clang::WarnUnusedResultAttr* nodiscard)
            //{
            //    const llvm::StringRef msg = nodiscard->getMessage();
            //    return "[[nodiscard(\"" + msg.str() + "\")]] ";
            //}
            //static std::string Serialize(const ContextItems& /*contextItems*/, const clang::DeprecatedAttr* deprecated)
            //{
            //    const llvm::StringRef msg = deprecated->getMessage();
            //    return "[[deprecated(\"" + msg.str() + "\")]] ";
            //}
            //static std::string Serialize(const ContextItems& /*contextItems*/, const clang::FinalAttr* /*Final*/) { return "final "; }
            static std::string Serialize(const ContextItems& contextItems, const clang::Attr* attr) { return Print(contextItems, attr); }
        };

        template<auto SerializeDecl, auto SerializeType, auto SerializeExpr>
        inline static std::string Attrs(const ContextItems& contextItems, const clang::Attr* attr)
        {
            struct Can
            {
                static bool Print(const ContextItems& contextItems, const clang::Attr* attr)
                {
                    switch (attr->getKind())
                    {
                    case clang::attr::Visibility:      // Windows doesn't have this concept
                    case clang::attr::ReturnsNonNull:  // or this one
                    case clang::attr::AllocSize:
                    case clang::attr::MSInheritance:   // always skip these
                        return false;
                    default: break;
                    }
                    return !NeedsManualSerialization(contextItems, attr);
                }
            };

            SerializationNeeds serializationNeeds
            {
                .isAnonymous           = !Can::Print(contextItems, attr),
                .hasNamespaceAlias     = false, // for now
                .hasInternalLinkageRef = nullptr != InternalLinkageRefFinder::FindReference(attr)
            };
            if (serializationNeeds.AreAllFalse())
                return Print(contextItems, attr);

            switch (attr->getKind())
            {
            case clang::attr::Aligned:          if (const auto* alignedAttr = clang::dyn_cast<clang::AlignedAttr         >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), alignedAttr); break;
            //case clang::attr::WarnUnusedResult: if (const auto*   nodiscard = clang::dyn_cast<clang::WarnUnusedResultAttr>(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), nodiscard);   break;
            //case clang::attr::Deprecated:       if (const auto*  deprecated = clang::dyn_cast<clang::DeprecatedAttr      >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), deprecated);  break;
            //case clang::attr::Final:            if (const auto*       Final = clang::dyn_cast<clang::FinalAttr           >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), Final);       break;
            //case clang::attr::Override:         if (const auto*    override = clang::dyn_cast<clang::OverrideAttr        >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), override);    break;
            //case clang::attr::Unused:           if (const auto*      unused = clang::dyn_cast<clang::UnusedAttr          >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), unused);      break;
            //case clang::attr::ConstInit:        if (const auto*   constInit = clang::dyn_cast<clang::ConstInitAttr       >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), constInit);   break;
            //case clang::attr::NoUniqueAddress:  if (const auto*         nua = clang::dyn_cast<clang::NoUniqueAddressAttr >(attr)) return Attr<SerializeDecl, SerializeType, SerializeExpr>::Serialize(contextItems.withSerializationNeeds(serializationNeeds), nua);         break;

            // filtering out all of those attributes below here:
            case clang::attr::Visibility:      // Windows doesn't have this concept
            case clang::attr::ReturnsNonNull:  // or this one
            case clang::attr::AllocSize:
            case clang::attr::MSInheritance:   // always skip these
                return "";
            default: // turn this on later: 
                // return Serialize::Print(contextItems, attr);
                break;
            };
            throw OdrCop3::UnhandledException(std::string("unhandled attr::getKind: ") + enum_name<clang::attr::Kind,0, 512>(attr->getKind()));
        }
    }
}