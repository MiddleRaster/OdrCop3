#pragma once

#include <vector>
#include <string>
#include <map>

#include "..\src\TemplateSerializationUtils.h"
#include "..\src\SerializeDecls.h"
#include "..\src\SerializeTypes.h"
#include "..\src\SerializeExprs.h"

namespace OdrCop3
{
    enum InfoKind { Function, Typedef, Concept, Enum, Udt, Var };
    template<InfoKind K> struct InfoBase
    {
        const std::string TU;
        const std::string fullyQualified;
        bool operator==(const InfoBase& other) const { return fullyQualified == other.fullyQualified; }
    };
    using FunctionInfo = InfoBase<InfoKind::Function>;
    using  TypedefInfo = InfoBase<InfoKind::Typedef>;
    using  ConceptInfo = InfoBase<InfoKind::Concept>;
    using     EnumInfo = InfoBase<InfoKind::Enum>;
    using      UdtInfo = InfoBase<InfoKind::Udt>;
    using      VarInfo = InfoBase<InfoKind::Var>;

    struct AllMaps
    {
        std::map<std::string,std::vector<     UdtInfo>>      udtMap;
        std::map<std::string,std::vector<     VarInfo>>      varMap;
        std::map<std::string,std::vector<    EnumInfo>>     enumMap;
        std::map<std::string,std::vector< TypedefInfo>>  typedefMap;
        std::map<std::string,std::vector< ConceptInfo>>  conceptMap;
        std::map<std::string,std::vector<FunctionInfo>> functionMap;
    };

    class TheVisitor : public RecursiveASTVisitor<TheVisitor>
    {
        std::unordered_set<const clang::Decl*> recursingDecls;
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
            , contextItems (context, printPolicy, this->TU, recursingDecls)
        {}
    private:
        static std::string SerializeDecls(const ContextItems& contextItems, const clang::Decl  * decl) { return Serialize::Decls<                 &SerializeTypes, &SerializeExprs>(contextItems, decl); }
        static std::string SerializeTypes(const ContextItems& contextItems, const clang::QualType& qt) { return Serialize::Types<&SerializeDecls,                  &SerializeExprs>(contextItems, qt  ); }
        static std::string SerializeExprs(const ContextItems& contextItems, const clang::Expr  * expr) { return Serialize::Exprs<&SerializeDecls, &SerializeTypes                 >(contextItems, expr); }
    public:
        bool VisitFunctionDecl(FunctionDecl* functionDecl)
        {
            if (context->getSourceManager().isInSystemHeader(functionDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (functionDecl->isImplicit())
                return true;

            if (isa<CXXMethodDecl>(functionDecl))
                return true; // if it's a method, it's already in the UDT map

            if (functionDecl->getStorageClass() == clang::SC_Static || functionDecl->isInAnonymousNamespace())
                return true; // if the function has internal-linkage, skip it

            switch (functionDecl->getTemplateSpecializationKind()) { // same deal as with class templates
            case TemplateSpecializationKind::TSK_ExplicitInstantiationDeclaration:
            case TemplateSpecializationKind::TSK_ExplicitInstantiationDefinition:
            case TemplateSpecializationKind::TSK_ImplicitInstantiation:     
                return true; // no bodies; skip
            default:
                break;
            }

            if (!functionDecl->isThisDeclarationADefinition())
                return true;

            if (const FunctionTemplateDecl* functionTemplateDecl = functionDecl->getDescribedFunctionTemplate())
                maps.functionMap[CreateKeyForFunctionMap(functionDecl)].push_back({TU, SerializeDecls(contextItems, functionTemplateDecl)});
            else
                maps.functionMap[CreateKeyForFunctionMap(functionDecl)].push_back({TU, SerializeDecls(contextItems, functionDecl)});
            return true;
        }

        bool shouldVisitTemplateInstantiations() const { return true; }
        bool shouldVisitImplicitCode          () const { return true; }
        bool VisitClassTemplateDecl(clang::ClassTemplateDecl* classTemplateDecl)
        {
            if (context->getSourceManager().isInSystemHeader(classTemplateDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (classTemplateDecl->isInAnonymousNamespace())
                return true; // if the class/struc/union has internal-linkage, skip it

            if (!classTemplateDecl->isThisDeclarationADefinition())
                return true;

            if (classTemplateDecl->isCXXClassMember())
                return true; // already defined nested inside another UDT

            std::string key = classTemplateDecl->getQualifiedNameAsString();
            if (auto* CTSD = dyn_cast<ClassTemplateSpecializationDecl>(classTemplateDecl))
                key += TemplateArgsToString(contextItems, CTSD);
            else
                key += "<>";

            maps.udtMap[key].push_back({TU,SerializeDecls(contextItems, classTemplateDecl)});
            return true;
        }
        bool VisitCXXRecordDecl(CXXRecordDecl* cxxRecordDecl)
        {
            if (context->getSourceManager().isInSystemHeader(cxxRecordDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (cxxRecordDecl->isInAnonymousNamespace())
                return true; // if the class/struc/union has internal-linkage, skip it

            if (cxxRecordDecl->isLambda())
                return true; // skip lambdas

            if (cxxRecordDecl->getDescribedClassTemplate())
                return true; // has its own handler

            if (auto* classTemplateSpec = dyn_cast<ClassTemplateSpecializationDecl>(cxxRecordDecl))
                switch (classTemplateSpec->getSpecializationKind())
                {
                case TemplateSpecializationKind::TSK_ImplicitInstantiation:             // skip implicit because the user can't see it
                case TemplateSpecializationKind::TSK_ExplicitInstantiationDeclaration:  // e.g., extern template struct Foo<int>;
                case TemplateSpecializationKind::TSK_ExplicitInstantiationDefinition:   // e.g., template struct Foo<int>;
                    return true;
                default:
                    break;
                }

            if (!cxxRecordDecl->isThisDeclarationADefinition())
                return true;

            if (cxxRecordDecl->isCXXClassMember())
                return true; // already defined nested inside another UDT

            std::string key = cxxRecordDecl->getQualifiedNameAsString();
            if      (cxxRecordDecl->getDescribedClassTemplate())                                    key += "<>";
            else if (auto* CTPSD = dyn_cast<ClassTemplatePartialSpecializationDecl>(cxxRecordDecl)) key += TemplateArgsToString(contextItems, CTPSD);
            else if (auto* CTSD  = dyn_cast<ClassTemplateSpecializationDecl>       (cxxRecordDecl)) key += TemplateArgsToString(contextItems, CTSD);

            // class template specialization and partial specializations are subclasses of CXXRecordDecl
            if      (const ClassTemplatePartialSpecializationDecl* ctpsd = dyn_cast<ClassTemplatePartialSpecializationDecl>(cxxRecordDecl))
                maps.udtMap[key].push_back({TU,SerializeDecls(contextItems, ctpsd)});
            else if (const ClassTemplateSpecializationDecl       *  ctsd = dyn_cast<ClassTemplateSpecializationDecl       >(cxxRecordDecl))
                maps.udtMap[key].push_back({TU,SerializeDecls(contextItems, ctsd)});
            else
                maps.udtMap[key].push_back({TU,SerializeDecls(contextItems, cxxRecordDecl)});
            return true;
        }
        bool VisitEnumDecl(clang::EnumDecl* enumDecl)
        {
            if (context->getSourceManager().isInSystemHeader(enumDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (enumDecl->isInAnonymousNamespace())
                return true; // TU-local, not an ODR candidate

            if (!enumDecl->isThisDeclarationADefinition())
                return true;

            if (enumDecl->getDeclContext()->isFunctionOrMethod())
                return true; // enums local to function are covered by the body; no need to duplicate it here
            if (llvm::isa<clang::RecordDecl>(enumDecl->getDeclContext()))
                return true; // enums local to UDTs     are covered by the  UDT; no need to duplicate it here

            std::string  key = enumDecl->getNameAsString() == "" ? MakeUnnamedEnumKey(contextItems, enumDecl) : enumDecl->getQualifiedNameAsString();
            maps.enumMap[key].push_back({TU, SerializeDecls(contextItems, enumDecl)});
            return true;
        }
        bool VisitTypedefNameDecl(clang::TypedefNameDecl* typedefDecl)
        {
            if (context->getSourceManager().isInSystemHeader(typedefDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (typedefDecl->isImplicit())
                return true;

            if (typedefDecl->isInAnonymousNamespace())
                return true; // TU-local, not an ODR candidate

            if (const auto* tad = llvm::dyn_cast<TypeAliasDecl>(typedefDecl))
                if (tad->getDescribedAliasTemplate() != nullptr)
                    return true; // Skip the templated decl inside a TypeAliasTemplateDecl - handled below

            if (typedefDecl->getDeclContext()->isRecord())
                return true; // nested, not a top-level typedef

            std::string aliasName = typedefDecl->getQualifiedNameAsString();
            maps.typedefMap[aliasName].push_back({TU, SerializeDecls(contextItems, typedefDecl)});
            return true;
        }
        bool VisitTypeAliasTemplateDecl(clang::TypeAliasTemplateDecl* typeAliasTemplateDecl)
        {
            if (context->getSourceManager().isInSystemHeader(typeAliasTemplateDecl->getLocation()))
                return true;

            if (typeAliasTemplateDecl->isInAnonymousNamespace())
                return true;

            if (typeAliasTemplateDecl->getDeclContext()->isRecord())
                return true; // nested, not a top-level using alias

            std::string     aliasName = typeAliasTemplateDecl->getQualifiedNameAsString();
            maps.typedefMap[aliasName].push_back({TU, SerializeDecls(contextItems, typeAliasTemplateDecl)});
            return true;
        }

        bool VisitVarDecl(const VarDecl* varDecl)
        {
            if (context->getSourceManager().isInSystemHeader(varDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (isa<ParmVarDecl>(varDecl))
                return true; // skipping parameters to functions/methods
            if (varDecl->isLocalVarDecl())
                return true; // local variables can't be ODR violations
            if (isa<CXXRecordDecl>(varDecl->getDeclContext()))
                if (!varDecl->isOutOfLine()) // an out-of-class initialization
                    return true; // struct/class statics:  already done during CXXRecordDecl parsing

            auto linkage = varDecl->getLinkageAndVisibility().getLinkage();
            if (linkage != clang::Linkage::External &&
                linkage != clang::Linkage::UniqueExternal)
                return true; // internal linkage (anon namespace, static, const at file scope, etc.)

            std::string key = varDecl->getQualifiedNameAsString();
            if (varDecl->getDescribedVarTemplate())
                key += "<>";
            else if (auto* vtsd = dyn_cast<VarTemplateSpecializationDecl>(varDecl))
            {
                switch (vtsd->getSpecializationKind())
                {
                case clang::TemplateSpecializationKind::TSK_ImplicitInstantiation:              // the compiler generated this
                case clang::TemplateSpecializationKind::TSK_ExplicitInstantiationDeclaration:   // Programmer requested code generation, but did not define a new entity
                case clang::TemplateSpecializationKind::TSK_ExplicitInstantiationDefinition:    // extern template; only a promise
                    return true; // skip these, as they're not relevant to ODR violation detection
                default:
                    break;
                }
                if (auto* vtpsd = dyn_cast<VarTemplatePartialSpecializationDecl>(varDecl))
                    key += TemplateArgsToString(contextItems, vtpsd);
                else
                    key += TemplateArgsToString(contextItems, vtsd);
            }

            maps.varMap[key].push_back({TU, SerializeDecls(contextItems, varDecl)});
            return true;
        }
        bool VisitConceptDecl(const clang::ConceptDecl* conceptDecl)
        {
            if (context->getSourceManager().isInSystemHeader(conceptDecl->getLocation()))
                return true; // skip anything not in the main file or a user header

            if (conceptDecl->getLinkageAndVisibility().getLinkage()!= Linkage::External)
                return true;

            std::string key = conceptDecl->getQualifiedNameAsString();
            maps.conceptMap[key].push_back({TU, SerializeDecls(contextItems, conceptDecl)});
            return true;
        }

    private:
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
                    if (const auto* ttp = llvm::dyn_cast<TemplateTypeParmDecl>(param); ttp && ttp->isImplicit())
                    {                  // abbreviated functions get auto, even though it's not legal C++
                        key += "auto"; // necessary because one can write "template<typename T> void HalfAbbreviated(T t, auto x) {}"
                        continue;
                    }
                
                    std::string s;
                    llvm::raw_string_ostream os(s);
                    param->print(os, contextItems.printPolicy);
                    os.flush();
                    key += s;
                }
                key += ">";
            }
            else if (const FunctionDecl* primaryTemplate = funcDecl->getPrimaryTemplate() ? funcDecl->getPrimaryTemplate()->getTemplatedDecl() : nullptr)
            {   // explicit instantiation
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

            key += "(";

            // all parameters, without typedefs; no parameter names. Include "...".
            for (const ParmVarDecl* param : funcDecl->parameters())
            {
                if (param != *funcDecl->param_begin())
                    key += ",";

                std::string typeStr;

                QualType qt = param->getType();
                if (qt->isDependentType())
                    typeStr = qt.getAsString(printPolicy);                                         // Preserve template spelling: T, Args, TT<T>, etc.
                else if (const auto* templateSpecType = qt->getAs<TemplateSpecializationType>())
                    typeStr = qt.getAsString(printPolicy);                                         // Preserve template arguments: Wrapper<int>
                else if (const auto* tagType = qt->getAs<TagType>())
                    typeStr = tagType->getDecl()->getQualifiedNameAsString();                      // Preserve qualified names, including anonymous namespaces.
                else
                    typeStr = qt.getCanonicalType().getUnqualifiedType().getAsString(printPolicy); // Normalize ordinary types.

                const std::string from = "anonymous namespace";
                if (auto pos = typeStr.find(from); pos != std::string::npos)
                    typeStr.replace(pos, from.size(), "anonymous namespace in " + TU);
                
                key += typeStr;
            }
            if (funcDecl->isVariadic())
                key += funcDecl->param_empty() ? "..." : ",...";
            key += ")";
            return key;
        }
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