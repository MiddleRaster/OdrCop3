#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test ExploratoryTestsOfClangAST[] =
{
    {"Initial Serialize::Decls and FunctionDeclSerializer test", []
        {
            std::string code = "[[maybe_unused]] void foo(volatile int* i=nullptr) noexcept { (void)i; }\n"
                               "template<typename T> T multiply(T a, T b) { return a*b; }\n"
                               "struct complex { double r; double i; }; template<> complex multiply<complex>(complex a, complex b) { return { a.r*b.r-a.i*b.i, a.r*b.i+a.i*b.r }; }"
                               "template<typename T, typename U> T    add            (T   t, U     u) { return t + u; }\n"
                               "template<                      > int  add<int, short>(int t, short u) { return t - u; }\n";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(6, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found a map entry");

            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("input.cc", vec[0].TU, "should have gotten the TU name");

            {
                Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs found");
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct complex { // sizeof=16\n"
                                 "   double r;\n"
                                 "   double i;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                Assert::AreEqual(5, maps.functionMap.size(), "wrong number of functions found");
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template<> int __cdecl add<int, short>(int t, short u) { return t - u; }\n",                                             (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T, typename U> T __cdecl add(T t, U u) { return t + u; }\n",                                           (*it++).second[0].fullyQualified);
                Assert::AreEqual("[[maybe_unused]] void __cdecl foo(volatile int * i = nullptr) noexcept { (void)i; }\n",                                  (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> complex __cdecl multiply<complex>(complex a, complex b) { return {a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r}; }\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> T __cdecl multiply(T a, T b) { return a * b; }\n",                                                  (*it++).second[0].fullyQualified);
            }
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

            Assert::AreEqual(2, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(1, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");
                Assert::AreEqual("export void __cdecl Baz([[deprecated]] int x, int y [[maybe_unused]]) {}\n",    (*it++).second[0].fullyQualified, "should have gotten the function and body");
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
                                "   explicit void __cdecl Foo(int i) : x(7), i(i) { this->i++; }\n"
                                "   [[nodiscard]] const Bar * __cdecl GetBar() const & override { return this; }\n"
                                "   (lambda at input.cc:2:333) __cdecl make_lambda() const {\n"
                                "       return [this](int x) {\n"
                                "           return x + this->i;\n"
                                "       };\n"
                                "   }\n"
                                "   template<typename T> T __cdecl doTemplateyStuff(const T & value) requires requires { typename T::value_type; } const { return value; }\n"
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

            Assert::AreEqual(5, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

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

            Assert::AreEqual(2, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(1, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("E",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("enum E { A=1, B, C };\n"                                     , (*it++).second[0].fullyQualified, "should have gotten the enum");
            }
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("N", it->first,               "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct N { // sizeof=4\n"
                                 "   enum N::(anonymous type at input.cc:3:12) { D=0, E, F };\n"
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

                Assert::AreEqual("using Color = enum (anonymous type at input.cc:2:9) { Red, Green, Blue }; // typedef enum (anonymous type at input.cc:2:9) { Red, Green, Blue } Color;\n"
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
                               "struct A { Alias member; Alias2 member2; \n"
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
                               "   int S::* pointertodatamember;"
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
                Assert::AreEqual("using (anonymous namespace)::Color2 = enum (anonymous type at input.cc:2:13) { Red, Green, Blue }; // typedef enum (anonymous type at input.cc:2:13) { Red, Green, Blue } (anonymous namespace)::Color2;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Alias = S; // typedef S Alias;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Alias2 = S; // typedef S Alias2;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<auto F, auto... Fs> using AllTrue = decltype((F() && ... && Fs())); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Color = enum (anonymous type at input.cc:2:13) { Red, Green, Blue }; // typedef enum (anonymous type at input.cc:2:13) { Red, Green, Blue } Color;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename C, typename R, typename... Args> using MemberFuncPtr = R (C::*)(Args...); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename R, typename... Args> using NoexceptFuncPtr = R (*)(Args...) noexcept; // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<template <template <typename T> class Inner, typename U> class Outer, template <typename X> class Wrap> using RecursiveAlias = typename Outer<Wrap, int>::type; // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<auto F, typename... Args> using ReturnTypeOf = decltype(F(Args{}...)); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename R, typename... Args> using TemplateUsingAliasToPointerToFunction = R (*)(Args...); // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using TypedefForPointerToFunction = int (*)(double, const char *); // typedef int (*)(double, const char *) TypedefForPointerToFunction;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using UsingAliasForPointerToFunction = int (*)(double, const char *); // typedef int (*)(double, const char *) UsingAliasForPointerToFunction;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename R, typename... Args> using VariadicFuncPtr = R (*)(Args..., ...); // no typedef equivalent\n"
                             , (*it++).second[0].fullyQualified);
            }
            {
                Assert::AreEqual(5, maps.udtMap.size(), "wrong number of UDTs in map");
                auto it = maps.udtMap.begin();

                Assert::AreEqual("struct A { // sizeof=104\n"
                                 "   S member;\n"
                                 "   S member2;\n"
                                 "   Color color;\n"
                                 "   enum (anonymous type at input.cc:2:13) { Red, Green, Blue } color2;\n"
                                 "   enum (anonymous namespace)::Color3 { Red, Green, Blue } color3;\n"
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
                                 "   int S::* pointertodatamember;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Inner {\n"
                                 "                        using Inner::type = T; // typedef T Inner::type;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<template <typename> class Inner, typename U> struct Outer {\n"
                                 "                                                         using Outer::type = typename Inner<U>::type; // typedef typename Inner<U>::type Outer::type;\n"
                                 "                                                      };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename X> struct Wrap {\n"
                                 "                        using Wrap::type = X; // typedef X Wrap::type;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                Assert::AreEqual(2, maps.enumMap.size(), "wrong number of enums in map");
                auto it = maps.enumMap.begin();

                Assert::AreEqual("enum (anonymous type at input.cc:2:13) { Red, Green, Blue };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum Color4 { Red, Green, Blue };\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"Testing ClassTemplateSpecialization and ClassTemplatePartialSpecialization", []
        {
            std::string code = "template<typename T> struct Box{}; struct S{}; struct A { Box<S> value; };\n"
                               "template<typename T, unsigned N> struct Array { T data[N];  T get(unsigned i) const { return data[i]; } void set(unsigned i, const T& value) { data[i] = value; } };\n"
                               "template<unsigned N> struct Array<bool, N> { unsigned char data[(N+7)/8]; bool get(unsigned i) const { return (data[i/8]>>(i%8))&1u;}\n"
                               "   void set(unsigned i, bool value) { unsigned char mask = static_cast<unsigned char>(1u<<(i%8)); if (value) data[i/8] |= mask; else data[i/8] &= static_cast<unsigned char>(~mask); } };\n"
                               "template<> struct Array<bool, 8> { unsigned char data; bool get(unsigned i) const { return (data >> i) & 1u; }\n"
                               "   void set(unsigned i, bool value) { unsigned char mask = static_cast<unsigned char>(1u << i);  if (value) data |= mask; else data &= static_cast<unsigned char>(~mask); } }; \n"
                               "template struct Array<int, 4>;\nextern template struct Array<double, 8>;\n"
                               "template<typename T> T identity(T value) { return value; }"
                               "template int identity<int>(int);"
                               "template<> bool identity<bool>(bool value) { return !value; }"
                               "int  a = identity<int>(42);"
                               "bool b = identity<bool>(true);"
                               "template<typename T> struct Wrapper      {   T value;            };"
                               "template<          > struct Wrapper<int> { int value; int extra; };"
                               "struct User { Wrapper<int> x; };"
                               ;
 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(9, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(2, maps.varMap.size(), "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(), "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(2, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct A { // sizeof=1\n"
                                 "   Box<S> value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T, unsigned int N> struct Array {\n"
                                 "                                        T data[N];\n"
                                 "                                        T __cdecl get(unsigned int i) const { return this->data[i]; }\n"
                                 "                                        void __cdecl set(unsigned int i, const T & value) { this->data[i] = value; }\n"
                                 "                                     };\n"
                              , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template<> struct Array<bool, 8> { // sizeof=1\n"
                                 "              unsigned char data;\n"
                                 "              bool __cdecl get(unsigned int i) const { return (this->data >> i) & 1U; }\n"
                                 "              void __cdecl set(unsigned int i, bool value) {\n"
                                 "                  unsigned char mask = static_cast<unsigned char>(1U << i);\n"
                                 "                  if (value)\n"
                                 "                      this->data |= mask;\n"
                                 "                  else\n"
                                 "                      this->data &= static_cast<unsigned char>(~mask);\n"
                                 "              }\n"
                                 "           };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <unsigned int N> struct Array<bool, N> {\n"
                                 "                             unsigned char data[(N + 7) / 8];\n"
                                 "                             bool __cdecl get(unsigned int i) const { return (this->data[i / 8] >> (i % 8)) & 1U; }\n"
                                 "                             void __cdecl set(unsigned int i, bool value) {\n"
                                 "                                 unsigned char mask = static_cast<unsigned char>(1U << (i % 8));\n"
                                 "                                 if (value)\n"
                                 "                                     this->data[i / 8] |= mask;\n"
                                 "                                 else\n"
                                 "                                     this->data[i / 8] &= static_cast<unsigned char>(~mask);\n"
                                 "                             }\n"
                                 "                          };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Box {\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct User { // sizeof=8\n"
                                 "   Wrapper<int> x;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Wrapper {\n"
                                 "                        T value;\n"
                                 "                     };\n"                    
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Wrapper<int> { // sizeof=8\n"
                                 "              int value;\n"
                                 "              int extra;\n"
                                 "           };\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("int a=identity<int>(42);\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("bool b=identity<bool>(true);\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template<> bool __cdecl identity<bool>(bool value) { return !value; }\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> T __cdecl identity(T value) { return value; }\n"     , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"Testing VarDecl, VarTemplateDecl, VarTemplateSpecializationDecl and VarTemplatePartialSpecializationDecl", []
        {
            std::string code = "template<typename T, int Tag=0> constexpr T DefaultValue = T{};\n"
                               "template<> constexpr int DefaultValue<int, 0> = 42;\n"
                               "template<typename T> constexpr T* DefaultValue<T*, 0> = nullptr;\n"
                               "template<int N> constexpr int Square = N*N;\n"
                               "template const int Square<5>; extern template const int Square<7>;\n"
                               "int    x = DefaultValue<int>;\n"
                               "double y = DefaultValue<double>;\n"
                               "int    z = Square<5>;\n"
                               "template<typename T> T GlobalValue={};\n"
                               "       template    int GlobalValue<int>;\n"
                               "extern template double GlobalValue<double>;\n"
                               "template<>        char GlobalValue<char> = 42;\n"
                               "            int    a = GlobalValue<int>;\n"
                               "            double b = GlobalValue<double>;\n"
                               "              char c = GlobalValue<char>;\n"
                               "template<typename T> constexpr bool IsPointerLike = false; template<typename T> constexpr bool IsPointerLike<T*> = true;\n"
                               ;
 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 0, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(14, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("template<typename T, int Tag=0> constexpr const T DefaultValue=T{};\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> constexpr T *const DefaultValue<T *, 0>=nullptr;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> constexpr const int DefaultValue<int, 0>=42;\n"              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> T GlobalValue{};\n"                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> char GlobalValue<char>=42;\n"                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> constexpr const bool IsPointerLike=false;\n"       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> constexpr const bool IsPointerLike<T *>=true;\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<int N> constexpr const int Square=N * N;\n"                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int a=GlobalValue<int>;\n"                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("double b=GlobalValue<double>;\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("char c=GlobalValue<char>;\n"                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int x=DefaultValue<int>;\n"                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("double y=DefaultValue<double>;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int z=Square<5>;\n"                                                     , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"2. Friend declarations inside UDTs and templates", []
        {
            std::string code = "struct A { friend void f(A&); };"
                               "struct B { friend void f(B&); }; void f(B&) {}"
                               "struct C { friend void f(C&) {} };"
                               "class D; struct E { friend class D; };"
                               "struct F; struct G { friend struct F; };"
                               "template<typename T> struct H { template<typename U> friend void f(U); };"
                               "template<typename T> void fi(T); struct I { friend void fi<int>(int); };"
                               "template<typename T> class J; struct K { template<typename T> friend class J; };"
                               "template<typename T> struct L { friend T; };"
                               "template<typename T> struct M { friend void fm(M); };"
                               "template<typename T> struct N { friend typename T::type; };"
                               "namespace { struct Hidden {}; } struct O { friend void fo(Hidden ); };"
                               "                                struct P { friend void fp(Hidden*); };"
                               "                                struct Q { friend void fq(Hidden&); };"
                               "                                struct R { friend void fr(Hidden[10]); };"
                               "                                struct S { friend void fs(Hidden(*)()); };"
                               "                                struct T { friend void ft(void(*)(Hidden,int)); };"
                               "                                struct U { friend void fu(void(*)(const Hidden&,int)); };"
                               "                                struct V { friend void fv(void(Hidden::*)(int), int); };";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(19, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(), "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(), "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual( 2, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct A { // sizeof=1\n"
                                 "   friend void __cdecl f(A &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct B { // sizeof=1\n"
                                 "   friend void __cdecl f(B &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct C { // sizeof=1\n"
                                 "   friend void __cdecl f(C &) {}\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct E { // sizeof=1\n"
                                 "   friend class D;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct G { // sizeof=1\n"
                                 "   friend struct F;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct H {\n"
                                 "                        template<typename U> friend void __cdecl f(U);\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct I { // sizeof=1\n"
                                 "   friend void __cdecl fi<int>(int);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct K { // sizeof=1\n"
                                 "   template<typename T> friend class J;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct L {\n"
                                 "                        friend T;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct M {\n"
                                 "                        friend void __cdecl fm(M<T>);\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct N {\n"
                                 "                        friend typename T::type;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct O { // sizeof=1\n"
                                 "   friend void __cdecl fo(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                          });\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct P { // sizeof=1\n"
                                 "   friend void __cdecl fp(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                          } *);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Q { // sizeof=1\n"
                                 "   friend void __cdecl fq(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                          } &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct R { // sizeof=1\n"
                                 "   friend void __cdecl fr(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                          }[10]);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S { // sizeof=1\n"
                                 "   friend void __cdecl fs(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                          } (*)());\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct T { // sizeof=1\n"
                                 "   friend void __cdecl ft(void (*)(struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                                   }, int));\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct U { // sizeof=1\n"
                                 "   friend void __cdecl fu(void (*)(const struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                                         } &, int));\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct V { // sizeof=1\n"
                                 "   friend void __cdecl fv(void (struct (anonymous namespace)::Hidden { // sizeof=1\n"
                                 "                                }::*)(int), int);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual(       "void __cdecl f(B &) {}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("friend void __cdecl f(C &) {}\n", (*it++).second[0].fullyQualified);
            }
        }
    },

    {"Anonymous namespace typedef/alias", []
        {
            std::string code = "namespace { struct AnonType {}; } using AT = AnonType; typedef AnonType TDA;\n"
                               "namespace { template<typename T> struct Invisible { using type = T*; }; } template<typename T> using Alias = Invisible<T>;\n" // Alias<int> p;\n"
                               "namespace { template<typename T> using Ptr = T*; }"
                               "template<typename T> struct S { template<typename U> using Ptr = U*; };"
                               "template<typename T> struct Outer { template<typename U> using Alias = U*; };"
                               "template<> struct Outer<int> { template<typename U> using Alias = U&; };"
                               "template<typename T> struct Outer<T*> { template<typename U> using Alias = U&; };"
                               "namespace { namespace Deeply { namespace Nested { struct Struct { enum Enum { Alpha, Beta }; }; }}}"
                               "typedef Deeply::Nested::Struct::Enum DeeplyNestedEnum;"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(4, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(), "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(), "wrong number of enums in map");
            Assert::AreEqual(4, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<typename T> struct Outer {\n"
                                 "                        template<typename U> using Alias = U *; // no typedef equivalent\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Outer<T *> {\n"
                                 "                         template<typename U> using Alias = U &; // no typedef equivalent\n"
                                 "                      };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Outer<int> { // sizeof=1\n"
                                 "              template<typename U> using Alias = U &; // no typedef equivalent\n"
                                 "           };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct S {\n"
                                 "                        template<typename U> using Ptr = U *; // no typedef equivalent\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using AT = struct (anonymous namespace)::AnonType { // sizeof=1\n"
                                 "           }; // typedef (anonymous namespace)::AnonType AT;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> using Alias = template<typename T> struct (anonymous namespace)::Invisible {\n"
                                 "                                                           using (anonymous namespace)::Invisible::type = T *; // typedef T * (anonymous namespace)::Invisible::type;\n"
                                 "                                                        }; // no typedef equivalent\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeeplyNestedEnum = enum (anonymous namespace)::Deeply::Nested::Struct::Enum { Alpha, Beta }; // typedef enum (anonymous namespace)::Deeply::Nested::Struct::Enum { Alpha, Beta } DeeplyNestedEnum;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using TDA = struct (anonymous namespace)::AnonType { // sizeof=1\n"
                                 "}; // typedef (anonymous namespace)::AnonType TDA;\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"Enums defined in an anonymous namespace or with no name", []
        {
            std::string code = "namespace { enum AnonColor { R=0, G, B }; }\n"  // in anonymous namespace
                               "enum {     R,   G=1+0, B };\n"                  // no name
                               "enum RGB { R=0, G,     B };\n"                  // simple case: uses print facility
                               "AnonColor ac;\n"                                // use anonymous namespace enum in a var
                               "typedef enum { R, G, B=3 } CStyleColor;\n"      // special C-like syntax handling
                               "template<typename T> struct A { enum { X=42 }; }; int x = A<int>::X;\n"
                               "namespace L { namespace M { namespace N { struct LNM { void f() { enum LMN1 { X = 42 }; } enum { X = 42 }; }; }}}\n"
                               "namespace { enum class Mode { A, B }; } template<Mode M> struct EnumHolder {};\n"
                               ;

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(2, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(3, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(1, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<typename T> struct A {\n"
                                 "                        enum A::(anonymous type at input.cc:6:33) { X=42 };\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<enum class (anonymous namespace)::Mode : int { A, B } M> struct EnumHolder {\n"
                                 "                                                                  };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct LNM { // sizeof=1\n"
                                 "   void __cdecl f() { enum LMN1 { X = 42 }; }\n"
                                 "   enum L::M::N::LNM::(anonymous type at input.cc:7:91) { X=42 };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (anonymous type at input.cc:2:1) { R, G=1, B };\n"      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous type at input.cc:5:9) { R, G, B=3 };\n"      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum RGB { R=0, G, B };\n"                                   , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("enum (anonymous namespace)::AnonColor { R=0, G, B } ac;\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int x=A<int>::X;\n"                                          , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using CStyleColor = enum (anonymous type at input.cc:5:9) { R, G, B=3 }; // typedef enum (anonymous type at input.cc:5:9) { R, G, B=3 } CStyleColor;\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"6. Out-of-class static data member definitions", []
        {
            std::string code = "struct Soo { static int counter; static const char* name; };\n"
                               "int         Soo::counter = 0;\n"
                               "const char* Soo::name = \"soo\";\n"
                               "namespace { struct Moo { static const int counter; }; } const int Moo::counter = 0;\n"
                               "struct Outer { struct Inner { static const int counter; }; }; const int Outer::Inner::counter = 0;\n"
                               ;

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(3, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Outer { // sizeof=1\n"
                                 "   struct Inner { // sizeof=1\n"
                                 "      static const int counter;\n"
                                 "   };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Soo { // sizeof=1\n"
                                 "   static int counter;\n"
                                 "   static const char *name;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("static const int Outer::Inner::counter=0;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("static int Soo::counter=0;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("static const char *Soo::name=\"soo\";\n"    , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"9. Defaulted & deleted functions and lambdas", []
        {
            std::string code = "struct DD { DD(const DD&) = delete; DD(DD&&) = default; ~DD() {} }; \n"
                               "auto g_lambda = []() {};\n"
                               "namespace { constexpr auto hidden_lambda = [](int x) { return x + 1; }; } auto global_lambda = hidden_lambda;\n"
                               "namespace { constexpr auto a = [](int x) { return x + 1; }; } auto b = a; auto c = b;\n"
                               "struct LambdaHolder { inline static auto LambdaField{[](int, double) {}}; };"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(4, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct DD { // sizeof=1\n"
                                 "   void __cdecl DD(const DD &) =delete;\n"
                                 "   constexpr void __cdecl DD(DD &&) =default;\n"
                                 "   void __cdecl ~DD() {}\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);

                Assert::AreEqual("struct LambdaHolder { // sizeof=1\n"
                                 "   inline static LambdaHolder::(lambda at input.cc:5:54) LambdaField{[](int, double){}};\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("(anonymous namespace)::(lambda at input.cc:4:32) b=[](int x){ return x + 1; };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(anonymous namespace)::(lambda at input.cc:4:32) c=[](int x){ return x + 1; };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:2:17) g_lambda=[](){};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(anonymous namespace)::(lambda at input.cc:3:44) global_lambda=[](int x){ return x + 1; };\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },

    {"3. Nested namespaces and qualified names and keys", []
        {
            std::string code = "namespace A {  namespace B { struct STwoDeep {};               namespace C { struct SThreeDeep {};                      namespace { struct SInvisible {};     } } }}\n"
                               "namespace A {  namespace B { void FTwoDeep() {}                namespace C { int FThreeDeep() { return 0; }                  static void FInvisible() {}        } }}\n"
                               "namespace A {  namespace B { enum ETwoDeep { One, Two=2 };     namespace C { enum EThreeDeep { One, Two, Three=3};      namespace { enum EInvisible { Zero }; } } }}\n"
                               "namespace A {  namespace B { typedef A::B::STwoDeep MyTwoDeep; namespace C { using MyThreeDeep = A::B::C::SThreeDeep; } typedef A::B::C::EInvisible MyInvisible;  }}\n"
                               "namespace A {  namespace B { int g_TwoDeep = 42; namespace C { static int s_EThreeDeep = -1; } }}\n"
                               "void* operator new(size_t size) { return (void*)7; }\n"
                               "template<typename T, int N, template<typename> class TT, typename... Args > void templateyFunction(T value, TT<T> tt, Args... args) { }\n"
                               "template<typename T> struct Wrapper {}; template<> void templateyFunction<int, 42, Wrapper, char, double>(int,Wrapper<int>,char,double) {}\n"
                               "void FooWithAnonArg(A::B::MyInvisible) {}"
                               "void VariadicFunction(int count, ...) {}"
                               "extern \"C\" void ExternC(int, char**) {}"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(1, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(2, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(3, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(8, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("A::B::C::SThreeDeep"                                             , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("struct SThreeDeep { // sizeof=1\n};\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::STwoDeep"                                                  , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("struct STwoDeep { // sizeof=1\n};\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Wrapper<>"                                                       , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template<typename T> struct Wrapper {\n                     };\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("A::B::C::FThreeDeep()"                                                                                                                           , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("int __cdecl FThreeDeep() { return 0; }\n"                                                                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::FTwoDeep()"                                                                                                                                , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void __cdecl FTwoDeep() {}\n"                                                                                                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooWithAnonArg(A::B::C::(anonymous namespace in input.cc)::EInvisible)"                                                                          , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void __cdecl FooWithAnonArg(A::B::MyInvisible) {}\n"                                                                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("VariadicFunction(int,...)"                                                                                                                       , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void __cdecl VariadicFunction(int count,...) {}\n"                                                                                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("operator new(unsigned long long)"                                                                                                                , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void * __cdecl operator new(size_t size) { return (void *)7; }\n"                                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("templateyFunction<int,42,Wrapper,<char, double>>(int,Wrapper<int>,char,double)"                                                                  , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template<> void __cdecl templateyFunction<int, 42, Wrapper, <char, double>>(int, Wrapper<int>, char, double) {}\n"                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("templateyFunction<typename T,int N,template <typename> class TT,typename ...Args>(T,TT<T>,Args...)"                                              , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template<typename T, int N, template <typename> class TT, typename ...Args> void __cdecl templateyFunction(T value, TT<T> tt, Args... args) {}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void ExternC(int, char **)"                                                                                                                      , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void ExternC(int, char **) {}\n"                                                                                                                 , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("A::B::C::MyThreeDeep"                                                                                                                                                        , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("using A::B::C::MyThreeDeep = A::B::C::SThreeDeep; // typedef A::B::C::SThreeDeep A::B::C::MyThreeDeep;\n"                                                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::MyInvisible"                                                                                                                                                           , (*it).first, "should have gotten proper key");
                Assert::AreEqual("using A::B::MyInvisible = enum A::B::C::(anonymous namespace)::EInvisible { Zero }; // typedef enum A::B::C::(anonymous namespace)::EInvisible { Zero } A::B::MyInvisible;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::MyTwoDeep"                                                                                                                                                             , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("using A::B::MyTwoDeep = A::B::STwoDeep; // typedef A::B::STwoDeep A::B::MyTwoDeep;\n"                                                                                        , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("A::B::C::EThreeDeep",                      (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("enum EThreeDeep { One, Two, Three=3 };\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::ETwoDeep",                           (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("enum ETwoDeep { One, Two=2 };\n",          (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("A::B::g_TwoDeep",     (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("int g_TwoDeep=42;\n", (*it++).second[0].fullyQualified);
            }
        }
    },

    {"4. Internal linkage declarations (static variables/functions, anonymous namespace variables/functions)", []
        {
            std::string code = "static void StaticFunction() {}\n"
                               "namespace { void AnonymousFunction() {} }\n"
                               "static int g_Static = 7;\n"
                               "namespace { int g_AnonymousNamespace = 6;\n }"
                               ;

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
        }
    },

    {"Template Specializations and Instantiations", []
        {
            std::string code = "template<class T> struct AWrapper { T value; }; namespace { struct Hidden {}; } using MyHiddenWrapper = AWrapper<Hidden>;\n"

                               "template<typename V> struct Wrapped { V value; };\n"
                               "template<template<typename> class TT, typename U> struct Outer1 { TT<U> member; };\n"
                               "Outer1<Wrapped,int> outer1Instance;\n"

                               "struct HasFoo { template<typename V> struct Foo { V value; }; };\n"
                               "template<typename T, typename U> struct Outer2 { typename T::template Foo<U> member; };\n"
                               "Outer2<HasFoo,int> outer2Instance;\n"

                               "template<typename T> struct Goo { int x; }; template   struct Goo<int>;\n"
                               "template<typename T> struct Hoo { int x; }; template<> struct Hoo<int> { int y; };\n"

                               "template<typename T> struct X {}; template<typename T> struct X<T*> { int i; };\n"

                               "template<typename T> inline int v = 0; template<typename T> inline int v<T*> = 1;\n"

                               "template<typename T> void Function(T) {} template<> void Function(int) {}\n"

                               "template<typename T> void ExplicitInstantiation(T) {} template void ExplicitInstantiation<int>(int);\n"

                               "template<typename T> using Ptr = T*;\n"

                               "enum class Color { Red, Green }; int global = 0; template<auto V> struct Holder {};"
                               "Holder<42> h1; Holder<true> h2; Holder<Color::Red> h3; Holder<&global> h4; Holder<nullptr> h5;Holder<&Function<int>> h6;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(11, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(11, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual( 1, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual( 2, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual( 3, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template<class T> struct AWrapper {\n"
                                 "                     T value;\n"
                                 "                  };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Goo {\n"
                                 "                        int x;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct HasFoo { // sizeof=1\n"
                                 "   template<typename V> struct Foo {\n"
                                 "                           V value;\n"
                                 "                        };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<auto V> struct Holder {\n"
                                 "                 };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct Hoo {\n"
                                 "                        int x;\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Hoo<int> { // sizeof=4\n"
                                 "              int y;\n"
                                 "           };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<template <typename> class TT, typename U> struct Outer1 {\n"
                                 "                                                      TT<U> member;\n"
                                 "                                                   };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T, typename U> struct Outer2 {\n"
                                 "                                    typename T::template Foo<U> member;\n"
                                 "                                 };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename V> struct Wrapped {\n"
                                 "                        V value;\n"
                                 "                     };\n"        
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> struct X {\n"
                                 "                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct X<T *> {\n"
                                 "                         int i;\n"
                                 "                      };\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using MyHiddenWrapper = AWrapper<(anonymous namespace)::Hidden>; // typedef AWrapper<(anonymous namespace)::Hidden> MyHiddenWrapper;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> using Ptr = T *; // no typedef equivalent\n"                                                                      , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum class Color : int { Red, Green };\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("int global=0;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<42> h1;\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<true> h2;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<Color::Red> h3;\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<&global> h4;\n"                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<nullptr> h5;\n"                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<&Function<int>> h6;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Outer1<Wrapped, int> outer1Instance;\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual( "Outer2<HasFoo, int> outer2Instance;\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> inline int v=0;\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> inline int v<T *>=1;\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template<typename T> void __cdecl ExplicitInstantiation(T) {}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> void __cdecl Function<int>(int) {}\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<typename T> void __cdecl Function(T) {}\n", (*it++).second[0].fullyQualified);
            }
        }
    },

    {"Lambdas", []
        {
            std::string code = "auto lambda1 = [](auto x) {};\n"
                               "auto lambda2 = []<typename T>(T t){};\n"
                               "auto lambda3 = []<typename T>(T) requires(sizeof(T)==4) {};\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(3, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("(lambda at input.cc:1:16) lambda1=[](auto x){};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:2:16) lambda2=[]<typename T>(T t){};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:3:16) lambda3=[]<typename T>(T) requires (sizeof(T) == 4) {};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },

    {"Function templates", []
        {
            std::string code = "void Abbreviated(auto x) {} template<typename T> void FunctionTemplate(T) {}"
                               "template<typename T> void HalfAbbreviated(T t, auto x) {}"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(),  "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),   "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),   "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(), "wrong number of typedefs in map");
            Assert::AreEqual(3, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("Abbreviated<auto>(auto)"                                            , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("void __cdecl Abbreviated(auto x) {}\n"                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FunctionTemplate<typename T>(T)"                                    , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template<typename T> void __cdecl FunctionTemplate(T) {}\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("HalfAbbreviated<typename T,auto>(T,auto)"                           , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template<typename T> void __cdecl HalfAbbreviated(T t, auto x) {}\n", (*it++).second[0].fullyQualified);

            }
        }
    },


};
/* some missing test cases

7. Concepts
            template<typename T> concept C = sizeof(T) == 4;
         and
            template<C T> void f(); 

9. requires
            requires(sizeof(T)==4)
        and
            requires C<T>

10. Constrained auto
            C auto x = ...

11. Conversion operators
            operator bool() const;

12. Literal operators
            operator ""_x

13. Three-way comparison
            auto operator<=>(...) = default;
        or
            struct Point
            {
                int x, y;
                int <=>(const Point& other) const
                {
                    if (x < other.x)
                        return -1;
                    if (x > other.x)
                        return 1;
                    return 0;
                }
            };

14. Deduction guides
            A(int)->A<int>;

15. using enum
            using enum Color; // it looks like these don't CAUSE ODR violations, though they might expose them

16. Pointer-to-member function
            int (A::*p)();

17. Multi-dimensional arrays
            int x[3][4];

18. References to arrays
            int (&r)[3];

19. Function references
            void (&f)();

20. Array of pointers to functions
            void Foo(int) {}
            void Bar(int) {}
            void (*const table[2])(int) =
            {
                &Foo,
                &Bar
            };

21. Empty base optimization
            struct B {};
            struct D : B {};

22. Multiple inheritance
            struct D : A, B {};

23. Virtual inheritance
            struct D : virtual B {};

24. Access-specifier changes
            private:
            protected:
            public:
        inside the same class.

25. Scoped enum with underlying type
            enum class E : unsigned short

///////////////////////////////////////////////////////////////////// variables 
27. Inline variables
            inline int x=0;

28. Thread local
            thread_local int x;

29. constinit
            constinit int x=0;

30. constexpr static data members

30.5. Attributes
            [[nodiscard]]
            [[maybe_unused]]
            [[no_unique_address]] // The last one is especially relevant.

/////////////////////////////////////////////////////////////////////// lambdas
/////////////////////////////////////////////////////////////////////////// friends
34. Friend function template
            template<typename T> friend void f(T);

35. Friend class template
            template<typename T> class Wrapper;
            class A { template<typename T> friend class Wrapper; ;
            template<typename T> class Wrapper { public: T value; };

/////////////////////////////////////////////////////////////////////////// namespace

36. Inline namespaces
            inline namespace v1
        Those affect qualification.

37. Namespace aliases
            namespace N = M;

38. Using declarations
            using Base::foo;

39. Exception specifications
            noexcept(false)
        and
            noexcept(sizeof(T)==4)

40. Modules
            export namespace
        or
            export using


*/