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

                const ValueDecl* vd   = arg.getAsDecl();
                // IndirectionCvStripper ics(vd->getType().getCanonicalType());
                // QualType baseQT       = ics.GetBaseType();
                // const Type* baseTy    = baseQT.getTypePtr();
                // if (const auto* recTy = dyn_cast<RecordType>(baseTy))
                {
                //     const auto* recDecl = recTy->getDecl();
                //     if (recDecl->isInAnonymousNamespace())
                    {
                //         out += ics.ConstructPrefix();
                //         out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recDecl)), out.size() - (out.rfind('\n')+1));
                //         out  = out.substr(0, out.size()-2); // strip ";\n"
                //         out += ics.ConstructPointersAndReferences();

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
                    }
                } 
                out += vd->getQualifiedNameAsString();
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
                    std::string line = IndentBlock(SerializeDecl(contextItems, dyn_cast<CXXRecordDecl>(rd)), out.size());
                    if (wantAnonymousNamespaceWithTU)
                    {
                        const std::string from = "anonymous namespace";
                        if (auto pos = line.find(from); pos != std::string::npos)
                            line.replace(pos, from.size(), "anonymous namespace in " + contextItems.TU);
                    }
                    out += line;
                    out  = out.substr(0, out.size()-2); // strip last ";\n"
                } else
                    out += arg.getAsType().getAsString(contextItems.printPolicy);
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

    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    inline std::string TemplateArgsToString(const ContextItems& contextItems, const clang::ClassTemplateSpecializationDecl* ctsd, bool wantAnonymousNamespaceWithTU=false)
    {
        const clang::TemplateArgumentList & args   = ctsd->getTemplateArgs();
        const clang::TemplateParameterList* params = ctsd->getSpecializedTemplate()->getTemplateParameters();
        return TemplateArgsToString<SerializeDecl, SerializeType, SerializeAttr>(contextItems, args, params);
    }

    template <auto SerializeDecl, auto SerializeType, auto SerializeAttr>
    inline std::string ConstructTemplateParameterList(const ContextItems& contextItems, const clang::TemplateParameterList* params)
    {
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
                if (!ttp->getName().empty())
                    out += " " + ttp->getName().str();

                if (ttp->hasDefaultArgument())
                {
                    clang::QualType defaultType = ttp->getDefaultArgument().getArgument().getAsType();
                    out += "=";
                    out += defaultType.getAsString(contextItems.printPolicy);
                }
                continue;
            }
            if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
            {
                bool handled = false;

                const auto* enumTy = dyn_cast<clang::EnumType>(nttp->getType().getTypePtr());
                if (enumTy && enumTy->getDecl()->isInAnonymousNamespace()) {
                    out += SerializeDecl(contextItems, enumTy->getDecl());
                    if (!nttp->getName().empty())
                        out += " " + nttp->getName().str();
                    handled = true;
                }

                if (handled == false)
                {   // NTTP where type is a struct (new to C++20)

                    QualType nttpQT = nttp->getType().getCanonicalType();

                 // IndirectionCvStripper ics(nttpQT);
                 // QualType    baseQT = ics.GetBaseType();
                 // const Type* baseTy = baseQT.getTypePtr();

                    // Anonymous-namespace UDT NTTP
                 // if (const auto* recTy = llvm::dyn_cast<clang::RecordType>(baseTy))
                    {
                 //     const auto* recDecl = recTy->getDecl();
                 //     if (recDecl->isInAnonymousNamespace())
                 //     {
                 //         out += ics.ConstructPrefix();
                 //         out += IndentBlock(ConstructRecordSignature(llvm::dyn_cast<CXXRecordDecl>(recDecl)), out.size() - (out.rfind('\n') + 1));
                 //         out = out.substr(0, out.size() - 2); // strip ";\n"
                 //         out += ics.ConstructPointersAndReferences();

                 //         if (!nttp->getName().empty())
                 //             out += " " + nttp->getName().str();

                 //         handled = true;
                 //     }
                 //     else
                        //if (const auto* spec = llvm::dyn_cast<ClassTemplateSpecializationDecl>(recDecl))
                        //{
                        // // out += ics.ConstructPrefix();
                        //    out += spec->getQualifiedNameAsString();
                        //    out += IndentBlock(TemplateArgsToString(spec, false), out.size() - (out.rfind('\n') + 1));
                        //    out = out.substr(0, out.size() - 2); // strip ";\n"
                        //    out += ">" /*+ ics.ConstructPointersAndReferences()*/;

                        //    if (!nttp->getName().empty())
                        //        out += " " + nttp->getName().str();

                        //    handled = true;
                        //}
                    }
                }

                if (handled == false) // still unhandled? Use fallback
                {
                    std::string declStr;
                    llvm::raw_string_ostream declStream(declStr);
                    nttp->getType().print(declStream, contextItems.printPolicy, nttp->getName());
                    out += declStr;
                }

                if (nttp->hasDefaultArgument())
                {
                    std::string defaultStr;
                    llvm::raw_string_ostream defaultStream(defaultStr);
                    nttp->getDefaultArgument().getArgument().getAsExpr()->printPretty(defaultStream, nullptr, contextItems.printPolicy);
                    out += "=" + defaultStr;
                }
                continue;
            }
            if (const auto* ttp2 = clang::dyn_cast<clang::TemplateTemplateParmDecl>(param))
            {
                out += "template<";
                const clang::TemplateParameterList* innerParams = ttp2->getTemplateParameters();
                bool first2 = true;
                for (const clang::NamedDecl* innerParam : *innerParams)
                {
                    if (!first2)
                        out += ", ";
                    first2 = false;

                    if (const auto* innerTtp = clang::dyn_cast<clang::TemplateTypeParmDecl>(innerParam))
                        out += innerTtp->wasDeclaredWithTypename() ? "typename" : "class";
                    else if (const auto* innerNttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(innerParam))
                    {
                        out += innerNttp->getType().getAsString(contextItems.printPolicy);
                        if (!innerNttp->getName().empty())
                            out += " " + innerNttp->getName().str();
                    }
                }
                out += "> class";
                if (!ttp2->getName().empty())
                    out += " " + ttp2->getName().str();

                if (ttp2->hasDefaultArgument())
                {
                    std::string defaultStr;
                    llvm::raw_string_ostream defaultStream(defaultStr);
                    ttp2->getDefaultArgument().getArgument().getAsTemplate().print(defaultStream, contextItems.printPolicy);
                    out += "=" + defaultStr;
                }
                continue;
            }
        }
        out += "> ";
        return out;
    }
}