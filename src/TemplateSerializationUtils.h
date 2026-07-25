#pragma once

#include "SerializationUtils.h"

namespace OdrCop3
{
    template<typename T> requires std::is_same_v<T,ClassTemplateSpecializationDecl       > ||
                                  std::is_same_v<T,ClassTemplatePartialSpecializationDecl> ||
                                  std::is_same_v<T,VarTemplateSpecializationDecl         > ||
                                  std::is_same_v<T,VarTemplatePartialSpecializationDecl  >
    inline std::string TemplateArgsToString(const ContextItems& contextItems, const T* ctd)
    {
        const ASTTemplateArgumentListInfo * ArgsAsWritten = ctd->getTemplateArgsAsWritten();
        llvm::ArrayRef<TemplateArgumentLoc> ArgsRef(ArgsAsWritten->getTemplateArgs(), ArgsAsWritten->NumTemplateArgs);

        std::string              out;
        llvm::raw_string_ostream os(out);
        clang::printTemplateArgumentList(os, ArgsRef, contextItems.printPolicy, nullptr);
        os.flush();

        return out;
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    inline std::string ConstructTemplateParameterList(const ContextItems& contextItems, const clang::TemplateParameterList* params)
    {
        auto AddParameterPackAndNameAndDefaultArgument = [](const ContextItems& contextItems, const auto* tp) -> std::string
                                                         {
                                                             std::string out;
                                                             if (tp->isParameterPack())
                                                                 out += "...";
                                                             if (!tp->getName().empty())
                                                                 out += " " + tp->getName().str();
                                                             if (tp->hasDefaultArgument())
                                                             {
                                                                 std::string defaultStr;
                                                                 llvm::raw_string_ostream defaultStream(defaultStr);
                                                                 tp->getDefaultArgument().getArgument().getAsExpr()->printPretty(defaultStream, nullptr, contextItems.printPolicy);
                                                                 out += "=" + defaultStr;
                                                             }
                                                             return out;
                                                         };

        std::string out = "template<";

        bool first = true;
        for (const clang::NamedDecl* param : *params)
        {
            if (first)
                first = false;
            else
                out += ", ";

            if (const auto* ttp = clang::dyn_cast<clang::TemplateTypeParmDecl>(param))
            {
                out += ttp->wasDeclaredWithTypename() ? "typename" : "class";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp);
                continue;
            }
            if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
            {   // NTTP where type is a struct (new to C++20)
                QualType nttpQT = nttp->getType().getCanonicalType();
                if (NeedsManualSerialization(contextItems, nttpQT))
                {
                    out += IndentBlock(SerializeType(contextItems, nttpQT), LengthOfLastLine(out));
                    out  = TrimRightIf(out, " ");
                }
                else
                {
                    std::string declStr;
                    llvm::raw_string_ostream declStream(declStr);
                    nttp->getType().print(declStream, contextItems.printPolicy);
                    out += declStr;
                }
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, nttp);
                continue;
            }
            if (const auto* ttp2 = clang::dyn_cast<clang::TemplateTemplateParmDecl>(param))
            {
                std::string nested = ConstructTemplateParameterList<SerializeDecl, SerializeType, SerializeAttr>(contextItems, ttp2->getTemplateParameters());
                out += "template " + nested.substr(std::string("template").size()); // tweak the spacing a tiny bit
                out += "class";
                out += AddParameterPackAndNameAndDefaultArgument(contextItems, ttp2);
                continue;
            }
        }
        out += "> ";
        return out;
    }
}