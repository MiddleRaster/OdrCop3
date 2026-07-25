#pragma once

#include "SerializationUtils.h"

namespace OdrCop3
{
    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    inline std::string TemplateArgsToString(const ContextItems& contextItems, const clang::TemplateArgumentList& args, const clang::TemplateParameterList* params, bool wantAnonymousNamespaceWithTU=false)
    {
        std::string out;
        out += "<";

        for (unsigned i=0; i<args.size(); ++i)
        {
            if (i > 0)
                out += ", ";

            bool prependAddressOf = false;
            const clang::NamedDecl * paramDecl = params->getParam(i);
            if (const auto* nttp = dyn_cast<NonTypeTemplateParmDecl>(paramDecl); nttp)
            {
                QualType paramType = nttp->getType();
                if (paramType->isPointerType())
                    prependAddressOf = true;
            }

            const clang::TemplateArgument& arg = args[i];
            switch (arg.getKind())
            {
            case clang::TemplateArgument::Integral:
            { // if it's actually an enum value, try to display the enum
                const auto* enumTy = arg.getIntegralType()->getAs<clang::EnumType>();
                if (enumTy) {
                    llvm::APSInt val = arg.getAsIntegral();
                    for (const auto* ecd : enumTy->getDecl()->enumerators()) {
                        if (ecd->getInitVal() == val) {
                            out += SerializeDecl(contextItems, enumTy->getDecl()) + "::" + ecd->getNameAsString();
                            break;
                        }
                    }
                } else
                    out += llvm::toString(arg.getAsIntegral(), 10);
                break;
            }
            case clang::TemplateArgument::Declaration:
            {
                if (prependAddressOf == true)
                    out += "&";
                
                // Clang has a weird way of specifying when it's a reference
                QualType argType     = arg.getAsDecl()->getType();
                bool isPointer       = argType->isPointerType();
                bool isMemberPointer = argType->isMemberPointerType(); // already encoded in the type
                bool isReference     = !isPointer && !isMemberPointer;
                out += " ";
                if (isPointer)
                    out += "* ";
                else if (isReference)
                    out += "& ";
                 
                const  ValueDecl* valueDecl = arg.getAsDecl();
                out += valueDecl->getQualifiedNameAsString();
                break;
            }
            case clang::TemplateArgument::NullPtr:           out += "nullptr";                                                                                    break;
            case clang::TemplateArgument::Null:              out += "null";                                                                                       break;
            case clang::TemplateArgument::Template:          out += arg.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString();                          break;
            case clang::TemplateArgument::TemplateExpansion: out += arg.getAsTemplateOrTemplatePattern().getAsTemplateDecl()->getQualifiedNameAsString() + "..."; break;
            case clang::TemplateArgument::Type:
            {
                const auto* rd = arg.getAsType()->getAsCXXRecordDecl();
                if (rd && rd->isInAnonymousNamespace())
                {
                    std::string line = IndentBlock(SerializeDecl(contextItems, dyn_cast<CXXRecordDecl>(rd)), LengthOfLastLine(out));
                    if (wantAnonymousNamespaceWithTU)
                    {
                        const std::string from = "anonymous namespace";
                        if (auto pos = line.find(from); pos != std::string::npos)
                            line.replace(pos, from.size(), "anonymous namespace in " + contextItems.TU);
                    }
                    out += line;
                    out = TrimRightIf(out, ";\n");
                } else
                {
                    // was:
                    // out += arg.getAsType().getAsString(contextItems.printPolicy);
                    // but that gives type-parameter-0-0, for example. Do it manually (recursively, too).

                    auto printType = [&](auto &&self, clang::QualType qt) -> std::string
                    {
                        qt = qt.getCanonicalType();
                        const clang::Type *ty = qt.getTypePtr();

                        if (auto *            pointerType = llvm::dyn_cast<clang::PointerType            >(ty)) return self(self, pointerType->getPointeeType()) + "*";
                        if (auto *          referenceType = llvm::dyn_cast<clang::ReferenceType          >(ty)) return self(self, referenceType->getPointeeType()) + (referenceType->isRValueReferenceType() ? "&&" : "&");
                        if (auto *      constantArrayType = llvm::dyn_cast<clang::ConstantArrayType      >(ty)) return self(self, constantArrayType->getElementType()) + "[" + std::to_string(constantArrayType->getSize().getZExtValue()) + "]";
                        if (auto *      variableArrayType = llvm::dyn_cast<clang::VariableArrayType      >(ty)) return self(self, variableArrayType->getElementType()) + "[]";
                        if (auto *    incompleteArrayType = llvm::dyn_cast<clang::IncompleteArrayType    >(ty)) return self(self, incompleteArrayType->getElementType()) + "[]";
                        if (auto *dependentSizedArrayType = llvm::dyn_cast<clang::DependentSizedArrayType>(ty)) return self(self, dependentSizedArrayType->getElementType()) + "[]";
                        if (auto *  templateTypeParamType = llvm::dyn_cast<clang::TemplateTypeParmType   >(ty)) {
                            const clang::NamedDecl *paramDecl = params->getParam(templateTypeParamType->getIndex());
                            return paramDecl->getNameAsString();
                        }

                        std::string s;
                        llvm::raw_string_ostream os(s);
                        qt.print(os, contextItems.printPolicy); // if none of the above
                        os.flush();
                        return s;
                    };

                    out += printType(printType, arg.getAsType());
                }
                break;
            }
            case clang::TemplateArgument::Expression:
            {
                std::string s;
                llvm::raw_string_ostream os(s);
                arg.getAsExpr()->printPretty(os, nullptr, contextItems.printPolicy);
                out += os.str();
                break;
            }
            case clang::TemplateArgument::Pack:
            {
                out += "{";
                auto pack = arg.pack_elements();
                for (unsigned j = 0; j < pack.size(); ++j) {
                    if (j > 0)
                        out += ", ";

                    // Inline handling for pack elements
                    const clang::TemplateArgument& pe = pack[j];
                    switch (pe.getKind())
                    {
                    case clang::TemplateArgument::Type:              out += pe.getAsType().getAsString();                                                                break;
                    case clang::TemplateArgument::Integral:          out += llvm::toString(pe.getAsIntegral(), 10);                                                      break;
                    case clang::TemplateArgument::NullPtr:           out += "nullptr";                                                                                   break;
                    case clang::TemplateArgument::Declaration:       out += pe.getAsDecl()->getQualifiedNameAsString();                                                  break;
                    case clang::TemplateArgument::Null:              out += "null";                                                                                      break;
                    case clang::TemplateArgument::Template:          out += pe.getAsTemplate().getAsTemplateDecl()->getQualifiedNameAsString();                          break;
                    case clang::TemplateArgument::TemplateExpansion: out += pe.getAsTemplateOrTemplatePattern().getAsTemplateDecl()->getQualifiedNameAsString() + "..."; break;
                    case clang::TemplateArgument::Expression:
                    {
                        std::string s;
                        llvm::raw_string_ostream os(s);
                        pe.getAsExpr()->printPretty(os, nullptr, contextItems.printPolicy);
                        out += os.str();
                        break;
                    }
                    default:
                    {
                        std::string tmp;
                        llvm::raw_string_ostream os(tmp);
                        pe.print(contextItems.printPolicy, os, true);
                        out += os.str();
                        break;
                    }
                    }
                }
                out += "}";
                break;
            }
            default:
            {
                std::string tmp;
                llvm::raw_string_ostream os(tmp);
                arg.print(contextItems.printPolicy, os, true);
                out += os.str();
                break;
            }
            }
        }
        out += ">";
        return out;
    }

    template<typename T> requires std::is_same_v<T,ClassTemplateSpecializationDecl> || std::is_same_v<T,ClassTemplatePartialSpecializationDecl>
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