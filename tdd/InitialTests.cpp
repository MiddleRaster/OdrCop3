#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test ExploratoryTestsOfClangAST[] =
{
    {"Initial Serialize::Decls and FunctionDeclSerializer test", []
        {
            std::string code = "[[maybe_unused]] void foo(volatile int* i=nullptr) noexcept { (void)i; }";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(1, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found a map entry");

            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("input.cc", vec[0].TU, "should have gotten the TU name");
            Assert::AreEqual("[[maybe_unused]] void __cdecl foo(volatile int * i = nullptr) noexcept { (void)i; }\n", vec[0].fullyQualified, "should have gotten the function and body");
        }
    },
    {"Testing FunctionDeclSerializer on methods", []
        {
            std::string code = "export module M;\nexport struct Bar { virtual const Bar * GetBar() const & = 0; };"
                               "export void Baz([[deprecated]] int x, int y [[maybe_unused]]) {}"
                               "struct Foo : Bar {"
                               "   struct { int x; };"
                               "   int i;"
                               "   explicit Foo(int i) : x(7), i(i) { this->i++; }"
                               "   [[nodiscard]] const Bar* GetBar() const& override { return this; }"
                               "   auto make_lambda() const { return [this](int x) { return x + i; }; }"
                               "   template <typename T> T doTemplateyStuff(const T& value) const requires requires { typename T::value_type; } { return value; }"
                               "   explicit operator int() const { return 7; }"
                               "}; ";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(9, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found UDTs and functions/methods");

            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");
                Assert::AreEqual("export void __cdecl Baz([[deprecated]] int x, int y [[maybe_unused]]) {}\n",    (*it++).second[0].fullyQualified, "should have gotten the function and body");
                Assert::AreEqual("explicit void __cdecl Foo(int i) : x(7), i(i) { i++; }\n",                      (*it++).second[0].fullyQualified, "should have gotten the ctor and body");
                Assert::AreEqual("[[nodiscard]] const Bar * __cdecl GetBar() const & override { return this; }\n",(*it++).second[0].fullyQualified, "should have gotten the method and body");
                Assert::AreEqual("template <typename T> T __cdecl doTemplateyStuff(const T & value) "
                                 "requires requires { typename T::value_type; } const { return value; }\n",       (*it++).second[0].fullyQualified, "should have gotten the method and body");
                Assert::AreEqual("auto __cdecl make_lambda() const {\n"
                                 "    return [this](int x) {\n"
                                 "        return x + i;\n"
                                 "    };\n"
                                 "}\n",                                                                           (*it++).second[0].fullyQualified, "should have gotten the operator and body");
                Assert::AreEqual("explicit int __cdecl operator int() const { return 7; }\n",                     (*it++).second[0].fullyQualified, "should have gotten the operator and body");
            }
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Bar { // sizeof=8\n"
                                 "   export virtual const Bar * __cdecl GetBar() const & =0;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
                Assert::AreEqual("struct Foo : public Bar { // sizeof=16\n"
                                "   struct Foo::(anonymous type at input.cc:2:150) { // sizeof=4\n"
                                "      int x;\n"
                                "   };\n"
                                "   int i;\n"
                                "   explicit void __cdecl Foo(int i) : x(7), i(i) { i++; }\n"
                                "   [[nodiscard]] const Bar * __cdecl GetBar() const & override { return this; }\n"
                                "   auto __cdecl make_lambda() const {\n"
                                "       return [this](int x) {\n"
                                "           return x + i;\n"
                                "       };\n"
                                "   }\n"
                                "   template <typename T> T __cdecl doTemplateyStuff(const T & value) requires requires { typename T::value_type; } const { return value; }\n"
                                "   explicit int __cdecl operator int() const { return 7; }\n"
                                "};\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the struct");
            }
        }
    },

    {"Testing CXXRecordDeclSerializer on UDTs", []
        {
            std::string code = "struct Qux {}; struct Bar {}; struct Baz{}; struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : Baz, virtual private Bar, protected Qux {"
                               "public: [[deprecated(\"use y instead\")]] constexpr static int x = 0; "
                               "   inline static int y{0};"
                               "   int b:3=1;"
                               "   unsigned int c:2{3};"
                               "};"
                               "struct S { union { int a; double b; } u; };";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(6, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found a map entry");

            auto it = maps.udtMap.begin();
            Assert::AreEqual("Bar",      it->first,        "should have gotten correct key");
            Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

            Assert::AreEqual("struct Bar { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Baz { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : public Baz, private virtual Bar, protected Qux { // sizeof=32\n"
                             "public:\n"
                             "   [[deprecated(\"use y instead\")]] constexpr static const int x=0;\n"
                             "   inline static int y{0};\n"
                             "   int b:3=1;\n"
                             "   unsigned int c:2{3};\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Qux { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");

            Assert::AreEqual("struct S { // sizeof=8\n"
                             "   union S::(anonymous type at input.cc:1:294) { // sizeof=8\n"
                             "      int a;\n"
                             "      double b;\n"
                             "   };\n"
                             "   union S::(anonymous type at input.cc:1:294) u;\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("union S::(anonymous type at input.cc:1:294) { // sizeof=8\n"
                             "   int a;\n"
                             "   double b;\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
        }
    },

    {"Testing EnumDecl serialization", []
        {
            std::string code = "enum E { A=1, B, C };\n"
                               "struct S { E e=E::B; };\n"
                               "struct N { enum { D=0, E, F } eVal=E; };";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(4, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found an enum entry");

            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("E",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("enum E { A=1, B=2, C=3 };\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the enum");
                Assert::AreEqual("enum N::(anonymous type at input.cc:3:12) { D=0, E=1, F=2 };\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the enum");
            }
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("N", it->first,               "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct N { // sizeof=4\n"
                                 "   enum N::(anonymous type at input.cc:3:12) { D=0, E=1, F=2 };\n"
                                 "   enum N::(anonymous type at input.cc:3:12) eVal=E;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
                Assert::AreEqual("struct S { // sizeof=4\n"
                                 "   E e=E::B;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
            }
        }
    },

    {"Testing Typedef serialization", []
        {
            std::string code = "typedef int MyInt;\n"
                               "typedef enum { Red, Green, Blue } Color;\n";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(3, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found an enum entry");

            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("Color",    it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("using Color = enum (anonymous type at input.cc:2:9) { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous type at input.cc:2:9) { Red=0, Green=1, Blue=2 } Color;\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the typedef");
                Assert::AreEqual("using MyInt = int; // typedef int MyInt;\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the typedef");
            }
        }
    },

    {"Testing SerializeTypes and SerializeTypeRecord", []
        {
            std::string code = "namespace { struct Foo { [[maybe_unused]] Foo* foo; }; } struct Bar : Foo {};"
                               "struct S {}; struct A { S* p; const S& q; S r[2]; const Foo* a; Foo& b; Foo c[2]; Foo**& d; void (*callback)(S*); void (*callback2)(Foo*, int, double);"
                               "Foo (S::* mp)(double, const char*) = nullptr;"
                               "int (Foo::*mp2)(int); };";
 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(3, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found an enum entry");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("A",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct A { // sizeof=96\n"
                                 "   S *p;\n"
                                 "   const S &q;\n"
                                 "   S r[2];\n"
                                 "   const struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "            [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "         } *a;\n"
                                 "   struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "      [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "   } &b;\n"
                                 "   struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "      [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "   } c[2];\n"
                                 "   struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "      [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "   } * * &d;\n"
                                 "   void (*callback)(S *);\n"
                                 "   void (*callback2)(struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "                        [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "                     } *, int, double);\n"
                                 "   struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "      [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "   } (S::*mp)(double, const char *)=nullptr;\n"
                                 "   int (struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "           [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "        }::*mp2)(int);\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the class");
                Assert::AreEqual("struct Bar : public struct (anonymous namespace)::Foo { // sizeof=8\n"
                                 "                       [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "                    } { // sizeof=8\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the anonymous namespace base class");
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the class");
            }
        }
    },

    {"Testing TypedefDecl and TypeAliasDecl", []
        {
            std::string code = "struct S {}; using Alias = S; typedef S Alias2;\n"
                               "typedef     enum        { Red, Green, Blue } Color;\n" // Note: don't change the spacing as that changes the TU:line:col
                               "namespace {                            using Color2 = Color; }\n"
                               "namespace { enum Color3 { Red, Green, Blue }; }"
                               "            enum Color4 { Red, Green, Blue };\n"
                               "typedef int (*TypedefForPointerToFunction)(double, const char*);"
                               "using UsingAliasForPointerToFunction = int (*)(double, const char*);"
                               "namespace { typedef int (*AnonNamespaceTypedefToFuncionPointer)(int, double); }"
                               "namespace { using AnonNamespaceUsingAliasToFunctionPointer = int (*)(int, double); }"
                               "template <typename R, typename... Args> using TemplateUsingAliasToPointerToFunction = R(*)(Args...);"
                               "template <typename R, typename... Args> using NoexceptFuncPtr = R(*)(Args...) noexcept;"
                               "template <typename R, typename... Args> using VariadicFuncPtr = R(*)(Args..., ...);"
                               "template <typename C, typename R, typename... Args> using MemberFuncPtr = R(C::*)(Args...);"
                               "template <auto F, typename... Args> using ReturnTypeOf = decltype(F(Args{}...));\n"
                               "template <auto F, auto... Fs> using AllTrue = decltype((F() && ... && Fs()));\n"
                               "template <typename T> struct Inner { using type = T; };"
                               "template <template <typename> class Inner, typename U> struct Outer { using type = typename Inner<U>::type; };"
                               "template <typename X> struct Wrap { using type = X; };"
                               "template <\n"
                               "    template <\n"
                               "        template <typename T> class Inner,\n"
                               "        typename U\n"
                               "    > class Outer,\n"
                               "    template <typename X> class Wrap\n"
                               ">\n"
                               "using RecursiveAlias = typename Outer<Wrap, int>::type;\n"
                               "struct A { Alias member; Alias2 member2;\n"
                               "   Color  color;"
                               "   Color2 color2;"
                               "   Color3 color3;"
                               "   Color4 color4;"
                               "   TypedefForPointerToFunction              tdpfn1;"
                               "   UsingAliasForPointerToFunction           tdpfn2;"
                               "   AnonNamespaceTypedefToFuncionPointer     tdpfn3;"
                               "   AnonNamespaceUsingAliasToFunctionPointer tdpfn4;"
                               "   TemplateUsingAliasToPointerToFunction<void, double, const char*, int, TemplateUsingAliasToPointerToFunction<int, float, double, const char*>, int, int> tuapfn;\n"
                               "   NoexceptFuncPtr<void, int, long, long long, float, double, long double> tuapfnne;"
                               "   VariadicFuncPtr<int> tuapfnv;"
                               "   MemberFuncPtr<S, int, const char*> tuapfm;"
                               "   ReturnTypeOf<[](auto x, auto y) { return x*y; }, int, long> fieldDemoingReturnTypeOfNTTP;"
                               "   AllTrue<[]{ return true; },[]{ return true; },[]{ return false; }> allTrueField; "
                               "   RecursiveAlias<Outer, Wrap> recursivelyDefinedField;"
                               "};";
 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            {
                Assert::AreEqual(15, maps.typedefMap.size(), "wrong number of typedef/aliases in map");

                auto it = maps.typedefMap.begin();
                Assert::AreEqual("(anonymous namespace)::AnonNamespaceTypedefToFuncionPointer", it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc",                                                    it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("using (anonymous namespace)::AnonNamespaceTypedefToFuncionPointer = int (*)(int, double); // typedef int (*)(int, double) (anonymous namespace)::AnonNamespaceTypedefToFuncionPointer;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using (anonymous namespace)::AnonNamespaceUsingAliasToFunctionPointer = int (*)(int, double); // typedef int (*)(int, double) (anonymous namespace)::AnonNamespaceUsingAliasToFunctionPointer;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using (anonymous namespace)::Color2 = enum (anonymous type at input.cc:2:13) { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous type at input.cc:2:13) { Red=0, Green=1, Blue=2 } (anonymous namespace)::Color2;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Alias = S; // typedef S Alias;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Alias2 = S; // typedef S Alias2;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <auto F, auto... Fs> using AllTrue = decltype((F() && ... && Fs())); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Color = enum (anonymous type at input.cc:2:13) { Red=0, Green=1, Blue=2 }; // typedef enum (anonymous type at input.cc:2:13) { Red=0, Green=1, Blue=2 } Color;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename C, typename R, typename... Args> using MemberFuncPtr = R (C::*)(Args...); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename... Args> using NoexceptFuncPtr = R (*)(Args...) noexcept; // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <template <template <typename T> class Inner, typename U> class Outer, template <typename X> class Wrap> using RecursiveAlias = typename Outer<Wrap, int>::type; // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <auto F, typename... Args> using ReturnTypeOf = decltype(F(Args{}...)); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename... Args> using TemplateUsingAliasToPointerToFunction = R (*)(Args...); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using TypedefForPointerToFunction = int (*)(double, const char *); // typedef int (*)(double, const char *) TypedefForPointerToFunction;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using UsingAliasForPointerToFunction = int (*)(double, const char *); // typedef int (*)(double, const char *) UsingAliasForPointerToFunction;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename... Args> using VariadicFuncPtr = R (*)(Args..., ...); // no typedef equivalent\n"
                             , (*it++).second[0].fullyQualified);
            }
            {
                Assert::AreEqual(5, maps.udtMap.size(), "wrong number of UDTs in map");
                auto it = maps.udtMap.begin();

                Assert::AreEqual("struct A { // sizeof=104\n"
                                 "   S member;\n"
                                 "   S member2;\n"
                                 "   Color color;\n"
                                 "   Color color2;\n"
                                 "   enum (anonymous namespace)::Color3 { Red=0, Green=1, Blue=2 } color3;\n"
                                 "   Color4 color4;\n"
                                 "   int (*tdpfn1)(double, const char *);\n"
                                 "   int (*tdpfn2)(double, const char *);\n"
                                 "   int (*tdpfn3)(int, double);\n"
                                 "   int (*tdpfn4)(int, double);\n"
                                 "   void (*tuapfn)(double, const char *, int, int (*)(float, double, const char *), int, int);\n"
                                 "   void (*tuapfnne)(int, long, long long, float, double, long double) noexcept;\n"
                                 "   int (*tuapfnv)(...);\n"
                                 "   int (S::*tuapfm)(const char *);\n"
                                 "   long fieldDemoingReturnTypeOfNTTP;\n"
                                 "   bool allTrueField;\n"
                                 "   int recursivelyDefinedField;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Inner {\n"
                                 "   using Inner::type = T; // typedef T Inner::type;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<template<typename> class Inner, typename U> struct Outer {\n"
                                 "   using Outer::type = Inner<U>::type; // typedef Inner<U>::type Outer::type;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename X> struct Wrap {\n"
                                 "   using Wrap::type = X; // typedef X Wrap::type;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);

            }
            {
                Assert::AreEqual(2, maps.enumMap.size(), "wrong number of enums in map");
                auto it = maps.enumMap.begin();

                Assert::AreEqual("enum (anonymous type at input.cc:2:13) { Red=0, Green=1, Blue=2 };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum Color4 { Red=0, Green=1, Blue=2 };\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },

    /*

    5. Typedefs

    struct S {};
    using Alias = S;
    struct A
    {
        Alias member;
    };


    FieldDecl
     |
     TypedefType
          |
          +-- TypedefNameDecl
                  |
                  +-- RecordType


    6. Template specializations

    struct S {};
    template<typename T>
    struct Box {};
    struct A
    {
        Box<S> value;
    };


    TemplateSpecializationType
            |
            +-- TemplateDecl
            |
            +-- TemplateArgument
                    |
                    +-- RecordType


    ?

    7. Pointer-to-member (the oddball)

    struct S
    {
        int x;
    };
    struct A
    {
        int S::* pm;
    };

    */

    {"Testing ClassTemplateSpecialization", []
        {
            std::string code = "template<typename T> struct Box{}; struct S{}; struct A { Box<S> value; };\n";
 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.enumMap.size(), "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct A { // sizeof=1\n"
                                 "   Box<S> value;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Box {\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
            }
        }
    },

};
