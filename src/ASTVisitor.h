#pragma once

#include <vector>
#include <string>
#include <map>

#include "..\src\SerializationUtils.h"
#include "..\src\SerializeDecls.h"
#include "..\src\SerializeTypes.h"
#include "..\src\SerializeAttrs.h"

namespace OdrCop3
{
    enum InfoKind { Function, Typedef, Enum, Udt, Var };
    template<InfoKind K> struct InfoBase
    {
        const std::string TU;
        const std::string fullyQualified;
        bool operator==(const InfoBase& other) const { return fullyQualified == other.fullyQualified; }
    };
    using FunctionInfo = InfoBase<InfoKind::Function>;
    using  TypedefInfo = InfoBase<InfoKind::Typedef>;
    using     EnumInfo = InfoBase<InfoKind::Enum>;
    using      UdtInfo = InfoBase<InfoKind::Udt>;
    using      VarInfo = InfoBase<InfoKind::Var>;

    struct AllMaps
    {
        std::map<std::string,std::vector<     UdtInfo>>      udtMap;
        std::map<std::string,std::vector<     VarInfo>>      varMap;
        std::map<std::string,std::vector<    EnumInfo>>     enumMap;
        std::map<std::string,std::vector< TypedefInfo>>  typedefMap;
        std::map<std::string,std::vector<FunctionInfo>> functionMap;
    };

    class TheVisitor : public RecursiveASTVisitor<TheVisitor>
    {
        const std::string TU;
        ASTContext* context;
        const PrintingPolicy printPolicy;
        AllMaps& maps;
        ContextItems contextItems;
    public:
        TheVisitor(ASTContext* context, AllMaps& maps, const std::string& TU)
            : TU           (TU)
            , context      (context)
            , printPolicy  (context->getLangOpts())
            , maps         (maps)
            , contextItems (context, printPolicy)
        {}
    private:
        static std::string SerializeDecls(const ContextItems& contextItems, const clang::Decl* decl) { return Serialize::Decls<                 &SerializeTypes, &SerializeAttrs>(contextItems, decl); }
        static std::string SerializeTypes(const ContextItems& contextItems, const clang::Type* type) { return Serialize::Types<&SerializeDecls,                  &SerializeAttrs>(contextItems, type); }
        static std::string SerializeAttrs(const ContextItems& contextItems, const clang::Attr* attr) { return Serialize::Attrs<&SerializeDecls, &SerializeTypes                 >(contextItems, attr); }
    public:
        bool VisitFunctionDecl(FunctionDecl* funcDecl)
        {
            if (context->getSourceManager().isInSystemHeader(funcDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (funcDecl->isImplicit())
                return true;

            if (funcDecl->getStorageClass() == clang::SC_Static || funcDecl->isInAnonymousNamespace())
                return true; // if the function has internal-linkage, skip it

            if (const auto* method = dyn_cast<CXXMethodDecl>(funcDecl))
            if (const auto* record = dyn_cast<CXXRecordDecl>(method->getDeclContext()))
            if (record->isLambda())
                return true; // skip lambdas, too

            if (funcDecl->isThisDeclarationADefinition())
            {
                maps.functionMap[CreateKeyForFunctionMap(funcDecl)].push_back({TU, SerializeDecls(contextItems, funcDecl)});
            }
            return true;
        }

        bool shouldVisitTemplateInstantiations() const { return true; }
        bool shouldVisitImplicitCode          () const { return true; }
        bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl)
        {
            if (context->getSourceManager().isInSystemHeader(recordDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (recordDecl->isInAnonymousNamespace())
                return true; // if the class/struc/union has internal-linkage, skip it

            if (recordDecl->isLambda())
                return true; // skip lambdas

            if (recordDecl->isThisDeclarationADefinition())
            {
                std::string key = recordDecl->getQualifiedNameAsString();
                     if (recordDecl->getDescribedClassTemplate())                            key += "<>";
            //  else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl)) key += TemplateArgsToString(CTSD, true);

                maps.udtMap[key].push_back({TU,SerializeDecls(contextItems, recordDecl) });
            }
            return true;
        }
        bool VisitEnumDecl(clang::EnumDecl* enumDecl)
        {
            if (context->getSourceManager().isInSystemHeader(enumDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (enumDecl->isInAnonymousNamespace())
                return true; // TU-local, not an ODR candidate

            if (enumDecl->isThisDeclarationADefinition())
            {
                std::string  prettyName = enumDecl->getNameAsString() == "" ? MakeUnnamedEnumKey(enumDecl) : enumDecl->getQualifiedNameAsString();
                maps.enumMap[prettyName].push_back({TU, SerializeDecls(contextItems, enumDecl)});
            }
            return true;
        }
        bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefDecl)
        {
            if (context->getSourceManager().isInSystemHeader(typedefDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (typedefDecl->isImplicit())
                return true;

            if (const auto* tad = llvm::dyn_cast<TypeAliasDecl>(typedefDecl))
                if (tad->getDescribedAliasTemplate() != nullptr)
                    return true; // Skip the templated decl inside a TypeAliasTemplateDecl - handled below

            if (typedefDecl->getDeclContext()->isRecord()) {
                const auto* rd = llvm::cast<CXXRecordDecl>(typedefDecl->getDeclContext());
                if (rd->getDescribedClassTemplate() != nullptr)
                    return true; // inside a class template definition
                if (const auto* spec = llvm::dyn_cast<ClassTemplateSpecializationDecl>(rd))
                    return true; // inside a class template specialization or partial specialization
            }

            std::string aliasName = typedefDecl->getQualifiedNameAsString();
            maps.typedefMap[aliasName].push_back({TU, SerializeDecls(contextItems, typedefDecl)});
            return true;
        }
        bool VisitTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl* tatDecl)
        {
            //if (context->getSourceManager().isInSystemHeader(tatDecl->getLocation()))
            //    return true;

            //std::string     aliasName = tatDecl->getQualifiedNameAsString();
            //maps.typedefMap[aliasName].push_back({TU, ConstructTemplateAliasSignature(tatDecl)});
            return true;
        }

        bool VisitVarDecl(const VarDecl* varDecl)
        {
            //if (context->getSourceManager().isInSystemHeader(varDecl->getLocation()))
            //    return true; // skip anything not in the main file or a user header

            //if (varDecl->isLocalVarDecl())
            //    return true; // local variables can't be ODR violations
            //if (isa<CXXRecordDecl>(varDecl->getDeclContext()))
            //    return true; // struct/class statics:  already done during CXXRecordDecl parsing
            //auto linkage = varDecl->getLinkageAndVisibility().getLinkage();
            //if (linkage != clang::Linkage::External &&
            //    linkage != clang::Linkage::UniqueExternal)
            //    return true; // internal linkage (anon namespace, static, const at file scope, etc.)

            //std::string key = varDecl->getQualifiedNameAsString();
            //std::string out;

            //if (varDecl->isInline())
            //    out = "inline ";
            //if (varDecl->isConstexpr())
            //    out += "constexpr ";

            //std::string ptrSuffix;
            //QualType qt = varDecl->getType();
            //while (!qt->getPointeeType().isNull())
            //{
            //    ptrSuffix = std::string(qt.isConstQualified() ? " const" : "") + std::string(qt->isPointerType() ? " *" : " &") + ptrSuffix;
            //    qt = qt->getPointeeType();
            //}

            //std::string cvPrefix;
            //if (qt.isConstQualified())    cvPrefix += "const ";
            //if (qt.isVolatileQualified()) cvPrefix += "volatile ";
            //qt = qt.getUnqualifiedType();

            //qt = qt.getCanonicalType(); // normalize sugar (typedef, decltype, elaborated, etc.)

            //CXXRecordDecl* record = qt->getAsCXXRecordDecl();
            //if (record && record->isInAnonymousNamespace() && record->getIdentifier() != nullptr)
            //{
            //    out += cvPrefix;
            //    out += IndentBlock(ConstructRecordSignature(record), out.size());
            //    out  = out.substr(0, out.size()-2); // strip last ";\n"
            //    out += ptrSuffix + " " + key;
            //}
            //else if (record && record->isLambda())
            //    out += "auto " + key;
            //else if (record && llvm::isa<clang::ClassTemplateSpecializationDecl>(record))
            //{
            //    auto* spec = llvm::cast<clang::ClassTemplateSpecializationDecl>(record);
            //    out += spec->getQualifiedNameAsString();
            //    out += IndentBlock(TemplateArgsToString(spec), out.size());
            //    out = out.substr(0, out.size() - 1); // strip off last "\n"
            //    out += " " + key;
            //}
            //else if (const auto* enumTy = dyn_cast<clang::EnumType>(qt.getTypePtr()); enumTy && enumTy->getDecl()->isInAnonymousNamespace())
            //{
            //    out += cvPrefix + ConstructEnumDefinition(enumTy->getDecl());
            //    out += ptrSuffix + " " + key;
            //} 
            //else if (const auto* arr = dyn_cast<clang::ArrayType>(qt.getTypePtr()))
            //{
            //    QualType elem = arr->getElementType().getCanonicalType();

            //    if (auto* elemRec = elem->getAsCXXRecordDecl()) {
            //        out += cvPrefix;
            //        out += IndentBlock(ConstructRecordSignature(elemRec), out.size());
            //        out = out.substr(0, out.size()-2); // strip off last ";\n"
            //    }
            //    else if (auto* elemEnumTy = dyn_cast<clang::EnumType>(elem.getTypePtr()))
            //        out += cvPrefix + ConstructEnumDefinition(elemEnumTy->getDecl());
            //    else
            //        out += cvPrefix + elem.getUnqualifiedType().getAsString(printPolicy);

            //    if (const auto* cat = dyn_cast<clang::ConstantArrayType>(arr)) {
            //        uint64_t n = cat->getSize().getZExtValue();
            //        out += ptrSuffix + " " + key + "[" + std::to_string(n) + "]";
            //    } else
            //        out += ptrSuffix + " " + key + "[]";
            //}
            //else if (const auto* funcTy = llvm::dyn_cast<clang::FunctionProtoType>(qt.getTypePtr()))
            //    out += ConstructPointerToFunctionSignature(funcTy, ptrSuffix, key);
            //else
            //    out += varDecl->getType().getUnqualifiedType().getAsString(printPolicy) + " " + key;

            //if (varDecl->isInline() || varDecl->isConstexpr()) // inlines/constexpres get initiallizers; for everything else initializers are not ODR-relevant
            //if (varDecl->getInit())
            //{
            //    SourceLocation nameLoc = varDecl->getLocation();
            //    SourceLocation  endLoc = varDecl->getEndLoc();
            //    if (endLoc != nameLoc)
            //    {
            //        const Expr* init = varDecl->getInit()->IgnoreImplicit();
            //        if (const auto* ctor = dyn_cast<CXXConstructExpr>(init))
            //        {
            //            out += "{";
            //            for (unsigned i=0; i<ctor->getNumArgs(); ++i)
            //            {
            //                if (i > 0) out += ", ";
            //                out += Lexer::getSourceText(CharSourceRange::getTokenRange(ctor->getArg(i)->getSourceRange()), context->getSourceManager(), context->getLangOpts());
            //            }
            //            out += "}";
            //        }
            //        else if (const auto* initList = dyn_cast<InitListExpr>(init))
            //        {
            //            out += "{";
            //            for (unsigned i=0; i<initList->getNumInits(); ++i)
            //            {
            //                if (i > 0) out += ", ";
            //                out += Lexer::getSourceText(CharSourceRange::getTokenRange(initList->getInit(i)->getSourceRange()), context->getSourceManager(), context->getLangOpts());
            //            }
            //            out += "}";
            //        }
            //        else
            //        {
            //            std::string text = Lexer::getSourceText(CharSourceRange::getTokenRange(nameLoc, endLoc), context->getSourceManager(), context->getLangOpts()).str();
            //            StringRef suffix = StringRef(text).drop_front(varDecl->getName().size()).ltrim();
            //            if (suffix.starts_with("{"))
            //                out += suffix;
            //            else if (suffix.starts_with("="))
            //                out += " " + suffix.str();
            //            else
            //                out += " = " + suffix.str();
            //        }
            //    }
            //}
            //out += ";";

            //maps.varMap[key].push_back({TU, out});
            return true;
        }

    private:
#ifdef NOT_NOW
        std::string ConstructPointerToFunctionSignature(const clang::FunctionProtoType* funcTy, const std::string& ptrSuffix, const std::string& name)
        {
            std::string out;

            // return type
            QualType ret     = funcTy->getReturnType().getCanonicalType();
            if (auto* retRec = ret->getAsCXXRecordDecl(); retRec && retRec->isInAnonymousNamespace() && retRec->getIdentifier() != nullptr)
            {
                out += IndentBlock(ConstructRecordSignature(retRec), out.size());
                out = out.substr(0, out.size() - 2); // strip ";\n"
            }
            else if (auto* retEnum = llvm::dyn_cast<clang::EnumType>(ret.getTypePtr()); retEnum && retEnum->getDecl()->isInAnonymousNamespace())
                out += ConstructEnumDefinition(retEnum->getDecl());
            else
                out += ret.getUnqualifiedType().getAsString(printPolicy);

            // function pointer name
            out += " (";
            out += ptrSuffix.substr(1); // " *", " &", " const *", etc. // substr(1) to remove the leading ' '
            out += name;
            out += ")";

            // Parameter list
            out += "(";
            for (unsigned i=0; i<funcTy->getNumParams(); ++i)
            {
                if (i > 0)
                    out += ", ";

                QualType canonicalQT = funcTy->getParamType(i).getCanonicalType();
                IndirectionCvStripper ics(canonicalQT);

                const auto* recordType = llvm::dyn_cast<clang::RecordType>(ics.GetBaseType().getTypePtr());
                if (recordType && llvm::dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl()))
                { // class template instantiations
                    const auto* spec = llvm::dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl());
                    out += ics.ConstructPrefix();
                    out += spec->getQualifiedNameAsString();
                    out += IndentBlock(TemplateArgsToString(spec, false), out.size() - (out.rfind('\n') + 1));
                    out  = out.substr(0, out.size() - 2); // strip ";\n"
                    out += ">" + ics.ConstructPointersAndReferences();
                    continue;
                }
                if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                { // UDTs
                    out += ics.ConstructPrefix();
                    out += IndentBlock(ConstructRecordSignature(llvm::dyn_cast<CXXRecordDecl>(recordType->getDecl())), out.size() - (out.rfind('\n')+1));
                    out  = out.substr(0, out.size()-2); // strip ";\n"
                    out += ics.ConstructPointersAndReferences();
                    continue;
                }
                if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(ics.GetBaseType().getTypePtr()); enumTy && enumTy->getDecl()->isInAnonymousNamespace())
                { // enums
                    out += ics.ConstructPrefix();
                    out += ConstructEnumDefinition(enumTy->getDecl());
                    out += ics.ConstructPointersAndReferences();
                    continue;
                }
                if (auto* ptrTy = llvm::dyn_cast<clang::PointerType>(canonicalQT.getTypePtr()))
                {
                    if (auto* innerFuncTy = llvm::dyn_cast<clang::FunctionProtoType>(ptrTy->getPointeeType().getTypePtr()))
                    {
                        std::string innerPtrSuffix = IndirectionCvStripper(canonicalQT).ConstructPointersAndReferences();
                        out += IndentBlock(ConstructPointerToFunctionSignature(innerFuncTy, innerPtrSuffix, ""), out.size() - (out.rfind('\n')+1));
                        out  = out.substr(0, out.size()-1); // strip "\n"
                        continue;
                    }
                }

                {   // Plain type: prefix + base + ptr/ref/array layers
                    out += ics.ConstructPrefix();
                    out += ics.GetBaseType().getUnqualifiedType().getAsString(printPolicy);
                    out += ics.ConstructPointersAndReferences();
                }
            }

            if (funcTy->isVariadic())
            {
                if (funcTy->getNumParams() > 0)
                    out += ", ";
                out += "...";
            }
            out += ")";

            return out;
        }

        std::string ConstructTypedefSignature(const clang::TypedefNameDecl* typedefDecl)
        {
            QualType    underlying   = typedefDecl->getUnderlyingType().getCanonicalType();
            std::string aliasName    = typedefDecl->getQualifiedNameAsString();
            std::string resolvedType = underlying.getAsString(printPolicy);

            // nameless or anonymous UDTs
            bool needsFullInlining = false;
            const RecordType* recordType = underlying->getAs<RecordType>();
            if (recordType != nullptr) {
                if (recordType->getDecl()->getName().empty())
                    needsFullInlining = true;
                else {
                    const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(recordType->getDecl()->getDeclContext());
                    if (nsDecl != nullptr)
                        if (nsDecl->isAnonymousNamespace())
                            needsFullInlining = true;
                }
                if (needsFullInlining)
                {
                    std::string fqtd = "using " + aliasName + " = ";
                    fqtd += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), fqtd.size());
                    fqtd  = fqtd.substr(0, fqtd.size()-2); // strip last ";\n"
                    fqtd += "; // typedef " + resolvedType + " " + aliasName + ";";
                    return fqtd;
                }


                // anonymous-namespace function (or other ValueDecl) used as a non-type template argument
                const ClassTemplateSpecializationDecl* specDecl = dyn_cast<ClassTemplateSpecializationDecl>(recordType->getDecl());
                if (specDecl != nullptr)
                {
                    std::string fqtd2 = "using " + aliasName + " = " + specDecl->getSpecializedTemplate()->getNameAsString() + "<";

                    // lastColumn and column are absolute
                    int lastColumn = 0;

                    std::string argList;

                    bool anyInlined = false;
                    const TemplateArgumentList& args = specDecl->getTemplateArgs();
                    for (unsigned i=0; i<args.size(); ++i)
                    {
                        const TemplateArgument& arg = args.get(i);
                        bool isAnonNsFunc = false;
                        if (i != 0)
                            argList += ", ";

                        const FunctionDecl* funcDecl = nullptr;
                        if (arg.getKind() == TemplateArgument::Declaration)
                            funcDecl = dyn_cast<FunctionDecl>(arg.getAsDecl());
                        if (funcDecl != nullptr) {
                            const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(funcDecl->getDeclContext());
                            if (nsDecl != nullptr && nsDecl->isAnonymousNamespace())
                                isAnonNsFunc = true;
                        }

                        if (isAnonNsFunc) {
                            argList += "&(";

                            int column = static_cast<int>(argList.size() - (argList.rfind('\n') + 1));
                            if (lastColumn == 0)
                                column += static_cast<int>(fqtd2.size()); // only first time

                            argList += IndentBlock(ConstructFunctionSignature(funcDecl), column); // -lastColumn);
                            argList  = argList.substr(0, argList.size()-1); // remove "\n"
                            argList += ")";

                            anyInlined = true;
                            lastColumn = column;
                        } else {
                            llvm::raw_string_ostream stream(argList);
                            arg.print(printPolicy, stream, /*IncludeType=*/false);
                        }
                    }

                    if (anyInlined)
                    {
                        size_t col1 = fqtd2.size() - (fqtd2.rfind('\n')+1);// get length of last line of fqtd2

                        std::string whatWillBeReused = argList + ">";
                        fqtd2 += whatWillBeReused + "; // typedef " + specDecl->getSpecializedTemplate()->getNameAsString() + "<";

                        size_t col2 = fqtd2.size() - (fqtd2.rfind('\n')+1); // get length of last line of fqtd2
                        fqtd2 += IndentBlock(whatWillBeReused, col2-col1);
                        fqtd2  = fqtd2.substr(0, fqtd2.size()-1); // remove "\n"
                        fqtd2 += " " + aliasName + ";";
                        return fqtd2;
                    }
                }
            }

            // nameless or anonymous enums
            bool isNameless           = false;
            bool isAnonymousNamespace = false;
            const EnumType * enumType = underlying->getAs<EnumType>();
            if (enumType != nullptr)
            {
                if (enumType->getDecl()->getName().empty())
                    isNameless = true;
                const NamespaceDecl* nsDecl = dyn_cast<NamespaceDecl>(enumType->getDecl()->getDeclContext());
                if (nsDecl != nullptr)
                    if (nsDecl->isAnonymousNamespace())
                        isAnonymousNamespace = true;
            }
            if (isNameless || isAnonymousNamespace)
            {   // if nameless, enumSig will look like this:  "enum (unnamed enum : Red) { Red = 0, Green = 1, Blue = 2 };"
                std::string enumSig = ConstructEnumDefinition(enumType->getDecl());
                if (isAnonymousNamespace && isNameless)
                {   // if both, need to insert "(anonymous namespace)::" between the "enum " and "(unnamed enum"
                    enumSig = enumSig.substr(5); // strip off "enum "
                    enumSig = "enum (anonymous namespace)::" + enumSig;
                }

                return "using " + aliasName + " = " + enumSig + "; // typedef " + enumSig + " " + aliasName + ";";
            }

            return "using " + aliasName + " = " + resolvedType + "; // typedef " + resolvedType + " " + aliasName + ";";
        }
        std::string ConstructTemplateAliasSignature(const clang::TypeAliasTemplateDecl* tatDecl)
        {
            std::string params;
            for (const NamedDecl* param : *tatDecl->getTemplateParameters())
            {
                if (!params.empty())
                    params += ", ";
                params += param->getNameAsString();
            }

            TypeAliasDecl* aliasDecl = tatDecl->getTemplatedDecl();
            std::string    aliasName = tatDecl->getQualifiedNameAsString();

            std::string fqtd = "template <" + params + "> using " + aliasName + " = ";
            
            QualType             underlying = aliasDecl->getUnderlyingType();
            const RecordType   * recordType = underlying.getCanonicalType()->getAs<RecordType>();
            const NamespaceDecl* nsDeclCtx  = recordType != nullptr ? dyn_cast<NamespaceDecl>(recordType->getDecl()->getDeclContext()) : nullptr;
            if (recordType != nullptr && (recordType->getDecl()->isAnonymousStructOrUnion() || (nsDeclCtx != nullptr && nsDeclCtx->isAnonymousNamespace())))
            {
                fqtd += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), fqtd.size());
                fqtd  = fqtd.substr(0, fqtd.size()-2); // strip last ";\n"
                fqtd += "; // no typedef equivalent";
            } else
                fqtd += underlying.getAsString(printPolicy) + "; // no typedef equivalent";

            return fqtd;
        }
        std::string TemplateArgsToString(const clang::ClassTemplateSpecializationDecl* ctsd, bool wantAnonymousNamespaceWithTU = false)
        {
            std::string out;
            out += "<";

            const clang::TemplateArgumentList &   args = ctsd->getTemplateArgs();
            const clang::TemplateParameterList* params = ctsd->getSpecializedTemplate()->getTemplateParameters();

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
                                out += ConstructEnumDefinition(enumTy->getDecl()) + "::" + ecd->getNameAsString();
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
                    IndirectionCvStripper ics(vd->getType().getCanonicalType());
                    QualType baseQT       = ics.GetBaseType();
                    const Type* baseTy    = baseQT.getTypePtr();
                    if (const auto* recTy = dyn_cast<RecordType>(baseTy))
                    {
                        const auto* recDecl = recTy->getDecl();
                        if (recDecl->isInAnonymousNamespace())
                        {
                            out += ics.ConstructPrefix();
                            out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recDecl)), out.size() - (out.rfind('\n')+1));
                            out  = out.substr(0, out.size()-2); // strip ";\n"
                            out += ics.ConstructPointersAndReferences();

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
                        std::string line = IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(rd)), out.size());
                        if (wantAnonymousNamespaceWithTU)
                        {
                            const std::string from = "anonymous namespace";
                            if (auto pos = line.find(from); pos != std::string::npos)
                                line.replace(pos, from.size(), "anonymous namespace in " + TU);
                        }
                        out += line;
                        out  = out.substr(0, out.size()-2); // strip last ";\n"
                    } else
                        out += arg.getAsType().getAsString();
                }
                    break;
                case clang::TemplateArgument::Expression:
                {
                    std::string s;
                    llvm::raw_string_ostream os(s);
                    arg.getAsExpr()->printPretty(os, nullptr, context->getPrintingPolicy());
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
                            pe.getAsExpr()->printPretty(os, nullptr, context->getPrintingPolicy());
                            out += os.str();
                            break;
                        }
                        default:
                        {
                            std::string tmp;
                            llvm::raw_string_ostream os(tmp);
                            pe.print(printPolicy, os, true);
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
                    arg.print(printPolicy, os, true);
                    out += os.str();
                    break;
                }
                }
            }
            out += ">";
            return out;
        }

        class IndirectionCvStripper
        {
            enum class IndirKind { Ptr, LRef, RRef, Array };
            struct IndirLayer
            {
                IndirKind kind;
                bool      isConst;    // const on this declarator layer (e.g. T* const)
                bool      isVolatile; // volatile on this declarator layer (rarely seen but correct)
                uint64_t  arraySize;  // 0 means “[]”, nonzero means “[N]”
            };
            const std::vector<IndirLayer> layers; // outermost last, so we append in reverse
            const bool baseConst;
            const bool baseVolatile;
            const QualType baseType;

            static std::tuple<std::vector<IndirLayer>, QualType, bool, bool> Peel(QualType qualType)
            {
                std::vector<IndirLayer> layers;
                while (true)
                {
                    if (const auto* pt = qualType->getAs<PointerType>())
                    {
                        layers.push_back({IndirKind::Ptr, qualType.isConstQualified(), qualType.isVolatileQualified()});
                        qualType = pt->getPointeeType().getCanonicalType();
                    }
                    else if (const auto* rt = qualType->getAs<LValueReferenceType>())
                    {
                        layers.push_back({IndirKind::LRef, false, false});
                        qualType = rt->getPointeeType().getCanonicalType();
                    }
                    else if (const auto* rt = qualType->getAs<RValueReferenceType>())
                    {
                        layers.push_back({IndirKind::RRef, false, false});
                        qualType = rt->getPointeeType().getCanonicalType();
                    }
                    else if (const auto* at = dyn_cast<ArrayType>(qualType.getTypePtr()))
                    {
                        uint64_t n = 0;
                        if (const auto* cat = dyn_cast<ConstantArrayType>(at))
                            n = cat->getSize().getZExtValue();

                        layers.push_back({IndirKind::Array, false, false, n});
                        qualType = at->getElementType().getCanonicalType();
                    } else
                        break;
                }
                return {std::move(layers), qualType, qualType.isConstQualified(), qualType.isVolatileQualified()};
            }
            explicit IndirectionCvStripper(std::tuple<std::vector<IndirLayer>, QualType, bool, bool> t)
                : layers      (std::move(std::get<0>(t)))
                , baseType    (std::get<1>(t))
                , baseConst   (std::get<2>(t))
                , baseVolatile(std::get<3>(t))
            {}

            bool HasArray    () const { for (auto& layer : layers) if (layer.kind == IndirKind::Array                                ) return true; return false; }
            bool HasReference() const { for (auto& layer : layers) if (layer.kind == IndirKind::LRef || layer.kind == IndirKind::RRef) return true; return false; }
        public:
            explicit IndirectionCvStripper(QualType qualType) : IndirectionCvStripper(Peel(qualType)) {}

            QualType GetBaseType() const { return baseType; }

            std::string ConstructPrefix() const
            {
                std::string prefix;
                if (baseConst)    prefix += "const ";
                if (baseVolatile) prefix += "volatile ";
                return prefix;
            }
            std::string ConstructPointersAndReferences() const
            {
                std::string out;

                if (HasArray() && HasReference())
                    out += " ("; // someType (&someName)[N] case

                // Re-apply layers in reverse (innermost first)
                for (auto it = layers.rbegin(); it != layers.rend(); ++it)
                {
                    switch (it->kind)
                    {
                    case IndirKind::Ptr:  out += "*";  break;
                    case IndirKind::LRef: out += "&";  break;
                    case IndirKind::RRef: out += "&&"; break;
                    }
                    if (it->isConst)      out += "const ";
                    if (it->isVolatile)   out += "volatile ";
                }

                // if the layers were not all arrays
                if (!out.starts_with(" ("))
                    if (out.size() > 0)
                        out = " " + out; // put a space between type and * or &
                return out;
            }
            std::string ConstructSuffixWithName(const std::string& name) const
            {
                std::string out;

                if (HasArray() && HasReference()) {
                    out += name;
                    out += ")"; // someType (&someName)[N] case
                } else
                    if (name != "")
                         out += " " + name;

                for (auto it=layers.rbegin(); it!=layers.rend(); ++it) // layers are outermost last, so append in reverse
                {
                    if (it->kind == IndirKind::Array)
                    {
                        if (it->arraySize == 0)
                            out += "[]";
                        else
                            out += "[" + std::to_string(it->arraySize) + "]";
                    }
                }
                return out;
            }
        };
#endif
        std::string CreateKeyForFunctionMap(const clang::FunctionDecl* funcDecl)
        {
            // C APIs (like DllMain and main) trigger this path.
            std::unique_ptr<clang::MangleContext> mangleContext(context->createMangleContext());
            if (!mangleContext->shouldMangleDeclName(funcDecl))
            {
                std::string display = funcDecl->getType().getAsString(printPolicy);
                return display.replace(display.find('('), 0, funcDecl->getNameAsString());
            }

            // cannot use MSVC mangling for anything else:  MSVC incorporates return type, which disallows some valid ODR violations detection. So making my own key, from:
        /*
        | Feature / Property                                    | In Key?      | In Value?   | Notes                                                              |
        |-------------------------------------------------------|--------------|-------------|--------------------------------------------------------------------|
     x  |                                  **Entity Identity**  |              |             |                                                                    |
     x  | Fully-qualified namespace scope                       | Yes          | No          | Include inline namespace canonicalization policy. Exclude internal-linkage entities from cross-TU map. |
     x  | Enclosing class scope                                 | Yes          | No          | Full canonical enclosing class chain for members.                  |
     x  | Function name                                         | Yes          | No          | Includes ordinary function identifier.                             |
        | Operator kind                                         | Yes          | No          | `operator+`, `operator()`, `operator new`, etc.                    |
        | Conversion operator target type                       | Yes          | No          | `operator int()` and `operator long()` are different functions.    |
        | Constructor / destructor identity                     | Yes          | No          | Constructor kind and destructor target class belong in identity.   |
        |                                       **Parameters**  |              |             |                                                                    |
     x  | Parameter count                                       | Yes          | No          | Includes fixed arity.                                              |
     x  | Canonical parameter types                             | Yes          | Yes         | Key uses overload identity; Value may preserve written/dependent form for ODR diagnostics. |
     x  | Parameter packs                                       | Yes          | Yes         | Key should encode pack pattern, e.g. `T#0...`; Value should encode dependent pack refs structurally. |
     x  | C-style variadic `...`                                | Yes          | No          | `f(int)` vs `f(int, ...)` are distinct. |
     x  | Parameter top-level cv on by-value params             | Usually no   | Yes         | `void f(int)` and `void f(const int)` are same function type for parameter adjustment; preserve spelling/value if desired. |
     x  | Parameter names                                       | No           | No          | Not ODR-significant for function identity or definition equivalence.      |
     x  | Default function arguments                            | No           | Yes         | Compare where present; presence/absence across TUs needs careful policy because defaults attach to declarations. |
        | Explicit object parameter, C++23                      | Yes          | Yes         | Its type participates in overload identity; written form useful in Value. |
     x  |                       **Member Function Qualifiers**  |              |             |                                                                           |
     x  | Member `const` / `volatile`                           | Yes          | No          | `f()` and `f() const` are distinct overloads.                             |
     x  | Member ref-qualifier `&` / `&&`                       | Yes          | No          | Distinct overloads.                                                       |
     x  | Static vs non-static member                           | No           | Yes         | static must not be in the key so that the two functions will be compared  |
        |                          **Exception Specification**  |              |             |                                                                           |
     x  | `noexcept` / dynamic exception spec                   | No           | Yes         | Not an overload discriminator. Mismatch should be reported.               |
     x  | `noexcept(expr)` expression                           | No           | Yes         | Preserve structural/token-equivalent expression in Value.                 |
     x  |                                    **Calling / ABI**  |              |             |                                                                           |
     x? | Calling convention                                    | Maybe        | Yes         | Put in Key only if your frontend/compiler allows it to distinguish overloads in your target mode. Always keep in Value. |
     x  | Language linkage, `extern "C"`                        | No           | Yes         | Do not use to avoid comparison; mismatch is meaningful. C linkage also has special no-overload rules. |
     x  | DLL import/export attributes                          | No           | Maybe       | Usually not ODR identity; useful as Value if you care about ABI/build mismatches. |
     x  |                                      **Return Type**  |              |             |                                                                                  |
     x  | Ordinary return type                                  | No           | Yes         | Same name/params differing only by return type should land together and report. |
        | Conversion function return type                       | Yes          | No          | It is the conversion target and effectively part of the function name.         |
     x  | Deduced return type spelling/body dependency          | No           | Yes         | `auto`/`decltype(auto)` and trailing return expression belong in Value.       |
     x  |                                        **Templates**  |              |             |                                                                              |
     x  | Function template vs non-template                     | Yes          | No          | Different identity category.                                                |
     x  | Template parameter list shape                         | Yes          | Yes         | Key: kind/arity/pack/constraints shape. Value: full written/dependent form. |
     x  | Template parameter names                              | No           | No          | Alpha-renamable.                                                                  |
     x  | Template default arguments                            | No           | Yes         | Like function defaults: compare when present, but declarations may accumulate defaults. |
        | Template constraints / requires-clause, identity form | Yes          | Yes         | Key needs enough canonical constraint identity to distinguish constrained overloads. Value keeps full expression for ODR equivalence. |
     x  | Explicit specialization marker                        | Yes          | No          | Primary template and explicit specialization are different comparison entities. |
        | Explicit specialization arguments                     | Yes          | Yes         | Key identifies specialization; Value can preserve written form. |
     x  | Instantiation-only artifacts                          | No           | No          | Don’t key compiler-generated instantiation bookkeeping as source entities unless deliberately checking instantiated bodies. |
     x  |                                **Storage / Linkage**  |              |             |                                                                                |
     x  | Internal-linkage free function, `static`              | No cross-TU  | Maybe local | Exclude from global cross-TU ODR map; can still check within one TU if useful. |
     x  | Anonymous namespace entity                            | No cross-TU  | Maybe local | Same: TU-local identity. |
     x  | `inline`                                              | No           | Yes         | Not overloadable; definitions across TUs must agree. |
     x  | `extern` declaration marker                           | No           | Usually no  | Storage declaration detail, unless your diagnostic wants it. |
     x  |                              **Function Specifiers**  |              |             |                                     |
     x  | `constexpr`                                           | No           | Yes         | Not overloadable; mismatch matters. |
     x  | `consteval`                                           | No           | Yes         | Not overloadable; mismatch matters. |
     x  | `constinit`                                           | N/A          | N/A         | Variables only. |
     x  | `virtual`                                             | No           | Yes         | Not overloadable; class/function definition mismatch. |
     x  | `override`                                            | No           | Usually no  | Compile-time check/specifier; can be omitted unless you want strict spelling comparison. |
     x  | `final` on function                                   | No           | Yes         | Not overloadable, but semantically affects overriding. Keep in Value. |
     x  | Pure virtual `= 0`                                    | No           | Yes         | Not overloadable; mismatch matters. |
     x  | `= default` / `= delete`                              | No           | Yes         | Definition form must agree. |
     x  | `explicit` on constructors                            | No           | Yes         | Not an overload discriminator. |
     x  | `explicit` on conversion operators                    | No           | Yes         | Target type is Key; `explicit` is not a separate overload. |
     x  | `explicit(expr)`                                      | No           | Yes         | Preserve expression in Value.    |
     x  |                                       **Attributes**  |              |             |                                  |
        | Calling-convention attributes                         | Maybe        | Yes         | Same rule as calling convention. |
        | ABI-affecting attributes                              | Usually no   | Yes         | Do not drop blindly: `abi_tag`, layout/codegen/vendor attributes can matter. |
     x  | Semantic/codegen attributes                           | No           | Yes         | Safer to keep in Value: e.g. `[[noreturn]]`, target attributes, allocator-like vendor attrs. |
     x  | Diagnostic-only attributes                            | No           | Usually no  | `[[nodiscard]]`, `[[deprecated]]`, maybe omit unless doing strict token ODR. |
     x  | Unknown attributes                                    | No           | Yes         | Conservative policy: keep in Value so you don’t silently miss meaningful differences. |
     x  |                                **Body / Definition**  |              |             |                                     |
     x  | Function body                                         | No           | Yes         | Core ODR comparison payload.        |
     x  | Local type definitions                                | No           | Yes         | Serialize structurally inside body. |
     x  | Local statics                                         | No           | Yes         | Serialize as part of body/local declaration structure. |
     x  | Dependent expressions                                 | No           | Yes         | Preserve AST structure, not raw spelling where possible. |
     x  | Lambda expressions                                    | No           | Yes         | Not independent cross-TU function keys; serialize structurally inside containing Value.          |
     x  | Lambda `operator()` as separate entity                | No           | No          | Unless you are doing a nested structural serialization; don’t put it in the global function map. |
     x  |                                          **Friends**  |              |             |                                                                                                  |
        | Friend declaration only                               | No           | No          | No definition to compare, except as part of class declaration if doing strict class ODR.         |
        | Friend function definition with external linkage      | Yes          | Yes         | Can be keyed like a free function, or compared as part of class Value; avoid double-reporting.   |
        | Inline friend inside class                            | Yes / policy | Yes         | It does define a namespace-scope function; either index it or let class structural comparison own it. |
        */

            std::string key;

            if (const auto* conv = llvm::dyn_cast<clang::CXXConversionDecl>(funcDecl))
            {
                std::string typeName, className;
                {
                    llvm::raw_string_ostream typeOs(typeName);
                    conv->getConversionType().print(typeOs, printPolicy);

                    llvm::raw_string_ostream classOs(className);
                    conv->getParent()->printQualifiedName(classOs);
                }
                key += className + "::operator " + typeName;
            }
            else
            {
                std::string out;
                llvm::raw_string_ostream os(out);
                funcDecl->printQualifiedName(os);
                key += os.str();
            }

            // template stuff, if any
            if (const FunctionTemplateDecl* templateDecl = funcDecl->getDescribedFunctionTemplate())
            {   // primary template — serialize parameter kinds
                key += "<";
                const TemplateParameterList* params = templateDecl->getTemplateParameters();
                for (unsigned i=0; i<params->size(); ++i)
                {
                    if (i > 0)
                        key += ",";
                    const NamedDecl* param = params->getParam(i);
                    if (const TemplateTypeParmDecl* typeParam = dyn_cast<TemplateTypeParmDecl>(param))
                    {
                        const bool isConversionTypeParam = llvm::isa<clang::CXXConversionDecl>(funcDecl);
                        const std::string name = typeParam->getNameAsString();
                        key += (isConversionTypeParam && !name.empty() ? name : (typeParam->wasDeclaredWithTypename() ? "typename" : "class")) + (typeParam->isParameterPack() ? "..." : "");
                    }
                    else if (const NonTypeTemplateParmDecl* nonTypeParam = dyn_cast<NonTypeTemplateParmDecl>(param))
                        key += nonTypeParam->getType().getCanonicalType().getUnqualifiedType().getAsString(printPolicy) + (nonTypeParam->isParameterPack() ? "..." : "");
                    else if (const TemplateTemplateParmDecl* templateParam = dyn_cast<TemplateTemplateParmDecl>(param))
                        key += std::string("template<...> class") + (templateParam->isParameterPack() ? "..." : "");
                }
                key += ">";
            }
            else if (const FunctionDecl* primaryTemplate = funcDecl->getPrimaryTemplate() ? funcDecl->getPrimaryTemplate()->getTemplatedDecl() : nullptr)
            {   // explicit instantiation — serialize the actual template arguments
                const TemplateArgumentList* args = funcDecl->getTemplateSpecializationArgs();
                if (args)
                {
                    key += "<";
                    for (unsigned i=0; i<args->size(); ++i)
                    {
                        if (i > 0)
                            key += ",";
                        const TemplateArgument& arg = args->get(i);
                        llvm::raw_string_ostream os(key);
                        arg.print(printPolicy, os, /*IncludeType=*/true);
                    }
                    key += ">";
                }
            }
            else if (funcDecl->getTemplateSpecializationKind() == TSK_ExplicitSpecialization)
            {
                if (const TemplateArgumentList* args = funcDecl->getTemplateSpecializationArgs())
                {
                    key += "<";
                    for (unsigned i=0; i<args->size(); ++i)
                    {
                        if (i > 0)
                            key += ",";
                        llvm::raw_string_ostream os(key);
                        args->get(i).print(printPolicy, os, /*IncludeType=*/true);
                    }
                    key += ">";
                }
            }

            key += "(";

            // all parameters, without typedefs; no parameter names. Include "...".
            for (const ParmVarDecl* param : funcDecl->parameters())
            {
                if (param != *funcDecl->param_begin())
                    key += ",";

                std::string typeStr = param->getType().getCanonicalType().getUnqualifiedType().getAsString(printPolicy);

                const std::string from = "anonymous namespace";
                if (auto pos = typeStr.find(from); pos != std::string::npos)
                    typeStr.replace(pos, from.size(), "anonymous namespace in " + TU);
                
                key += typeStr;
            }
            if (funcDecl->isVariadic())
                key += funcDecl->param_empty() ? "..." : ",...";
            key += ")";

            // for methods, cv/&/&& is next
            if (const CXXMethodDecl* methodDecl = dyn_cast<CXXMethodDecl>(funcDecl))
            {
                if (methodDecl->isConst())                      key += " const";
                if (methodDecl->isVolatile())                   key += " volatile";
                if (methodDecl->getRefQualifier() == RQ_LValue) key += " &";
                if (methodDecl->getRefQualifier() == RQ_RValue) key += " &&";
            }
            return key;
        }

#ifdef NOT_NOW
        std::string ConstructRecordSignature(const CXXRecordDecl* recordDecl)
        {
            std::string out;

            // template
            const clang::ClassTemplateDecl* ctd = recordDecl->getDescribedClassTemplate();
            if (ctd)
                out += ConstructTemplateParameterList(ctd->getTemplateParameters());
            else if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl))
                out += "template<> ";
            
            // struct/class/union keyword
            out += recordDecl->getKindName().str() + " ";

            // alignas/[[attributes]]/__declspecs
            out += ConstructAttributes(recordDecl);

            // name (if not nameless)
            if (!recordDecl->isAnonymousStructOrUnion())
                out += recordDecl->getQualifiedNameAsString(); // note: no space until after any <> stuff

            // if it's a template instantiation, add <arg> 
            if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(recordDecl))
            {
                out += IndentBlock(TemplateArgsToString(CTSD), out.size());
                out  = out.substr(0, out.size()-1); // strip off last "\n"
            }

            out += " ";

            // base classes
            std::string indentToColon(out.size(), ' ');
            bool previousWasAnonymous = false;
            bool firstBase = true;
            for (const clang::CXXBaseSpecifier& base : recordDecl->bases())
            {
                if (firstBase) {
                    firstBase = false;
                    out += ": ";
                } else {
                    if (previousWasAnonymous) {
                        previousWasAnonymous = false;
                        out += "\n" + indentToColon;
                    }
                    out += ", ";
                }

                switch (base.getAccessSpecifier()) {
                case clang::AS_public:    out += "public ";    break;
                case clang::AS_protected: out += "protected "; break;
                case clang::AS_private:   out += "private ";   break;
                case clang::AS_none:      out += " ";          break; // depends on context
                }
                if (base.isVirtual())
                    out += "virtual ";

                // when a base is defined in an anonymous namespace, include the full definition here.
                const clang::Type* type = base.getType().getCanonicalType().getTypePtr();
                const auto* recordType  = dyn_cast<RecordType>(type);
                if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                {
                    previousWasAnonymous = true;
                    out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), out.size() - (out.rfind('\n') + 1));  // length of last line up to current spot
                    out  = out.substr(0, out.size()-2); // strip off last ";\n"
                }
                else
                    out += base.getType().getAsString(printPolicy);
            }
            if (firstBase == false)
                out += " ";

            // final
            if (true == recordDecl->hasAttr<FinalAttr>())
                out += "final ";

            // sizeof as comment
            if (recordDecl->isCompleteDefinition() && !recordDecl->isDependentType())
                out += "{ // sizeof=" + std::to_string(context->getASTRecordLayout(recordDecl).getSize().getQuantity()) + "\n";
            else
                out += "{\n";

            // data-members and methods
            for (const clang::Decl* decl : recordDecl->decls())
            {
                if (clang::isa<clang::AccessSpecDecl>(decl))
                {
                    const auto* access = clang::dyn_cast<clang::AccessSpecDecl>(decl);
                    switch (access->getAccess())
                    {
                    case AS_public:    out += "public:\n";    break;
                    case AS_protected: out += "protected\n:"; break;
                    case AS_private:   out += "private:\n";   break;
                    default:                                  break;
                    }
                    continue;
                }
                if (const auto* field = clang::dyn_cast<clang::FieldDecl>(decl))
                {   // data members

                    if (field->isAnonymousStructOrUnion())
                        continue; // nameless unions/struct never have a variable name

                    out += "   ";

                    // attributes on data-members
                    out += ConstructAttributes(decl);

                    { // when a field is defined in an anonymous namespace, include the full definition here with the field.
                        IndirectionCvStripper ics(field->getType().getCanonicalType());
                        const QualType qualType = ics.GetBaseType();
                        const clang::Type* type = qualType.getTypePtr();

                        std::string definition;

                        const auto* recordType  = clang::dyn_cast<clang::RecordType>(type);
                        if (recordType && recordType->getDecl()->isInAnonymousNamespace())
                        {
                            definition = IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 3);
                            definition = definition.substr(0, definition.size()-2);
                        }
                        else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && !enumTy->getDecl()->getIdentifier())
                            definition = ConstructEnumDefinition(enumTy->getDecl()); // nameless enum
                        else if (const auto* enumTy = llvm::dyn_cast<clang::EnumType>(type); enumTy && enumTy->getDecl()->isInAnonymousNamespace())
                            definition = ConstructEnumDefinition(enumTy->getDecl()); // enum defined in anonymous namespace

                        if (!definition.empty()) {
                            out += ics.ConstructPrefix() + ConstructAttributes(field);
                            out += definition;
                            out += ics.ConstructPointersAndReferences() + ics.ConstructSuffixWithName(field->getNameAsString());
                        } else {
                            // field must be done this way to handle array fields as well.
                            std::string fieldStr;
                            llvm::raw_string_ostream os(fieldStr);
                            field->getType().print(os, printPolicy, field->getNameAsString());
                            os.flush();
                            out += fieldStr;
                        }
                    }

                    if (field->isBitField())
                    {
                        std::string bitWidth;
                        llvm::raw_string_ostream os(bitWidth);
                        field->getBitWidth()->printPretty(os, nullptr, printPolicy);
                        os.flush();
                        out += " : " + bitWidth;
                    }

                    if (field->hasInClassInitializer())
                        out += AddClassInitializer(field);

                    out += ";\n";
                    continue;
                }
                if (const auto* var = clang::dyn_cast<clang::VarDecl>(decl))
                {   // static data members  (VarDecl inside a record == static member)
                    out += "   ";

                    // attributes on static data-members
                    out += ConstructAttributes(decl);

                    if (var->isConstexpr())
                        out += "constexpr ";
                    else if (var->isInline())
                        out += "inline ";

                    { // static field: use same type+name print trick as non-static
                        std::string fieldStr;
                        llvm::raw_string_ostream os(fieldStr);
                        var->getType().print(os, printPolicy, var->getNameAsString());
                        os.flush();
                        out += "static " + fieldStr;
                    }

                    if (var->hasInit())
                    {
                        const Expr* expr = var->getInit();
                        llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
                        std::string      init = text.str();
                        if ((init.starts_with("{")) || init.starts_with("("))
                            out += init;
                        else
                            out += "=" + init;
                    }
                    out += ";\n";
                    continue;
                }
                if (const auto* method = clang::dyn_cast<clang::CXXMethodDecl>(decl))
                {   // methods
                    if (method->isImplicit())
                        continue;

                    out += IndentBlock(ConstructFunctionSignature(method, false), 3, "   ");
                    continue;
                }
                if (const auto* funcTemplateDecl = dyn_cast<FunctionTemplateDecl>(decl))
                {
                    out += IndentBlock(ConstructFunctionSignature(funcTemplateDecl->getTemplatedDecl(), false), 3, "   ");
                    continue;
                }

                if (const auto* nested = clang::dyn_cast<clang::CXXRecordDecl>(decl))
                {
                    if (nested->isInjectedClassName())
                        continue;

                    if (nested->isLambda())
                        continue;

                    out += IndentBlock(ConstructRecordSignature(nested), 3, "   ");
                    continue;
                }
                if (clang::isa<clang::IndirectFieldDecl>(decl))
                    continue; // these are Clang's mechanism for exposing the members of an anonymous struct/union as if they were directly accessible in the enclosing class.

                if (const auto* enumDecl = clang::dyn_cast<clang::EnumDecl>(decl))
                {   // a locally defined enum
                    out += ConstructEnumDefinition(enumDecl) + ";";
                    continue;
                }

                if (const auto* nestedTemplate = clang::dyn_cast<clang::ClassTemplateDecl>(decl))
                {
                    const clang::CXXRecordDecl* templated = nestedTemplate->getTemplatedDecl();
                    if (!templated->isCompleteDefinition())
                    {
                        const clang::TemplateParameterList* params = nestedTemplate->getTemplateParameters();
                        out += "   " + ConstructTemplateParameterList(params);
                        out += templated->getKindName().str() + " ";
                        out += nestedTemplate->getNameAsString() + ";\n";
                        continue;
                    }

                    out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
                    continue;
                }

                if (auto* friendDecl = dyn_cast<FriendDecl>(decl))
                {
                    if (auto* namedFriendDecl = friendDecl->getFriendDecl())
                    {
                        if (auto* funcDecl = dyn_cast<FunctionDecl>(namedFriendDecl))
                        {
                            out += IndentBlock(ConstructFunctionSignature(funcDecl, false), 3, "   ");
                            continue;
                        }
                        if (auto* funcTemplateDecl = dyn_cast<FunctionTemplateDecl>(namedFriendDecl))
                        {
                            out += IndentBlock(ConstructFunctionSignature(funcTemplateDecl->getTemplatedDecl(), false), 3, "   ");
                            continue;
                        }
                        if (auto* classTemplateDecl = dyn_cast<ClassTemplateDecl>(namedFriendDecl))
                        {
                            const auto* templated = classTemplateDecl->getTemplatedDecl();
                            if (!templated->isCompleteDefinition())
                            {
                                const clang::TemplateParameterList* params = classTemplateDecl->getTemplateParameters();
                                out += "   " + ConstructTemplateParameterList(params) + "friend ";
                                out += templated->getKindName().str() + " ";
                                out += classTemplateDecl->getQualifiedNameAsString() + ";\n";
                                continue;
                            }

                            out += IndentBlock(ConstructRecordSignature(templated), 3, "   ");
                            continue;
                        }
                    }
                    if (auto* friendTypeSourceInfo = friendDecl->getFriendType())
                    {
                        out += "   friend " + friendTypeSourceInfo->getType().getAsString(printPolicy) + ";\n";
                        continue;
                    }
                }

                if (auto* typedefDecl = dyn_cast<TypedefDecl>(decl))
                {
                    out += IndentBlock(ConstructTypedefSignature(typedefDecl), out.size() - (out.rfind('\n') + 1) + 3); // length of last line up to current spot (+3 for indenting)
                    continue;
                }
                if (auto* typeAliasDecl = dyn_cast<TypeAliasDecl>(decl))
                {
                    std::string      aliasName = typeAliasDecl->getNameAsString();
                    QualType        underlying = typeAliasDecl->getUnderlyingType();
                    if (const auto* recordType = underlying->getAs<RecordType>())
                    {
                        if (recordType->getDecl()->isInAnonymousNamespace())
                        {
                            out += "   using " + aliasName + " = ";
                            out += IndentBlock(ConstructRecordSignature(dyn_cast<CXXRecordDecl>(recordType->getDecl())), 12 + aliasName.size());
                            continue;
                        }
                    }
                    out += "   using " + aliasName + " = " + underlying.getAsString(printPolicy) + ";\n";
                    continue;
                }
                if (const auto* tatDecl = dyn_cast<TypeAliasTemplateDecl>(decl))
                {
                    out += IndentBlock(ConstructTemplateAliasSignature(tatDecl), out.size() - (out.rfind('\n') + 1)); // length of last line up to current spot
                    continue;
                }
                    
                { // unhandled
                    out += "unhandled clang::Decl ";
                    out += decl->getDeclKindName();
                    out += " ";
                    if (const auto* named = clang::dyn_cast<clang::NamedDecl>(decl))
                        out += named->getNameAsString();
                    out += "\n";
                }
            }

            out += "};";
            return out;
        }
        std::string ConstructTemplateParameterList(const clang::TemplateParameterList* params)
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
                        out += defaultType.getAsString(printPolicy);
                    }
                    continue;
                }
                if (const auto* nttp = clang::dyn_cast<clang::NonTypeTemplateParmDecl>(param))
                {
                    bool handled = false;

                    const auto* enumTy = dyn_cast<clang::EnumType>(nttp->getType().getTypePtr());
                    if (enumTy && enumTy->getDecl()->isInAnonymousNamespace()) {
                        out += ConstructEnumDefinition(enumTy->getDecl());
                        if (!nttp->getName().empty())
                            out += " " + nttp->getName().str();
                        handled = true;
                    }

                    if (handled == false)
                    {   // NTTP where type is a struct (new to C++20)

                        QualType nttpQT = nttp->getType().getCanonicalType();

                        IndirectionCvStripper ics(nttpQT);
                        QualType baseQT    = ics.GetBaseType();
                        const Type* baseTy = baseQT.getTypePtr();

                        // Anonymous-namespace UDT NTTP
                        if (const auto* recTy = llvm::dyn_cast<clang::RecordType>(baseTy))
                        {
                            const auto* recDecl = recTy->getDecl();
                            if (recDecl->isInAnonymousNamespace())
                            {
                                out += ics.ConstructPrefix();
                                out += IndentBlock(ConstructRecordSignature(llvm::dyn_cast<CXXRecordDecl>(recDecl)), out.size() - (out.rfind('\n')+1));
                                out  = out.substr(0, out.size()-2); // strip ";\n"
                                out += ics.ConstructPointersAndReferences();

                                if (!nttp->getName().empty())
                                    out += " " + nttp->getName().str();

                                handled = true;
                            }
                            else if (const auto* spec = llvm::dyn_cast<ClassTemplateSpecializationDecl>(recDecl))
                            {
                                out += ics.ConstructPrefix();
                                out += spec->getQualifiedNameAsString();
                                out += IndentBlock(TemplateArgsToString(spec, false), out.size() - (out.rfind('\n') + 1));
                                out  = out.substr(0, out.size()-2); // strip ";\n"
                                out += ">" + ics.ConstructPointersAndReferences();

                                if (!nttp->getName().empty())
                                    out += " " + nttp->getName().str();

                                handled = true;
                            }
                        }
                    }
                    
                    if (handled == false) // still unhandled? Use fallback
                    {
                        std::string declStr;
                        llvm::raw_string_ostream declStream(declStr);
                        nttp->getType().print(declStream, printPolicy, nttp->getName());
                        out += declStr;
                    }

                    if (nttp->hasDefaultArgument())
                    {
                        std::string defaultStr;
                        llvm::raw_string_ostream defaultStream(defaultStr);
                        nttp->getDefaultArgument().getArgument().getAsExpr()->printPretty(defaultStream, nullptr, printPolicy);
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
                            out += innerNttp->getType().getAsString(printPolicy);
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
                        ttp2->getDefaultArgument().getArgument().getAsTemplate().print(defaultStream, printPolicy);
                        out += "=" + defaultStr;
                    }
                    continue;
                }
            }
            out += "> ";
            return out;
        }

        std::string AddClassInitializer(const clang::FieldDecl* fieldDecl)
        {
            std::string out;
            const Expr* expr      = fieldDecl->getInClassInitializer();
            const auto* declRef   = llvm::dyn_cast<clang::DeclRefExpr>(expr->IgnoreParenImpCasts());
            const auto* enumConst = declRef ? llvm::dyn_cast<clang::EnumConstantDecl>(declRef->getDecl()) : nullptr;
            if (enumConst)
            {
                const auto* enumDecl = llvm::dyn_cast<clang::EnumDecl>(enumConst->getDeclContext());
                out += "=" + ConstructEnumName(enumDecl, enumConst);
            }
            else
            {
                llvm::StringRef  text = clang::Lexer::getSourceText(CharSourceRange::getTokenRange(expr->getSourceRange()), context->getSourceManager(), context->getLangOpts());
                std::string      init = text.str();
                if ((init.starts_with("{")) || init.starts_with("("))
                    out += init;
                else
                    out += "=" + init;
            }
            return out;
        }

        std::string ConstructEnumName(const EnumDecl* enumDecl, const EnumConstantDecl* enumConst)
        {
            std::string out;

            if (enumDecl && enumDecl->isInAnonymousNamespace())
            {   // inline the full enum definition so the complete shape is compared
                std::string enumStr = enumDecl->isScoped() ? "enum class " : "enum ";
                enumStr += "(anonymous namespace)::";
                if (enumDecl->getIdentifier() != nullptr)
                    enumStr += enumDecl->getNameAsString() + " ";
                else
                    enumStr += "(unnamed enum) ";
                if (enumDecl->isFixed())
                    enumStr += ": " + enumDecl->getIntegerType().getCanonicalType().getAsString() + " ";
                enumStr += "{ ";
                for (const clang::EnumConstantDecl* e : enumDecl->enumerators())
                    enumStr += e->getNameAsString() + "=" + llvm::toString(e->getInitVal(), 10) + ", ";
                if (enumDecl->enumerators().empty())
                    enumStr += "}";
                else
                    enumStr = enumStr.substr(0, enumStr.size() - 2) + " }";
                out += enumStr + (enumConst != nullptr ? "::" + enumConst->getNameAsString() : "");
            }
            else if (enumDecl)
                out += enumDecl->getQualifiedNameAsString() + (enumConst != nullptr ? "::" + enumConst->getNameAsString() : "");
            else if (enumConst)
                out += enumConst->getNameAsString();
            // else not an enum
            return out;
        }
        std::string ConstructEnumDefinition(const clang::EnumDecl* enumDecl)
        {
            std::string underlyingType = enumDecl->getIntegerType().getCanonicalType().getAsString();
            bool              isScoped = enumDecl->isScoped();   // enum class vs enum
            bool               isFixed = enumDecl->isFixed();
            std::string       enumName = enumDecl->getNameAsString();
            std::string     prettyEnum = enumName == "" ? makeUnnamedEnumKey(enumDecl) : enumDecl->getQualifiedNameAsString();

            std::string fqe = (isScoped ? "enum class " : "enum ") + prettyEnum + (isFixed ? " : " + underlyingType : "") + " { ";
            for (const clang::EnumConstantDecl* enumeratorDecl : enumDecl->enumerators())
            {
                std::string enumeratorName = enumeratorDecl->getName().str();
                std::string val = llvm::toString(enumeratorDecl->getInitVal(), 10);
                fqe += enumeratorName + "=" + val + ", ";
            }
            fqe = fqe.substr(0, fqe.size() - 2) + " }";
            return fqe;
        }

        std::string ConstructAttribute(const Attr* attr)
        {
            std::string out;

            if (const auto* alignedAttr = clang::dyn_cast<clang::AlignedAttr>(attr))
            {
                if (alignedAttr->isAlignmentExpr())
                {
                    const clang::Expr* expr = alignedAttr->getAlignmentExpr();
                    if (expr && expr->isIntegerConstantExpr(*context))
                    {
                        auto optInt = expr->getIntegerConstantExpr(*context);
                        if (optInt.has_value())
                        {
                            out += "alignas(" + std::to_string(optInt.value().getExtValue()) + ") ";
                            return out;
                        }
                    }
                }
                // alignment specified as a type: alignas(SomeType)
                out += "alignas(" + alignedAttr->getAlignmentType()->getType().getAsString() + ") ";
                return out;
            }
            if (const auto* nodiscard = clang::dyn_cast<clang::WarnUnusedResultAttr>(attr))
            {
                const llvm::StringRef msg = nodiscard->getMessage();
                out += msg.empty() ? "[[nodiscard]] " : ("[[nodiscard(\"" + msg.str() + "\")]] ");
                return out;
            }
            if (const auto* deprecated = clang::dyn_cast<clang::DeprecatedAttr>(attr))
            {
                const llvm::StringRef msg = deprecated->getMessage();
                out += msg.empty() ? "[[deprecated]] " : ("[[deprecated(\"" + msg.str() + "\")]] ");
                return out;
            }

            // filter out final (maybe more?)
            if (const auto* Final = clang::dyn_cast<clang::FinalAttr>(attr))
                return out;

            // attributes other than alignas, nodiscard and deprecated, nor final
            std::string raw;
            llvm::raw_string_ostream os(raw);
            attr->printPretty(os, printPolicy);
            os.flush();

            // strip off ("")
            constexpr std::string_view empty_parens = "(\"\")";
            if (auto pos = raw.find(empty_parens); pos != std::string::npos)
                raw.replace(pos, empty_parens.size(), "");
            out += raw + " ";

            return out;
        }
        std::string ConstructAttributes(const Decl* decl)
        {
            std::string out;
            for(const auto* attr : decl->attrs())
                out += ConstructAttribute(attr);
            return out;
        }
    private:
        static std::string IndentBlock(const std::string& block, size_t indentWidth, const std::string& firstLinePrefix = "")
        {
            std::istringstream iss(block);
            std::string indentation(indentWidth, ' ');
            std::string out;
            bool first = true;
            for (std::string line; std::getline(iss, line);)
            {
                if (first) {
                    first = false;
                    out += firstLinePrefix + line + "\n";
                } else
                    out +=     indentation + line + "\n";
            }
            return out;
        }
#endif
    };

    class VisitorConsumer : public ASTConsumer
    {
        TheVisitor visitor;
    public:
        VisitorConsumer(ASTContext* context, AllMaps& maps, const std::string& TU) : visitor(context, maps, TU) {}
        void HandleTranslationUnit(ASTContext& context) override
        {
            visitor.TraverseDecl(context.getTranslationUnitDecl());
        }
    };

    class VisitorAction : public ASTFrontendAction
    {
        AllMaps& maps;
    public:
        explicit VisitorAction(AllMaps& maps) : maps(maps) {}
        std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& CI, llvm::StringRef InFile) override
        {
            return std::make_unique<VisitorConsumer>(&CI.getASTContext(), maps, InFile.str());
        }
    };

    class VisitorActionFactory : public clang::tooling::FrontendActionFactory
    {
        AllMaps& maps;
    public:
        explicit VisitorActionFactory(AllMaps& maps) : maps(maps) {}
        std::unique_ptr<clang::FrontendAction> create() override { return std::make_unique<VisitorAction>(maps); }
    };


    struct OdrViolationReporter
    {
        template<typename Out> static int ReportOdrViolations(const AllMaps& maps, Out&& out)
        {
            int violations = 0;
            violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.    enumMap, out);
            violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.functionMap, out);
            violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps. typedefMap, out);
            violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.     udtMap, out);
            violations += OdrCop3::OdrViolationReporter::ReportOdrViolations(maps.     varMap, out);
            return violations;
        }
        template<typename T, typename Out> static int ReportOdrViolations(const std::map<std::string,T>& map, Out&& out)
        {
            int violationCount = 0;
            for (auto& [name, items] : map)
            {
                if (items.size() < 2)
                    continue;

                if (std::all_of(items.begin() + 1, items.end(), [&](const auto& x) { return x == items[0]; }))
                    continue;

                ++violationCount;
                out << "\nODR VIOLATION: " << name << '\n';

                std::vector<bool> printed(items.size(), false);
                for (size_t i=0; i<items.size(); ++i)
                {
                    if (printed[i])
                        continue;

                    out << "[" << items[i].TU << "]\n";
                    out        << items[i].fullyQualified << '\n';
                    printed[i] = true;

                    for (size_t j=i+1; j<items.size(); ++j)
                    {
                        if (!printed[j] && (items[i] == items[j]))
                        {
                            out << "[" << items[j].TU << "] - same as above\n";
                            printed[j] = true;
                        }
                    }
                }
            }
            return violationCount;
        }
    };
}