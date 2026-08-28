#include "stdafx.h"

import std;
import tdd20;
using namespace TDD20;

#include "..\src\ASTVisitor.h"

Test ExploratoryTestsOfClangAST[] =
{
    {"Initial Serialize::Decls and FunctionDeclSerializer test", []
        {
            std::string code =
                               "[[maybe_unused]] void foo(volatile int* i=nullptr) noexcept { (void)i; }\n"
                               "template<typename T> T multiply(T a, T b) { return a*b; }\n"
                               "struct complex { double r; double i; }; template<> complex multiply<complex>(complex a, complex b) { return { a.r*b.r-a.i*b.i, a.r*b.i+a.i*b.r }; }"
                               "template<typename T, typename U> T    add            (T   t, U     u) { return t + u; }\n"
                               "template<                      > int  add<int, short>(int t, short u) { return t - u; }\n"
                               "namespace { struct Anonymous {}; } [[maybe_unused]] Anonymous ReturnAnonymous() { return Anonymous{}; }\n"
                               "auto FunctionWithTrailingAnonymousNamespaceReturnType() noexcept -> Anonymous { return {}; }\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(7, maps.functionMap.size(), "wrong number of functions in map");

            const auto& vec = maps.functionMap.begin()->second;
            Assert::AreEqual("input.cc", vec[0].TU, "should have gotten the TU name");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct complex {\n"
                                 "    double r;\n"
                                 "    double i;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("auto FunctionWithTrailingAnonymousNamespaceReturnType() noexcept -> struct (anonymous namespace)::Anonymous {\n"
                                 "                                                                    } {\n"
                                 "    return {};\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("[[maybe_unused]] struct (anonymous namespace)::Anonymous {\n"
                                 "                 } ReturnAnonymous() {\n"
                                 "                     return Anonymous{};\n"
                                 "                 }\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> int add<int, short>(int t, short u) {\n"
                                 "    return t - u;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> T add(T t, U u) {\n"
                                 "    return t + u;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("[[maybe_unused]] void foo(volatile int *i = nullptr) noexcept {\n"
                                 "    (void)i;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> complex multiply<complex>(complex a, complex b) {\n"
                                 "    return {a.r * b.r - a.i * b.i, a.r * b.i + a.i * b.r};\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> T multiply(T a, T b) {\n"
                                 "    return a * b;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
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
                               "}; "
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(1, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");
                Assert::AreEqual("void Baz([[deprecated(\"\")]] int x, int y [[maybe_unused]]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the function and body");
            }
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Bar {\n"
                                 "    virtual const Bar *GetBar() const & = 0;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
                Assert::AreEqual("struct Foo : Bar {\n"
                                 "    struct {\n"
                                 "        int x;\n"
                                 "    };\n"
                                 "    int i;\n"
                                 "    explicit Foo(int i) : x(7), i(i) {\n"
                                 "        this->i++;\n"
                                 "    }\n"
                                 "    [[nodiscard(\"\")]] const Bar *GetBar() const & override {\n"
                                 "        return this;\n"
                                 "    }\n"
                                 "    (lambda at input.cc:2:333) make_lambda() const {\n"
                                 "        return [this](int x) {\n"
                                 "            return x + this->i;\n"
                                 "        };\n"
                                 "    }\n"
                                 "    template <typename T> T doTemplateyStuff(const T &value) const requires requires { typename T::value_type; } {\n"
                                 "        return value;\n"
                                 "    }\n"
                                 "    explicit operator int() const {\n"
                                 "        return 7;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the struct");
            }
        }
    },
    {"Testing CXXRecordDeclSerializer on UDTs", []
        {
            std::string code = "struct Qux {}; struct Bar {}; struct Baz{}; struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : Baz, virtual private Bar, protected Qux {"
                               "public: [[deprecated(\"use y instead\")]] constexpr static int x = 0; "
                               "   inline static int y{0};" // Decl::print() improperly drops the "inline"; my serializer is more correct than print()
                               "   int b:3=1;"
                               "   unsigned int c:2{3};"
                               "};"
                               "struct S {"
                               "    union { int a; double b; } u;" // Decl::print() doesn't handle this well either: it combines the two records, but doesn't inline the anonymous union type
                               "};"
                               "struct A { struct B { struct C { union { int x; }; }; }; };"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(6, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            auto it = maps.udtMap.begin();
            Assert::AreEqual("A",        it->first,        "should have gotten correct key");
            Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

            Assert::AreEqual("struct A {\n"
                             "    struct B {\n"
                             "        struct C {\n"
                             "            union {\n"
                             "                int x;\n"
                             "            };\n"
                             "        };\n"
                             "    };\n"
                             "};\n"
                          , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Bar {\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Baz {\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : Baz, virtual private Bar, protected Qux {\n"
                             "public:\n"
                             "    [[deprecated(\"use y instead\")]] static constexpr int x = 0;\n"
                             "    static inline int y{0};\n" // Decl::print() drops the "inline"; my serializer is does not
                             "    int b : 3 = 1;\n"
                             "    unsigned int c : 2 {3};\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Qux {\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct S {\n"
                           "    union (unnamed union at input.cc:1:297) {\n" // my serializer is considerably better than Decl::print() in this case
                           "        int a;\n"
                           "        double b;\n"
                           "    } u;\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
        }
    },

    {"Testing EnumDecl serialization", []
        {
            std::string code = "enum E { A=1, B, C };\n"
                               "struct S { E e=E::B; };\n"
                               "struct N {"
                               "    enum { D=0, E, F } eVal=E;" // EnumDecl::print() doesn't inline; not sufficient for ODR detection
                               "};"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("E",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("enum E {\n" 
                                 "    A = 1,\n" 
                                 "    B,\n" 
                                 "    C\n" 
                                 "};\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the enum");
            }
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("N", it->first,               "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct N {\n"
                                 "    enum (unnamed enum at input.cc:3:15) {\n"
                                 "        D = 0,\n"
                                 "        E,\n"
                                 "        F\n"
                                 "    } eVal = E;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
                Assert::AreEqual("struct S {\n"
                                 "    E e = E::B;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the struct");
            }
        }
    },

    {"Testing Typedef serialization", []
        {
            std::string code = "typedef int MyInt;\n"
                               "typedef enum { Red, Green, Blue } Color;\n" // my serialization is WAY better than EnumDecl::print()'s.
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(2, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:2:9) {\n" // my serialization is WAY better than EnumDecl::print()'s.
                                 "    Red,\n"                              // my serialization is WAY better than EnumDecl::print()'s.
                                 "    Green,\n"                            // my serialization is WAY better than EnumDecl::print()'s.
                                 "    Blue\n"                              // my serialization is WAY better than EnumDecl::print()'s.
                                 "};\n"                                    // my serialization is WAY better than EnumDecl::print()'s.
                              , (*it++).second[0].fullyQualified, "should have gotten the typedef");
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef enum (unnamed enum at input.cc:2:9) {\n"
                                 "            Red,\n"
                                 "            Green,\n"
                                 "            Blue\n"
                                 "        } Color;\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the typedef");
                Assert::AreEqual("typedef int MyInt;\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the typedef");
            }
        }
    },

    {"Testing SerializeTypes and SerializeTypeRecord", []
        {
            std::string code = "namespace { struct Foo { [[maybe_unused]] Foo* foo; }; } struct Bar : Foo {};"
                               "struct S {}; struct A { S* p; const S& q; S r[2]; const Foo* a; Foo& b; Foo c[2]; Foo**& d; void (*callback)(S*); void (*callback2)(Foo*, int, double);"
                               "Foo (S::* mp)(double, const char*) = nullptr;"
                               "int (Foo::*mp2)(int); };"
                               ; 
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(3, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("A",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct A {\n"
                                 "    S *p;\n"
                                 "    const S &q;\n"
                                 "    S r[2];\n"
                                 "    const struct (anonymous namespace)::Foo {\n"
                                 "              [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "          } *a;\n"
                                 "    struct (anonymous namespace)::Foo {\n"
                                 "        [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "    } &b;\n"
                                 "    struct (anonymous namespace)::Foo {\n"
                                 "        [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "    } c[2];\n"
                                 "    struct (anonymous namespace)::Foo {\n"
                                 "        [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "    } **&d;\n"
                                 "    void (*callback)(S *);\n"
                                 "    void (*callback2)(struct (anonymous namespace)::Foo {\n"
                                 "                          [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "                      } *, int, double);\n"
                                 "    struct (anonymous namespace)::Foo {\n"
                                 "        [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "    } (S::*mp)(double, const char *) = nullptr;\n"
                                 "    int (struct (anonymous namespace)::Foo {\n"
                                 "             [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "         }::*mp2)(int);\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the class");
                Assert::AreEqual("struct Bar : struct (anonymous namespace)::Foo {\n"
                                 "                 [[maybe_unused]] struct (anonymous namespace)::Foo *foo;\n"
                                 "             } {\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the anonymous namespace base class");
                Assert::AreEqual("struct S {\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified, "should have gotten the class");
            }
        }
    },
    {"Testing TypedefDecl and TypeAliasDecl", []
        {
            std::string code =
                               "struct S {}; using Alias = S; typedef S Alias2;\n"
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
                               "};"
                              ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 5, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 2, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(12, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("input.cc",                                                    it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("using Alias = S;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef S Alias2;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <auto F, auto ...Fs> using AllTrue = decltype((F() && ... && Fs()));\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (unnamed enum at input.cc:2:13) {\n"
                                 "            Red,\n"
                                 "            Green,\n"
                                 "            Blue\n"
                                 "        } Color;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename C, typename R, typename ...Args> using MemberFuncPtr = R (C::*)(Args...);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename ...Args> using NoexceptFuncPtr = R (*)(Args...) noexcept;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <template <template <typename T> class Inner, typename U> class Outer, template <typename X> class Wrap> using RecursiveAlias = typename Outer<Wrap, int>::type;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <auto F, typename ...Args> using ReturnTypeOf = decltype(F(Args{}...));\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename ...Args> using TemplateUsingAliasToPointerToFunction = R (*)(Args...);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef int (*TypedefForPointerToFunction)(double, const char *);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using UsingAliasForPointerToFunction = int (*)(double, const char *);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename R, typename ...Args> using VariadicFuncPtr = R (*)(Args..., ...);\n"
                             , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.udtMap.begin();

                Assert::AreEqual("struct A {\n"
                                 "    Alias member;\n"
                                 "    Alias2 member2;\n"
                                 "    enum (unnamed enum at input.cc:2:13) {\n"
                                 "        Red,\n"
                                 "        Green,\n"
                                 "        Blue\n"
                                 "    } color;\n"
                                 "    enum (unnamed enum at input.cc:2:13) {\n"
                                 "        Red,\n"
                                 "        Green,\n"
                                 "        Blue\n"
                                 "    } color2;\n"
                                 "    enum (anonymous namespace)::Color3 {\n"
                                 "        Red,\n"
                                 "        Green,\n"
                                 "        Blue\n"
                                 "    } color3;\n"
                                 "    Color4 color4;\n"
                                 "    TypedefForPointerToFunction tdpfn1;\n"
                                 "    UsingAliasForPointerToFunction tdpfn2;\n"
                                 "    AnonNamespaceTypedefToFuncionPointer tdpfn3;\n"
                                 "    AnonNamespaceUsingAliasToFunctionPointer tdpfn4;\n"
                                 "    TemplateUsingAliasToPointerToFunction<void, double, const char *, int, TemplateUsingAliasToPointerToFunction<int, float, double, const char *>, int, int> tuapfn;\n"
                                 "    NoexceptFuncPtr<void, int, long, long long, float, double, long double> tuapfnne;\n"
                                 "    VariadicFuncPtr<int> tuapfnv;\n"
                                 "    MemberFuncPtr<S, int, const char *> tuapfm;\n"
                                 "    ReturnTypeOf<[](auto x, auto y) {\n"
                                 "        return x * y;\n"
                                 "    }, int, long> fieldDemoingReturnTypeOfNTTP;\n"
                                 "    AllTrue<[] {\n"
                                 "        return true;\n"
                                 "    }, [] {\n"
                                 "        return true;\n"
                                 "    }, [] {\n"
                                 "        return false;\n"
                                 "    }> allTrueField;\n"
                                 "    RecursiveAlias<Outer, Wrap> recursivelyDefinedField;\n"
                                 "    int S::*pointertodatamember;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Inner {\n"
                                 "    using type = T;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <template <typename> class Inner, typename U> struct Outer {\n"
                                 "    using type = typename Inner<U>::type;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename X> struct Wrap {\n"
                                 "    using type = X;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:2:13) {\n"
                                 "    Red,\n"
                                 "    Green,\n"
                                 "    Blue\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum Color4 {\n"
                                 "    Red,\n"
                                 "    Green,\n"
                                 "    Blue\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Testing ClassTemplateSpecialization and ClassTemplatePartialSpecialization", []
        {
            std::string code =
                               "template<typename T> struct Box{}; struct S{}; struct A { Box<S> value; };\n"
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
            Assert::AreEqual(2, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(2, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct A {\n"
                                 "    Box<S> value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, unsigned int N> struct Array {\n"
                                 "    T data[N];\n"
                                 "    T get(unsigned int i) const {\n"
                                 "        return this->data[i];\n"
                                 "    }\n"
                                 "    void set(unsigned int i, const T &value) {\n"
                                 "        this->data[i] = value;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Array<bool, 8> {\n"
                                 "    unsigned char data;\n"
                                 "    bool get(unsigned int i) const {\n"
                                 "        return (this->data >> i) & 1U;\n"
                                 "    }\n"
                                 "    void set(unsigned int i, bool value) {\n"
                                 "        unsigned char mask = static_cast<unsigned char>(1U << i);\n"
                                 "        if (value)\n"
                                 "            this->data |= mask;\n"
                                 "        else\n"
                                 "            this->data &= static_cast<unsigned char>(~mask);\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <unsigned int N> struct Array<bool, N> {\n"
                                 "    unsigned char data[(N + 7) / 8];\n"
                                 "    bool get(unsigned int i) const {\n"
                                 "        return (this->data[i / 8] >> (i % 8)) & 1U;\n"
                                 "    }\n"
                                 "    void set(unsigned int i, bool value) {\n"
                                 "        unsigned char mask = static_cast<unsigned char>(1U << (i % 8));\n"
                                 "        if (value)\n"
                                 "            this->data[i / 8] |= mask;\n"
                                 "        else\n"
                                 "            this->data[i / 8] &= static_cast<unsigned char>(~mask);\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Box {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct User {\n"
                                 "    Wrapper<int> x;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Wrapper {\n"
                                 "    T value;\n"
                                 "};\n"                    
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Wrapper<int> {\n"
                                 "    int value;\n"
                                 "    int extra;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("int a = identity<int>(42);\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("bool b = identity<bool>(true);\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template<> bool identity<bool>(bool value) {\n"
                                 "    return !value;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> T identity(T value) {\n"
                                 "    return value;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Testing VarDecl, VarTemplateDecl, VarTemplateSpecializationDecl and VarTemplatePartialSpecializationDecl", []
        {
            std::string code =
                               "template<typename T, int Tag=0> constexpr T DefaultValue = T{};\n"
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

            Assert::AreEqual( 0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(14, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("template <typename T, int Tag = 0> constexpr T DefaultValue = T{};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> constexpr T *DefaultValue<T *, 0> = nullptr;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> constexpr int DefaultValue<int, 0> = 42;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> T GlobalValue = {};\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> char GlobalValue<char> = 42;\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> constexpr bool IsPointerLike = false;\n"       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> constexpr bool IsPointerLike<T *> = true;\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <int N> constexpr int Square = N * N;\n"                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int a = GlobalValue<int>;\n"                                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("double b = GlobalValue<double>;\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("char c = GlobalValue<char>;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int x = DefaultValue<int>;\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("double y = DefaultValue<double>;\n"                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int z = Square<5>;\n"                                                , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"2. Friend declarations inside UDTs and templates", []
        {
            std::string code =
                               "struct A { friend void f(A&); };"
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
                               "                                struct V { friend void fv(void(Hidden::*)(int), int); };"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(19, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual( 2, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct A {\n"
                                 "    friend void f(A &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct B {\n"
                                 "    friend void f(B &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct C {\n"
                                 "    friend void f(C &) {\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct E {\n"
                                 "    friend class D;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct G {\n"
                                 "    friend struct F;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct H {\n"
                                 "    template <typename U> friend void f(U);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct I {\n"
                                 "    friend void fi<int>(int);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct K {\n"
                                 "    template <typename T> friend class J;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct L {\n"
                                 "    friend T;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct M {\n"
                                 "    friend void fm(M<T>);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct N {\n"
                                 "    friend typename T::type;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct O {\n"
                                 "    friend void fo(struct (anonymous namespace)::Hidden {\n"
                                 "                   });\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct P {\n"
                                 "    friend void fp(struct (anonymous namespace)::Hidden {\n"
                                 "                   } *);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Q {\n"
                                 "    friend void fq(struct (anonymous namespace)::Hidden {\n"
                                 "                   } &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct R {\n"
                                 "    friend void fr(struct (anonymous namespace)::Hidden {\n"
                                 "                   } [10]);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct S {\n"
                                 "    friend void fs(struct (anonymous namespace)::Hidden {\n"
                                 "                   } (*)());\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct T {\n"
                                 "    friend void ft(void (*)(struct (anonymous namespace)::Hidden {\n"
                                 "                            }, int));\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct U {\n"
                                 "    friend void fu(void (*)(const struct (anonymous namespace)::Hidden {\n"
                                 "                                  } &, int));\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct V {\n"
                                 "    friend void fv(void (struct (anonymous namespace)::Hidden {\n"
                                 "                         }::*)(int), int);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void f(B &) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void f(C &) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Anonymous namespace typedef/alias", []
        {
            std::string code =
                               "namespace { struct AnonType {}; } using AT = AnonType; typedef AnonType TDA;\n"
                               "namespace { template<typename T> struct Invisible { using type = T*; }; } template<typename T> using Alias = Invisible<T>;\n"
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
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template <typename T> struct Outer {\n"
                                 "    template <typename U> using Alias = U *;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Outer<T *> {\n"
                                 "    template <typename U> using Alias = U &;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Outer<int> {\n"
                                 "    template <typename U> using Alias = U &;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct S {\n"
                                 "    template <typename U> using Ptr = U *;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using AT = struct (anonymous namespace)::AnonType {\n"
                                 "           };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using Alias = template <typename T> struct (anonymous namespace)::Invisible {\n"
                                 "                                        using type = T *;\n"
                                 "                                    };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::Deeply::Nested::Struct::Enum {\n"
                                 "            Alpha,\n"
                                 "            Beta\n"
                                 "        } DeeplyNestedEnum;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonType {\n"
                                 "        } TDA;\n"
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

            Assert::AreEqual(3, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(2, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(3, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(1, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template <typename T> struct A {\n"
                                 "    enum (unnamed enum at input.cc:6:33) {\n"
                                 "        X = 42\n"
                                 "    };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <enum class (anonymous namespace)::Mode {\n"
                                 "              A,\n"
                                 "              B\n"
                                 "          } M> struct EnumHolder {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct LNM {\n"
                                 "    void f() {\n"
                                 "        enum LMN1 {\n"
                                 "            X = 42\n"
                                 "        };\n"
                                 "    }\n"
                                 "    enum (unnamed enum at input.cc:7:91) {\n"
                                 "        X = 42\n"
                                 "    };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:2:1) {\n"
                                 "    R,\n"
                                 "    G = 1,\n"
                                 "    B\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:5:9) {\n"
                                 "    R,\n"
                                 "    G,\n"
                                 "    B = 3\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum RGB {\n"
                                 "    R = 0,\n"
                                 "    G,\n"
                                 "    B\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("enum (anonymous namespace)::AnonColor {\n"
                                 "    R = 0,\n"
                                 "    G,\n"
                                 "    B\n"
                                 "} ac;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int x = A<int>::X;\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef enum (unnamed enum at input.cc:5:9) {\n"
                                 "            R,\n"
                                 "            G,\n"
                                 "            B = 3\n"
                                 "        } CStyleColor;\n"
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

            Assert::AreEqual(2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(3, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Outer {\n"
                                 "    struct Inner {\n"
                                 "        static const int counter;\n"
                                 "    };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Soo {\n"
                                 "    static int counter;\n"
                                 "    static const char *name;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("const int Outer::Inner::counter = 0;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Soo::counter = 0;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("const char *Soo::name = \"soo\";\n"    , (*it++).second[0].fullyQualified);
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

            Assert::AreEqual(2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(4, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct DD {\n"
                                 "    DD(const DD &) = delete;\n"
                                 "    DD(DD &&) = default;\n"
                                 "    ~DD() noexcept {\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct LambdaHolder {\n"
                                 "    static inline LambdaHolder::(lambda at input.cc:5:54) LambdaField{[](int, double) {\n"
                                 "                                                                      }};\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("class (anonymous namespace)::(lambda at input.cc:4:32) {\n"
                                 "    inline constexpr int operator()(int x) const {\n"
                                 "        return x + 1;\n"
                                 "    }\n"
                                 "} b = [](int x) {\n"
                                 "          return x + 1;\n"
                                 "      };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("class (anonymous namespace)::(lambda at input.cc:4:32) {\n"
                                 "    inline constexpr int operator()(int x) const {\n"
                                 "        return x + 1;\n"
                                 "    }\n"
                                 "} c = [](int x) {\n"
                                 "          return x + 1;\n"
                                 "      };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:2:17) g_lambda = []() {\n"
                                 "                                     };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("class (anonymous namespace)::(lambda at input.cc:3:44) {\n"
                                 "    inline constexpr int operator()(int x) const {\n"
                                 "        return x + 1;\n"
                                 "    }\n"
                                 "} global_lambda = [](int x) {\n"
                                 "                      return x + 1;\n"
                                 "                  };\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"3. Nested namespaces and qualified names and keys", []
        {
            std::string code = 
                               "namespace A {  namespace B { struct STwoDeep {};               namespace C { struct SThreeDeep {};                      namespace { struct SInvisible {};     } } }}\n"
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

            Assert::AreEqual(3, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(1, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(2, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(3, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(8, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("A::B::C::SThreeDeep"                         , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("struct SThreeDeep {\n};\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::STwoDeep"                              , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("struct STwoDeep {\n};\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Wrapper<>"                                   , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template <typename T> struct Wrapper {\n};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("A::B::C::FThreeDeep()"                                                                                      , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("int FThreeDeep() {\n    return 0;\n}\n"                                                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::FTwoDeep()"                                                                                           , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void FTwoDeep() {\n}\n"                                                                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooWithAnonArg(A::B::C::(anonymous namespace in input.cc)::EInvisible)"                                     , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void FooWithAnonArg(enum A::B::C::(anonymous namespace)::EInvisible {\n"
                                 "                        Zero\n"
                                 "                    }) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("VariadicFunction(int,...)"                                                                                  , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void VariadicFunction(int count, ...) {\n}\n"                                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("operator new(unsigned long long)"                                                                           , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void *operator new(size_t size) {\n    return (void *)7;\n}\n"                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("templateyFunction<int,42,Wrapper,<char, double>>(int,Wrapper<int>,char,double)"                             , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template<> void templateyFunction<int, 42, Wrapper, char, double>(int, Wrapper<int>, char, double) {\n"
                                 "}\n"                                                                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("templateyFunction<typename T,int N,template <typename> class TT,typename ...Args>(T,TT<T>,Args...)"         , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("template <typename T, int N, template <typename> class TT, typename ...Args> void templateyFunction(T value, TT<T> tt, Args ...args) {\n"
                                 "}\n"                                                                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void ExternC(int, char **)"                                                                                 , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("void ExternC(int, char **) {\n}\n"                                                                          , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("A::B::C::MyThreeDeep"                                           , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("using MyThreeDeep = A::B::C::SThreeDeep;\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::MyInvisible"                                              , (*it).first, "should have gotten proper key");
                Assert::AreEqual("typedef enum A::B::C::(anonymous namespace)::EInvisible {\n"
                                 "            Zero\n"
                                 "        } MyInvisible;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::MyTwoDeep"                                                , (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("typedef A::B::STwoDeep MyTwoDeep;\n"                            , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("A::B::C::EThreeDeep",                                        (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("enum EThreeDeep {\n    One,\n    Two,\n    Three = 3\n};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("A::B::ETwoDeep",                                             (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("enum ETwoDeep {\n    One,\n    Two = 2\n};\n",               (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("A::B::g_TwoDeep",       (*it  ).first, "should have gotten proper key");
                Assert::AreEqual("int g_TwoDeep = 42;\n", (*it++).second[0].fullyQualified);
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

            Assert::AreEqual(0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
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

            Assert::AreEqual(11, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(11, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 2, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual( 3, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("template <class T> struct AWrapper {\n"
                                 "    T value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Goo {\n"
                                 "    int x;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct HasFoo {\n"
                                 "    template <typename V> struct Foo {\n"
                                 "        V value;\n"
                                 "    };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <auto V> struct Holder {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct Hoo {\n"
                                 "    int x;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct Hoo<int> {\n"
                                 "    int y;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <template <typename> class TT, typename U> struct Outer1 {\n"
                                 "    TT<U> member;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> struct Outer2 {\n"
                                 "    typename T::template Foo<U> member;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename V> struct Wrapped {\n"
                                 "    V value;\n"
                                 "};\n"        
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct X {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct X<T *> {\n"
                                 "    int i;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using MyHiddenWrapper = AWrapper<struct (anonymous namespace)::Hidden {\n"
                                 "                                 }>;\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using Ptr = T *;\n"  , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum class Color {\n    Red,\n    Green\n};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("int global = 0;\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<42> h1;\n"                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<true> h2;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<Color::Red> h3;\n"                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<&global> h4;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<nullptr> h5;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Holder<&Function<int>> h6;\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Outer1<Wrapped, int> outer1Instance;\n"        , (*it++).second[0].fullyQualified);
                Assert::AreEqual( "Outer2<HasFoo, int> outer2Instance;\n"        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> inline int v = 0;\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> inline int v<T *> = 1;\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template <typename T> void ExplicitInstantiation(T) {\n}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> void Function<int>(int) {\n}\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> void Function(T) {\n}\n"             , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Lambdas", []
        {
            std::string code = "auto lambda1 = [](auto x) {};\n"
                               "auto lambda2 = []<typename T>(T t){};\n"
                               "auto lambda3 = []<typename T>(T) requires(sizeof(T)==4) {};\n"
                               "namespace { auto Lambda = [](int x) { return x * 2; }; } struct StructWithLambdaField { decltype(Lambda) mpf = Lambda; };\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(3, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct StructWithLambdaField {\n"
                                 "    class (anonymous namespace)::(lambda at input.cc:4:27) {\n"
                                 "        inline constexpr int operator()(int x) const {\n"
                                 "            return x * 2;\n"
                                 "        }\n"
                                 "    } mpf = (anonymous namespace)::Lambda;\n"
                                 "};\n"
                               , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("(lambda at input.cc:1:16) lambda1 = [](auto x) {\n"
                                 "                                    };\n"                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:2:16) lambda2 = []<typename T>(T t) {\n"
                                 "                                    };\n"                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("(lambda at input.cc:3:16) lambda3 = []<typename T>(T) requires (sizeof(T) == 4) {\n"
                                 "                                    };\n"                                            , (*it++).second[0].fullyQualified);
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

            Assert::AreEqual(0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
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
                Assert::AreEqual("Abbreviated<auto>(auto)"                                       , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("void Abbreviated(auto x) {\n}\n"                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FunctionTemplate<typename T>(T)"                               , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> void FunctionTemplate(T) {\n}\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("HalfAbbreviated<typename T,auto>(T,auto)"                      , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> void HalfAbbreviated(T t, auto x) {\n}\n", (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Class templates", []
        {
            std::string code = "namespace { struct Structural { int value; }; }\n"
                               "template<Structural S> struct Foo {}; Foo<Structural{42}> foo;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(1, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("Foo<>", (*it).first, "should have gotten correct key");
                Assert::AreEqual("template <struct (anonymous namespace)::Structural {\n"
                                 "              int value;\n"
                                 "          } S> struct Foo {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("foo", (*it).first, "should have gotten correct key");
                Assert::AreEqual("Foo<struct (anonymous namespace)::Structural {\n"
                                 "        int value;\n"
                                 "    }{42}> foo;\n"
                              , (*it++).second[0].fullyQualified);
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

    {"Requires", []
        {
            std::string code = "template <typename T> T FunctionTemplateWithRequiresClause(const T& value) requires requires { typename T::value_type; } { return value; }\n"
                               "template<typename T> concept TheConcept = sizeof(T) == 4;"
                               "template<typename T> requires TheConcept<T> void FunctionTemplateWithConcept(T) {}\n"

                               "namespace { struct NoSeeUm {}; }\n"
                               "template<typename T, typename U> concept IsSameConcept = true;\n"
                               "template<typename T> requires IsSameConcept<T, NoSeeUm> void FunctionTemplateWithAnonymousConcept(T) {}\n"

                               "template<typename T> requires TheConcept<T> struct ClassRequiresTheConcept { T value; };\n"
                               "template<typename T> requires IsSameConcept<T, NoSeeUm> struct ClassTemplateWithAnonymousConcept { T value; };\n"

                               "template<auto V> concept ValueConcept = true;\n"
                                   "struct ValueHolder { static int value; };\n"
                                   "template<typename T> requires ValueConcept<&ValueHolder::value> struct TestDeclarationArg {};\n"

                                   "namespace { struct Hidden { static const int value = 0; }; }\n"
                                   "template<typename T> requires ValueConcept<&Hidden::value> struct TestAnonymousNamespaceArg {};\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(5, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(3, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(3, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("ClassRequiresTheConcept<>"                                                       , (*it).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> requires TheConcept<T> struct ClassRequiresTheConcept {\n"
                                 "    T value;\n"
                                 "};\n"                                                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ClassTemplateWithAnonymousConcept<>"                                             , (*it).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> requires IsSameConcept<T, struct (anonymous namespace)::NoSeeUm {\n"
                                 "                                                }> struct ClassTemplateWithAnonymousConcept {\n"
                                 "    T value;\n"
                                 "};\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires ValueConcept<&(struct (anonymous namespace)::Hidden {\n"
                                 "                                                  static const int value = 0;\n"
                                 "                                              })::value> struct TestAnonymousNamespaceArg {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires ValueConcept<&ValueHolder::value> struct TestDeclarationArg {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ValueHolder {\n"
                                 "    static int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
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
                Assert::AreEqual("FunctionTemplateWithAnonymousConcept<typename T>(T)"                                                                                     , (*it).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> requires IsSameConcept<T, struct (anonymous namespace)::NoSeeUm {\n"
                                 "                                                }> void FunctionTemplateWithAnonymousConcept(T) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FunctionTemplateWithConcept<typename T>(T)"                                                                                              , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> requires TheConcept<T> void FunctionTemplateWithConcept(T) {\n"
                                 "}\n"                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FunctionTemplateWithRequiresClause<typename T>(const T &)"                                                                               , (*it  ).first, "should have gotten correct key");
                Assert::AreEqual("template <typename T> T FunctionTemplateWithRequiresClause(const T &value) requires requires { typename T::value_type; } {\n"
                                 "    return value;\n"
                                 "}\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
                Assert::AreEqual("IsSameConcept"                                                    , (*it  ).first,                    "should have gotten the correct key");
                Assert::AreEqual("template <typename T, typename U> concept IsSameConcept = true;\n", (*it++).second[0].fullyQualified, "should have gotten the correct concept");
                Assert::AreEqual("TheConcept"                                                       , (*it  ).first,                    "should have gotten the correct key");
                Assert::AreEqual("template <typename T> concept TheConcept = sizeof(T) == 4;\n"     , (*it++).second[0].fullyQualified, "should have gotten the correct concept");
                Assert::AreEqual("ValueConcept"                                                     , (*it  ).first,                    "should have gotten the correct key");
                Assert::AreEqual("template <auto V> concept ValueConcept = true;\n"                 , (*it++).second[0].fullyQualified, "should have gotten the correct concept");
            }
        }
    },
    {"Conversion operators", []
        {
            std::string code =
                               "struct ConversionOperatorClass_Declaration { operator int() const; };\n"
                               "struct ConversionOperatorClass_Definition  { operator int() { return -7;} };\n"
                               "struct ConversionOperatorClass_Template { template<class T> operator T() const { return T{}; } };\n"
                               "int i= ConversionOperatorClass_Template{}; double d = ConversionOperatorClass_Template{};\n"
                               "namespace { struct Hidden { }; } struct ConversionOperatorClass_AnonymousReturnType { operator Hidden() const; };\n"
                               "struct UsingHiddenInExplicitExpression { template<class T> explicit(sizeof(Hidden) == sizeof(T)) operator T() const; };\n"

                               "struct ConversionOperatorClass_Explicit          { explicit operator bool() const; };\n"
                               "struct ConversionOperatorClass_ExplicitCondition { explicit(true) operator bool() const; };\n"
                               "struct ConversionOperatorClass_Noexcept          { operator int() const noexcept; };\n"
                               "struct ConversionOperatorClass_RefQualified      { operator int() &; operator double() &&; };\n"
                               "struct ConversionOperatorClass_ConstVolatile     { operator int() const volatile; };\n"
                               "struct ConversionOperatorClass_Constexpr         { constexpr operator int() const { return 7; } };\n"
                               "struct ConversionOperatorClass_Deleted           { operator int() const = delete; };\n"

                               "namespace { struct HiddenPointer   { }; } struct ConversionOperatorClass_AnonymousPointer   { operator       HiddenPointer  *() const; };\n"
                               "namespace { struct HiddenReference { }; } struct ConversionOperatorClass_AnonymousReference { operator const HiddenReference&() const; };\n"

                               "template<class T> struct Wrapper { }; namespace { struct HiddenTemplateArgument { }; } struct ConversionOperatorClass_AnonymousTemplateArgument { operator Wrapper<HiddenTemplateArgument>() const; };\n"

                               "struct ConversionOperatorClass_ConstrainedTemplate { template<class T> requires (sizeof(T) > 1) operator T() const { return T{}; } };\n"

                               "struct ConversionOperatorClass_Nodiscard { [[nodiscard]] operator int() const { return 7; } };\n"

                               "using FunctionPointer = int (*)(); struct ConversionOperatorClass_FunctionPointer { operator FunctionPointer() const; };\n"
                               "namespace { struct HiddenFunctionReturn {}; using HiddenFunctionPointer = HiddenFunctionReturn(*)(); } struct ConversionOperatorClass_AnonymousFunctionPointer { operator HiddenFunctionPointer() const; };\n"
                               "struct ConversionOperatorClass_MemberPointer { operator int ConversionOperatorClass_MemberPointer::*() const; };\n"
                               "using IntArrayReference = int(&)[3]; struct ConversionOperatorClass_ArrayReference { operator IntArrayReference() const; };\n"
                               "using FunctionReference = int(&)(); struct ConversionOperatorClass_FunctionReference { operator FunctionReference() const; };\n"

                               "namespace { struct HiddenConstraintType {}; } struct ConversionOperatorClass_AnonymousConstraint { template<class T> requires (sizeof(HiddenConstraintType) == sizeof(T)) operator T() const; };\n"

                               "struct ConversionOperatorClass_TemplateExplicit { template<class T> explicit(sizeof(T) > 4) operator T() const { return T{}; } };\n"
                               "struct ConversionOperatorClass_ConditionalNoexcept { template<class T> operator T() const noexcept(sizeof(T) <= sizeof(int)) { return T{}; } };\n"
                               "namespace { struct HiddenNoexceptType {}; } template<class T> struct ConversionOperatorClass_AnonymousNoexceptExpression { operator int() const noexcept(sizeof(HiddenNoexceptType) == sizeof(T)); };\n"
                               "struct ConversionOperatorClass_CvRefQualified { operator int() const&; operator double() volatile&&; };\n"
                               "struct ConversionOperatorClass_Consteval { consteval operator int() const { return 7; } };\n"
                               "struct ConversionOperatorClass_Deprecated { [[deprecated(\"use something else\")]] operator int() const; };\n"
                               "struct ConversionOperatorClass_Virtual { virtual operator int() const; };\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(31, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 2, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 3, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousConstraint {\n"
                                 "    template <class T> requires (sizeof(struct (anonymous namespace)::HiddenConstraintType {\n"
                                 "                                        }) == sizeof(T)) operator T() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousFunctionPointer {\n"
                                 "    operator struct (anonymous namespace)::HiddenFunctionReturn {\n"
                                 "             } (*)()() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <class T> struct ConversionOperatorClass_AnonymousNoexceptExpression {\n"
                                 "    operator int() const noexcept(sizeof(struct (anonymous namespace)::HiddenNoexceptType {\n"
                                 "                                         }) == sizeof(T));\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousPointer {\n"
                                 "    operator struct (anonymous namespace)::HiddenPointer {\n"
                                 "             } *() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousReference {\n"
                                 "    operator const struct (anonymous namespace)::HiddenReference {\n"
                                 "                   } &() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousReturnType {\n"
                                 "    operator struct (anonymous namespace)::Hidden {\n"
                                 "             }() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_AnonymousTemplateArgument {\n"
                                 "    operator Wrapper<struct (anonymous namespace)::HiddenTemplateArgument {\n"
                                 "                     }>() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_ArrayReference {\n"
                                "    operator IntArrayReference() const;\n"
                                "};\n"
                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_ConditionalNoexcept {\n"
                                 "    template <class T> operator T() const noexcept(sizeof(T) <= sizeof(int)) {\n"
                                 "        return T{};\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_ConstVolatile {\n"
                                 "    operator int() const volatile;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Consteval {\n"
                                 "    consteval operator int() const {\n"
                                 "        return 7;\n"
                                 "    }\n"
                                 "};\n"              
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Constexpr {\n"
                                 "    constexpr operator int() const {\n"
                                 "        return 7;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_ConstrainedTemplate {\n"
                                 "    template <class T> requires (sizeof(T) > 1) operator T() const {\n"
                                 "        return T{};\n"
                                 "    }\n"
                                 "};\n"                    
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_CvRefQualified {\n"
                                 "    operator int() const &;\n"
                                 "    operator double() volatile &&;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Declaration {\n"
                                 "    operator int() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Definition {\n"
                                 "    operator int() {\n"
                                 "        return -7;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Deleted {\n"
                                 "    operator int() const = delete;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Deprecated {\n"
                                 "    [[deprecated(\"use something else\")]] operator int() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Explicit {\n"
                                 "    explicit operator bool() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_ExplicitCondition {\n"
                                 "    explicit(true) operator bool() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_FunctionPointer {\n"
                                 "    operator FunctionPointer() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_FunctionReference {\n"
                                 "    operator FunctionReference() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_MemberPointer {\n"
                                 "    operator int ConversionOperatorClass_MemberPointer::*() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Nodiscard {\n"
                                 "    [[nodiscard(\"\")]] operator int() const {\n"
                                 "        return 7;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Noexcept {\n"
                                 "    operator int() const noexcept;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_RefQualified {\n"
                                 "    operator int() &;\n"
                                 "    operator double() &&;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Template {\n"
                                 "    template <class T> operator T() const {\n"
                                 "        return T{};\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_TemplateExplicit {\n"
                                 "    template <class T> explicit(sizeof(T) > 4) operator T() const {\n"
                                 "        return T{};\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConversionOperatorClass_Virtual {\n"
                                 "    virtual operator int() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct UsingHiddenInExplicitExpression {\n"
                                 "    template <class T> explicit(sizeof(struct (anonymous namespace)::Hidden {\n"
                                 "                                       }) == sizeof(T)) operator T() const;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <class T> struct Wrapper {\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("double d = ConversionOperatorClass_Template{};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual(   "int i = ConversionOperatorClass_Template{};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using FunctionPointer = int (*)();\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using FunctionReference = int (&)();\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using IntArrayReference = int (&)[3];\n" , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
        }
    },
    {"Unnamed enum/class/struct/union tests", []
        {
            std::string code = 
// typedef/using alias around unnamed union/struct/class/enum
                               "struct TypedefAroundUnnamedUnion  { typedef union  { int i; double d; } MyTypedefedUnnamedUnion ; };\n"
                               "struct TypedefAroundUnnamedStruct { typedef struct { int i; double d; } MyTypedefedUnnamedStruct; };\n"
                               "struct TypedefAroundUnnamedClass  { typedef class  { int i; double d; } MyTypedefedUnnamedClass ; };\n"
                               "struct TypedefAroundUnnamedEnum   { typedef enum   { i=0,          d  } MyTypedefedUnnmaedEnum  ; };\n"
                               "struct UsingAroundUnnamedUnion    { using MyUsingAliasedUnnamedUnion  = union  { int i; double d; }; };\n"
                               "struct UsingAroundUnnamedStruct   { using MyUsingAliasedUnnamedStruct = struct { int i; double d; }; };\n"
                               "struct UsingAroundUnnamedClass    { using MyUsingAliasedUnnamedClass  = class  { int i; double d; }; };\n"
                               "struct UsingAroundUnnamedEnum     { using MyUsingAliasedUnnmaedEnum   = enum   { i = 0,        d  }; };\n"
// nested unnamed union/struct/class/enum with field
                               "struct NamelessEnumUsedAsAField   { enum   { A,  B,  C } field; };\n"
                               "struct NamelessStructUsedAsAField { struct { int x, y; } field; };\n"
                               "struct NamelessClassUsedAsAField  { class  { int x, y; } field; };\n"
                               "struct NamelessUnionUsedAsAField  { union  { int x, y; } field; };\n"
                               "struct StructContainingALargeIntegerField\n"
                               "{\n"
                               "    typedef union {\n"
                               "        struct { unsigned long  LowPart; signed long HighPart; } Parts;\n"
                               "        long long QuadPart;\n"
                               "    } LARGE_INTEGER;\n"
                               "    LARGE_INTEGER field;\n"
                               "};\n"
// unnamed struct as static / global(var)
                               "struct { int x; double y;         }             UnnamedStructUsedAsGlobalVar;\n"
                               "struct { struct { int x; } inner; } DoubleNestedUnnamedStructUsedAsGlobalVar;\n"
// typedef of using alias of typedef of using alias.
                               "typedef int A; using B = A; typedef B C; using D = C;\n"
                               "typedef struct { int x; } E; using F = E; typedef F G; using H = G;\n"
// template using alias
                               "typedef struct { int x; } UnnamedStruct; template <typename T> using UnnamedStructAlias = T; UnnamedStructAlias<UnnamedStruct> GlobalVariableUnnamedStruct;\n"
                               "namespace { struct Invisible {}; } UnnamedStructAlias<Invisible> GlobalVariableInvisibleStruct;\n"
                               "typedef enum { One, Two } UnnamedEnum; UnnamedStructAlias<UnnamedEnum> GlobalVariableUnnamedEnum;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(13, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 5, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(11, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct NamelessClassUsedAsAField {\n"
                                 "    class (unnamed class at input.cc:11:37) {\n"
                                 "        int x;\n"
                                 "        int y;\n"
                                 "    } field;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamelessEnumUsedAsAField {\n"
                                 "    enum (unnamed enum at input.cc:9:37) {\n"
                                 "        A,\n"
                                 "        B,\n"
                                 "        C\n"
                                 "    } field;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamelessStructUsedAsAField {\n"
                                 "    struct (unnamed struct at input.cc:10:37) {\n"
                                 "        int x;\n"
                                 "        int y;\n"
                                 "    } field;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamelessUnionUsedAsAField {\n"
                                 "    union (unnamed union at input.cc:12:37) {\n"
                                 "        int x;\n"
                                 "        int y;\n"
                                 "    } field;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct StructContainingALargeIntegerField {\n"
                                 "    typedef union (unnamed union at input.cc:15:13) {\n"
                                 "                struct (unnamed struct at input.cc:16:9) {\n"
                                 "                    unsigned long LowPart;\n"
                                 "                    long HighPart;\n"
                                 "                } Parts;\n"
                                 "                long long QuadPart;\n"
                                 "            } LARGE_INTEGER;\n"
                                 "    union (unnamed union at input.cc:15:13) {\n"
                                 "        struct (unnamed struct at input.cc:16:9) {\n"
                                 "            unsigned long LowPart;\n"
                                 "            long HighPart;\n"
                                 "        } Parts;\n"
                                 "        long long QuadPart;\n"
                                 "    } field;\n"
                                 "};\n"                    
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct TypedefAroundUnnamedClass {\n"
                                 "    typedef class (unnamed class at input.cc:3:45) {\n"
                                 "                int i;\n"
                                 "                double d;\n"
                                 "            } MyTypedefedUnnamedClass;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct TypedefAroundUnnamedEnum {\n"
                                 "    typedef enum (unnamed enum at input.cc:4:45) {\n"
                                 "                i = 0,\n"
                                 "                d\n"
                                 "            } MyTypedefedUnnmaedEnum;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct TypedefAroundUnnamedStruct {\n"
                                 "    typedef struct (unnamed struct at input.cc:2:45) {\n"
                                 "                int i;\n"
                                 "                double d;\n"
                                 "            } MyTypedefedUnnamedStruct;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct TypedefAroundUnnamedUnion {\n"
                                 "    typedef union (unnamed union at input.cc:1:45) {\n"
                                 "                int i;\n"
                                 "                double d;\n"
                                 "            } MyTypedefedUnnamedUnion;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct UsingAroundUnnamedClass {\n"
                                 "    using MyUsingAliasedUnnamedClass = class (unnamed class at input.cc:7:73) {\n"
                                 "                                           int i;\n"
                                 "                                           double d;\n"
                                 "                                       };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct UsingAroundUnnamedEnum {\n"
                                 "    using MyUsingAliasedUnnmaedEnum = enum (unnamed enum at input.cc:8:73) {\n"
                                 "                                          i = 0,\n"
                                 "                                          d\n"
                                 "                                      };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct UsingAroundUnnamedStruct {\n"
                                 "    using MyUsingAliasedUnnamedStruct = struct (unnamed struct at input.cc:6:73) {\n"
                                 "                                            int i;\n"
                                 "                                            double d;\n"
                                 "                                        };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct UsingAroundUnnamedUnion {\n"
                                 "    using MyUsingAliasedUnnamedUnion = union (unnamed union at input.cc:5:73) {\n"
                                 "                                           int i;\n"
                                 "                                           double d;\n"
                                 "                                       };\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("struct (unnamed struct at input.cc:22:1) {\n"
                                 "    struct (unnamed struct at input.cc:22:10) {\n"
                                 "        int x;\n"
                                 "    } inner;\n"
                                 "} DoubleNestedUnnamedStructUsedAsGlobalVar;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("UnnamedStructAlias<struct (anonymous namespace)::Invisible {\n"
                                 "                   }> GlobalVariableInvisibleStruct;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("UnnamedStructAlias<enum (unnamed enum at input.cc:27:9) {\n"
                                 "                       One,\n"
                                 "                       Two\n"
                                 "                   } > GlobalVariableUnnamedEnum;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("UnnamedStructAlias<struct (unnamed struct at input.cc:25:9) {\n"
                                 "                       int x;\n"
                                 "                   }> GlobalVariableUnnamedStruct;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (unnamed struct at input.cc:21:1) {\n"
                                 "    int x;\n"
                                 "    double y;\n"
                                 "} UnnamedStructUsedAsGlobalVar;\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:27:9) {\n"
                                 "    One,\n"
                                 "    Two\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef int A;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual(  "using B = A;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual(  "typedef B C;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual(  "using D = C;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (unnamed struct at input.cc:24:9) {\n"
                                 "            int x;\n"
                                 "        } E;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using F = struct (unnamed struct at input.cc:24:9) {\n"
                                 "              int x;\n"
                                 "          };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (unnamed struct at input.cc:24:9) {\n"
                                 "            int x;\n"
                                 "        } G;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using H = struct (unnamed struct at input.cc:24:9) {\n"
                                 "              int x;\n"
                                 "          };\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (unnamed enum at input.cc:27:9) {\n"
                                 "            One,\n"
                                 "            Two\n"
                                 "        } UnnamedEnum;\n"
                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (unnamed struct at input.cc:25:9) {\n"
                                 "            int x;\n"
                                 "        } UnnamedStruct;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using UnnamedStructAlias = T;\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
        }
    },

    {"Concepts", []
        {
            std::string code = 
                               "namespace std {"
                                   "template<typename T> inline constexpr bool is_integral_v = false;"
                                   "template<typename T, typename U> concept same_as = __is_same(T, U) && __is_same(U, T);"
                                   "template<typename T> inline constexpr bool is_pointer_v = __is_pointer(T);"
                                   "template<class T, class U> inline constexpr bool is_same_v = false;"
                               "}\n"
                               "template<typename T> concept C01 = sizeof(T) == 4;\n"
                               "template<typename T> concept C02 = sizeof(T) > 1;\n"
                               "template<typename T> concept C03 = requires(T t) { t.foo(); };\n"
                               "template<typename T> concept C04 = requires(T t) { t.foo(); t.bar(); };\n"
                               "template<typename T> concept C05 = requires { typename T::value_type; };\n"
                               "template<typename T> concept C06 = std::is_integral_v<T>; \n"
                               "template<typename T, typename U> concept SameSize    = sizeof(T) == sizeof(U);\n"
                               "template<typename T, typename U> concept Convertible = requires(T t) { static_cast<U>(t); };\n"
                               "template<typename T, typename U> concept HasMember   = requires(T t, U u) { t.foo(u); };\n"
                               "template<typename T> concept C07 = requires(T t) { { t.foo() }; };\n"
                               "template<typename T> concept C08 = requires(T t) { { t.foo() } -> std::same_as<int>; };\n"
                               "template<typename T> concept C09 = requires(T t) { { t.foo() } noexcept; };\n"
                               "template<typename T> concept C10 = requires(T t) { { t.foo() } noexcept -> std::same_as<int>; };\n"
                               "template<typename T> concept C11 = requires(T t) { requires sizeof(T) == 4; };\n"
                               "template<typename T> concept C12 = requires(T t) { t.foo(); requires std::is_integral_v<decltype(t.foo())>; };\n"
                               "template<typename T> concept C13 = requires { typename T::value_type; };\n"
                               "template<typename T> concept C14 = requires { typename T::iterator; typename T::const_iterator; };\n"
                               "template<typename T> concept C15 = sizeof(T) == 4;\n"
                               "template<typename T> concept C16 = C15<T> && requires(T t) { t.foo(); };\n"
                               "template<typename T> concept C17 = C15<T> || std::is_pointer_v<T>;\n"
                               "template<typename T> concept HasValueType  = requires { typename T::value_type; };\n"
                               "template<typename T> concept HasNestedType = requires { typename T::nested::type; };\n"
                               "template<typename T> concept HasAlias      = requires { typename T::alias; };\n"
                               "namespace N { struct X18 { static int value; }; } template<typename T> concept C18 = requires { N::X18::value; };\n"
                               "namespace N { template<typename T> concept C19 = sizeof(T) == 4; template<typename T> concept D19 = C19<T> && requires(T t) { t.foo(); }; }\n"
                               "namespace { struct InvisibleX1 {};   template<typename T> concept C20 = requires { typename T::value_type; }; template<typename T> concept D20 = sizeof(T) == sizeof(InvisibleX1); }\n"
                               "namespace { struct InvisibleX2 {}; } template<typename T> concept C21 = std::is_same_v<T, InvisibleX2>;\n"
                               "template<typename T> concept C = sizeof(T) == 4; template<C T> void f1(T) {}\n"
                               "template<typename T> requires C<T> void g1(T) {}\n"
                               "template<typename T> requires (C<T> && sizeof(T) > 1) void h1(T) {}\n"
                               "template<C T> void f2(T x) {}\n"
                               "void g2(C auto x) {}\n"
                               "void h2(C auto&& x) {}\n"
                               "template<typename T> concept HasFoo = requires(T t) { { t.foo() } noexcept -> std::same_as<int>; };\n"
                               "template<typename T> concept Valid = HasFoo<T> && requires { typename T::value_type; } && sizeof(T) == 4;\n"
                               "template<typename T> requires Valid<T> struct S { T value; template<typename U> requires HasFoo<U> void f(U u) { u.foo(); } };\n"
                               "template<typename T> auto FunctionWithTrailingAnonymousNamespaceReturnType(T) -> InvisibleX2 requires HasFoo<T> { return {}; }\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 3, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(31, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 7, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct X18 {\n"
                                 "    static int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires Valid<T> struct S {\n"
                                 "    T value;\n"
                                 "    template <typename U> requires HasFoo<U> void f(U u) {\n"
                                 "        u.foo();\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);

            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("template <typename T> inline constexpr bool is_integral_v = false;\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> inline constexpr bool is_pointer_v = __is_pointer(T);\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <class T, class U> inline constexpr bool is_same_v = false;\n"       , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
                Assert::AreEqual("template <typename T> concept C = sizeof(T) == 4;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C01 = sizeof(T) == 4;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C02 = sizeof(T) > 1;\n"                                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C03 = requires (T t) { t.foo(); };\n"                                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C04 = requires (T t) { t.foo(); t.bar(); };\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C05 = requires { typename T::value_type; };\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C06 = std::is_integral_v<T>;\n"                                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C07 = requires (T t) { { t.foo() }; };\n"                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C08 = requires (T t) { { t.foo() } -> std::same_as<int>; };\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C09 = requires (T t) { { t.foo() } noexcept; };\n"                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C10 = requires (T t) { { t.foo() } noexcept -> std::same_as<int>; };\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C11 = requires (T t) { requires sizeof(T) == 4; };\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C12 = requires (T t) { t.foo(); requires std::is_integral_v<decltype(t.foo())>; };\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C13 = requires { typename T::value_type; };\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C14 = requires { typename T::iterator; typename T::const_iterator; };\n"              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C15 = sizeof(T) == 4;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C16 = C15<T> && requires (T t) { t.foo(); };\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C17 = C15<T> || std::is_pointer_v<T>;\n"                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C18 = requires { N::X18::value; };\n"                                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept C21 = is_same_v<T, struct (anonymous namespace)::InvisibleX2 {\n"
                                 "                                                 }>;\n"                                                             , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template <typename T, typename U> concept Convertible = requires (T t) { static_cast<U>(t); };\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasAlias = requires { typename T::alias; };\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasFoo = requires (T t) { { t.foo() } noexcept -> std::same_as<int>; };\n"            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> concept HasMember = requires (T t, U u) { t.foo(u); };\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasNestedType = requires { typename T::nested::type; };\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasValueType = requires { typename T::value_type; };\n"                               , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template <typename T> concept C19 = sizeof(T) == 4;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept D19 = C19<T> && requires (T t) { t.foo(); };\n"                                       , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template <typename T, typename U> concept SameSize = sizeof(T) == sizeof(U);\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept Valid = HasFoo<T> && requires { typename T::value_type; } && sizeof(T) == 4;\n"       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> concept same_as = __is_same(T, U) && __is_same(U, T);\n"                          , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("template <typename T> auto FunctionWithTrailingAnonymousNamespaceReturnType(T) -> struct (anonymous namespace)::InvisibleX2 {\n"
                                 "                                                                                  } requires HasFoo<T> {\n"
                                 "                          return {};\n"
                                 "                      }\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <C T> void f1(T) {\n}\n"                                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <C T> void f2(T x) {\n}\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires C<T> void g1(T) {\n}\n"                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void g2(C auto x) {\n}\n"                                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires (C<T> && sizeof(T) > 1) void h1(T) {\n}\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void h2(C auto &&x) {\n}\n"                                               , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"User-defined literals", []
        {
            std::string code = 
                                             "constexpr int operator\"\"_km(unsigned long long value) { return static_cast<int>(value * 1000); }\n"
"namespace { struct Distance { int feet; }; } constexpr Distance operator\"\"_miles(unsigned long long value) { return Distance{static_cast<int>(value * 5280)}; }\n"
                             "namespace Foo { constexpr int operator\"\"_yards(unsigned long long value) { return static_cast<int>(value * 3); } }\n"
                                             "constexpr int operator\"\"_text(const char* str, size_t length) { return static_cast<int>(length); }\n"
                                             "constexpr int operator\"\"_rawtext(const char* str) { return 42; }\n"
                                             "constexpr double operator\"\"_feet(long double value) { return static_cast<double>(value); }\n"
                                             "constexpr int operator\"\"_letter(char value) { return value; }\n"
                                                       "int operator\"\"_count(unsigned long long value) { return static_cast<int>(value); }\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(8, maps.functionMap.size(), "wrong number of functions in map");

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
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("constexpr int operator\"\"_yards(unsigned long long value) {\n"
                                 "    return static_cast<int>(value * 3);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int operator\"\"_count(unsigned long long value) {\n"
                                 "    return static_cast<int>(value);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr double operator\"\"_feet(long double value) {\n"
                                 "    return static_cast<double>(value);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr int operator\"\"_km(unsigned long long value) {\n"
                                 "    return static_cast<int>(value * 1000);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr int operator\"\"_letter(char value) {\n"
                                 "    return value;\n"
                                 "}\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr struct (anonymous namespace)::Distance {\n"
                                 "              int feet;\n"
                                 "          } operator\"\"_miles(unsigned long long value) {\n"
                                 "    return Distance{static_cast<int>(value * 5280)};\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr int operator\"\"_rawtext(const char *str) {\n"
                                 "    return 42;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constexpr int operator\"\"_text(const char *str, size_t length) {\n"
                                 "    return static_cast<int>(length);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Spaceship operators", []
        {
            std::string code =
                               "namespace std\n" // mock up STL
                               "{\n"
                               "    struct strong_ordering\n"
                               "    {\n"
                               "        int value;\n"
                               "        friend constexpr bool operator==(strong_ordering, strong_ordering) = default;\n"
                               "        static const strong_ordering less;\n"
                               "        static const strong_ordering equal;\n"
                               "        static const strong_ordering equivalent;\n"
                               "        static const strong_ordering greater;\n"
                               "    };\n"
                               "    inline constexpr strong_ordering strong_ordering::less{-1};\n"
                               "    inline constexpr strong_ordering strong_ordering::equal{0};\n"
                               "    inline constexpr strong_ordering strong_ordering::equivalent{0};\n"
                               "    inline constexpr strong_ordering strong_ordering::greater{1};\n"
                               "}\n"
                               "namespace { struct Hidden1 { int value; }; } struct Public1 { int value;     friend auto operator<=>(const Public1& lhs, const Hidden1& rhs) { return lhs.value <=> rhs.value; } };\n"
                               "namespace { struct Ordering {}; }            struct Public2 { int value; friend Ordering operator<=>(const Public2&    , const Public2&    ) { return Ordering{}; } };\n"
                                                                            "struct Public3 { int value; };         auto operator<=>(const Public3& lhs, const Hidden1& rhs) { return lhs.value <=> rhs.value; }\n"
                                                                            "struct Public4 { int value; };     Ordering operator<=>(const Public4&    , const Public4&    ) { return {}; }\n"
                                                                            "struct Public5 { int value; };     Ordering operator<=>(const Public5    &, const Hidden1&    ) { return {}; }\n"
                                                                            "struct Public6 { int value; template<typename T> auto operator<=>(const T& rhs) const { return value <=> rhs.value; } };\n"
                                                                            "struct Public7 { int value; friend auto operator<=>(const Public7&, const Hidden1&); }; auto operator<=>(const Public7& lhs, const Hidden1& rhs) { return lhs.value <=> rhs.value; }\n"
             "namespace Outer { namespace { struct Hidden8 { int value; }; } struct Public8 {            friend auto operator<=>(const Public8&, const Hidden8&) { return 0 <=> 0; } }; }\n"
                               "struct Point  { int x; int y; auto operator<=>(const Point&) const = default; };\n"
                               "struct Point2 { int x; int y; std::strong_ordering operator<=>(const Point2& other) const { return x < other.x ? std::strong_ordering::less : x > other.x ? std::strong_ordering::greater : y < other.y ? std::strong_ordering::less : y > other.y ? std::strong_ordering::greater : std::strong_ordering::equal; } };\n"
                               "struct Point3 { int value; auto operator<=>(const Point3&) const& = default; };\n"
                               "struct Point4 { int value; auto operator<=>(const Point4& rhs) const&&        { return value <=> rhs.value; } };\n"
                               "struct Point5 { int value; auto operator<=>(const Point5& rhs) const noexcept { return value <=> rhs.value; } };\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(14, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 4, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 8, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Public8 {\n"
                                 "    friend std::strong_ordering operator<=>(const Public8 &, const struct Outer::(anonymous namespace)::Hidden8 {\n"
                                 "                                                                       int value;\n"
                                 "                                                                   } &) {\n"
                                 "               return 0 <=> 0;\n"
                                 "           }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Point {\n"
                                 "    int x;\n"
                                 "    int y;\n"
                                 "    std::strong_ordering operator<=>(const Point &) const = default;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Point2 {\n"
                                 "    int x;\n"
                                 "    int y;\n"
                                 "    std::strong_ordering operator<=>(const Point2 &other) const {\n"
                                 "        return this->x < other.x ? std::strong_ordering::less : this->x > other.x ? std::strong_ordering::greater : this->y < other.y ? std::strong_ordering::less : this->y > other.y ? std::strong_ordering::greater : std::strong_ordering::equal;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Point3 {\n"
                                 "    int value;\n"
                                 "    std::strong_ordering operator<=>(const Point3 &) const & = default;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Point4 {\n"
                                 "    int value;\n"
                                 "    std::strong_ordering operator<=>(const Point4 &rhs) const && {\n"
                                 "        return this->value <=> rhs.value;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Point5 {\n"
                                 "    int value;\n"
                                 "    std::strong_ordering operator<=>(const Point5 &rhs) const noexcept {\n"
                                 "        return this->value <=> rhs.value;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public1 {\n"
                                 "    int value;\n"
                                 "    friend std::strong_ordering operator<=>(const Public1 &lhs, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                                                          int value;\n"
                                 "                                                                      } &rhs) {\n"
                                 "               return lhs.value <=> rhs.value;\n"
                                 "           }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public2 {\n"
                                 "    int value;\n"
                                 "    friend struct (anonymous namespace)::Ordering {\n"
                                 "           } operator<=>(const Public2 &, const Public2 &) {\n"
                                 "               return Ordering{};\n"
                                 "           }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public3 {\n"
                                 "    int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public4 {\n"
                                 "    int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public5 {\n"
                                 "    int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public6 {\n"
                                 "    int value;\n"
                                 "    template <typename T> auto operator<=>(const T &rhs) const {\n"
                                 "        return this->value <=> rhs.value;\n"
                                 "    }\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct Public7 {\n"
                                 "    int value;\n"
                                 "    friend std::strong_ordering operator<=>(const Public7 &, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                                                       int value;\n"
                                 "                                                                   } &);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct strong_ordering {\n"
                                 "    int value;\n"
                                 "    friend bool operator==(strong_ordering, strong_ordering) = default;\n"
                                 "    static const strong_ordering less;\n"
                                 "    static const strong_ordering equal;\n"
                                 "    static const strong_ordering equivalent;\n"
                                 "    static const strong_ordering greater;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("inline constexpr strong_ordering std::strong_ordering::equal{0};\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline constexpr strong_ordering std::strong_ordering::equivalent{0};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline constexpr strong_ordering std::strong_ordering::greater{1};\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline constexpr strong_ordering std::strong_ordering::less{-1};\n"     , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("std::strong_ordering operator<=>(const Public8 &, const struct Outer::(anonymous namespace)::Hidden8 {\n"
                                 "                                                            int value;\n"
                                 "                                                        } &) {\n"
                                 "    return 0 <=> 0;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("std::strong_ordering operator<=>(const Public1 &lhs, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                                               int value;\n"
                                 "                                                           } &rhs) {\n"
                                 "    return lhs.value <=> rhs.value;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::Ordering {\n"
                                 "} operator<=>(const Public2 &, const Public2 &) {\n"
                                 "    return Ordering{};\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("std::strong_ordering operator<=>(const Public3 &lhs, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                                               int value;\n"
                                 "                                                           } &rhs) {\n"
                                 "    return lhs.value <=> rhs.value;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::Ordering {\n"
                                 "} operator<=>(const Public4 &, const Public4 &) {\n"
                                 "    return {};\n"
                                 "}\n"
                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::Ordering {\n"
                                 "} operator<=>(const Public5 &, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                         int value;\n"
                                 "                                     } &) {\n"
                                 "    return {};\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("std::strong_ordering operator<=>(const Public7 &lhs, const struct (anonymous namespace)::Hidden1 {\n"
                                 "                                                               int value;\n"
                                 "                                                           } &rhs) {\n"
                                 "    return lhs.value <=> rhs.value;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("bool operator==(strong_ordering, strong_ordering) = default;\n", (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Pointer-to-member functions", []
        {
            std::string code =
                               "struct Foo { int member(double); } foo;\n"
                               "typedef int (Foo::* FooMemberPtr)(double);\n"
                               "using FooMemberPtr2 = int (Foo::*)(double);\n"

                // no typedef or using alias
                               "int Take(Foo& foo, int (Foo::* pmf)(double)) { return (foo.*pmf)(3.14); }\n"
                               "int globalInt = Take(foo, &Foo::member);\n"
                               "int (Foo::* FunctionReturningPointerToMember())(double) { return &Foo::member; }\n"
                               "int (Foo::* var)(double) = &Foo::member;\n"

                // typedef and using
                               "int Take2(Foo& foo, FooMemberPtr  pmf) { return (foo.*pmf)(3.14); }\n"
                               "int Take3(Foo& foo, FooMemberPtr2 pmf) { return (foo.*pmf)(3.14); }\n"
                               "FooMemberPtr  FunctionReturningPointerToMember2() { return &Foo::member; }\n"
                               "FooMemberPtr2 FunctionReturningPointerToMember3() { return &Foo::member; }\n"
                               "FooMemberPtr var2 = &Foo::member; FooMemberPtr2 var3 = &Foo::member;\n"
                // anonymous namespace 
                               "namespace { struct FooAnon { int member(double) { return 42; } } fooAnon; }\n"
                               "typedef int (FooAnon::*FooAnonMemberPtr)(double);\n"
                               "using FooAnonMemberPtr2 = int (FooAnon::*)(double);\n"
                               "int Take4(FooAnon& foo, FooAnonMemberPtr pmf) { return (foo.*pmf)(3.14); }\n"
                               "int Take5(FooAnon& foo, FooAnonMemberPtr2 pmf) { return (foo.*pmf)(3.14); }\n"
                               "FooAnonMemberPtr FunctionReturningPointerToMember4() { return &FooAnon::member; }\n"
                               "FooAnonMemberPtr2 FunctionReturningPointerToMember5() { return &FooAnon::member; }\n"
                               "FooAnonMemberPtr var4 = &FooAnon::member;\n"
                               "FooAnonMemberPtr2 var5 = &FooAnon::member;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 7, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(10, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Foo {\n"
                                 "    int member(double);\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("struct Foo foo;\n"                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int globalInt = Take(foo, &Foo::member);\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (Foo::*var)(double) = &Foo::member;\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooMemberPtr var2 = &Foo::member;\n"        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooMemberPtr2 var3 = &Foo::member;\n"        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (struct (anonymous namespace)::FooAnon {\n"
                                 "         int member(double) {\n"
                                 "             return 42;\n"
                                 "         }\n"
                                 "     }::*)(double) var4 = &FooAnon::member;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (struct (anonymous namespace)::FooAnon {\n"
                                 "         int member(double) {\n"
                                 "             return 42;\n"
                                 "         }\n"
                                 "     }::*)(double) var5 = &FooAnon::member;\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef int (struct (anonymous namespace)::FooAnon {\n"
                                 "                 int member(double) {\n"
                                 "                     return 42;\n"
                                 "                 }\n"
                                 "             }::*FooAnonMemberPtr)(double);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using FooAnonMemberPtr2 = int (struct (anonymous namespace)::FooAnon {\n"
                                 "                                   int member(double) {\n"
                                 "                                       return 42;\n"
                                 "                                   }\n"
                                 "                               }::*)(double);\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef int (Foo::*FooMemberPtr)(double);\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using FooMemberPtr2 = int (Foo::*)(double);\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("int (Foo::*FunctionReturningPointerToMember())(double) {\n"
                                 "    return &Foo::member;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooMemberPtr FunctionReturningPointerToMember2() {\n"
                                 "    return &Foo::member;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FooMemberPtr2 FunctionReturningPointerToMember3() {\n"
                                 "    return &Foo::member;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (struct (anonymous namespace)::FooAnon {\n"
                                 "         int member(double) {\n"
                                 "             return 42;\n"
                                 "         }\n"
                                 "     }::*)(double) FunctionReturningPointerToMember4() {\n"
                                 "    return &FooAnon::member;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (struct (anonymous namespace)::FooAnon {\n"
                                 "         int member(double) {\n"
                                 "             return 42;\n"
                                 "         }\n"
                                 "     }::*)(double) FunctionReturningPointerToMember5() {\n"
                                 "    return &FooAnon::member;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Take(Foo &foo, int (Foo::*pmf)(double)) {\n"
                                 "    return (foo .* pmf)(3.1400000000000001);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Take2(Foo &foo, FooMemberPtr pmf) {\n"
                                 "    return (foo .* pmf)(3.1400000000000001);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Take3(Foo &foo, FooMemberPtr2 pmf) {\n"
                                 "    return (foo .* pmf)(3.1400000000000001);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Take4(struct (anonymous namespace)::FooAnon {\n"
                                 "              int member(double) {\n"
                                 "                  return 42;\n"
                                 "              }\n"
                                 "          } &foo, int (struct (anonymous namespace)::FooAnon {\n"
                                 "                           int member(double) {\n"
                                 "                               return 42;\n"
                                 "                           }\n"
                                 "                       }::*)(double) pmf) {\n"
                                 "    return (foo .* pmf)(3.1400000000000001);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int Take5(struct (anonymous namespace)::FooAnon {\n"
                                 "              int member(double) {\n"
                                 "                  return 42;\n"
                                 "              }\n"
                                 "          } &foo, int (struct (anonymous namespace)::FooAnon {\n"
                                 "                           int member(double) {\n"
                                 "                               return 42;\n"
                                 "                           }\n"
                                 "                       }::*)(double) pmf) {\n"
                                 "    return (foo .* pmf)(3.1400000000000001);\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Multi-dimensional arrays", []
        {
            std::string code =
                               "struct Foo { int value; };\n"
                               "void Function1(Foo array[2][3]) {}\n"
                               "typedef Foo ArrayType1[2][3];\n"
                               "using ArrayType2 = Foo[2][3];\n"
                               "Foo(&Function2())[2][3] { static Foo array[2][3]; return array; }\n"
                               "namespace { struct FooAnon { int value; }; }\n"
                               "void Function3(FooAnon array[2][3]) {}\n"
                               "typedef FooAnon ArrayType3[2][3];\n"
                               "using ArrayType4 = FooAnon[2][3];\n"
                               "FooAnon(&Function4())[2][3] { static FooAnon array[2][3]; return array; }\n"
                               "void Function5(ArrayType3 array) {}\n"
                               "ArrayType4& Function6() { static ArrayType4 array{}; return array; }\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(6, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct Foo {\n"
                                 "    int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef Foo ArrayType1[2][3];\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ArrayType2 = Foo[2][3];\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::FooAnon {\n"
                                 "            int value;\n"
                                 "        } ArrayType3[2][3];\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ArrayType4 = struct (anonymous namespace)::FooAnon {\n"
                                 "                       int value;\n"
                                 "                   } [2][3];\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void Function1(Foo array[2][3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Foo (&Function2())[2][3] {\n"
                                 "    static Foo array[2][3];\n"
                                 "    return array;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void Function3(struct (anonymous namespace)::FooAnon {\n"
                                 "                   int value;\n"
                                 "               } array[2][3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::FooAnon {\n"
                                 "    int value;\n"
                                 "} (&Function4())[2][3] {\n"
                                 "    static FooAnon array[2][3];\n"
                                 "    return array;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void Function5(struct (anonymous namespace)::FooAnon {\n"
                                 "                   int value;\n"
                                 "               } array[2][3]) {\n"
                                 "}\n"
                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::FooAnon {\n"
                                 "    int value;\n"
                                 "} &Function6()[2][3] {\n"
                                 "    static ArrayType4 array{};\n"
                                 "    return array;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"References to arrays", []
        {
            std::string code =
                               "enum Color { Red, Green, Blue };\n"
                               "Color colorArray1D[3];\n"
                               "Color colorArray2D[2][3];\n"
                               "Color (&referenceToGlobalArray1D)[3]    = colorArray1D;\n"
                               "Color (&referenceToGlobalArray2D)[2][3] = colorArray2D;\n"
                               "Color (&ReturnArray1D())[3] { return colorArray1D; }\n"
                               "Color (&ReturnArray2D())[2][3] { return colorArray2D; }\n"
                               "void ArgumentArray1D(Color (&arg)[3]) {}\n"
                               "void ArgumentArray2D(Color (&arg)[2][3]) {}\n"
                               "using ColorArray1D = Color[3];\n"
                               "typedef ColorArray1D ColorArray1DTypedef;\n"
                               "typedef Color ColorArray2DTypedef[2][3];\n"
                               "using ColorArray2D = ColorArray2DTypedef;\n"
                               "ColorArray1D& referenceUsingArray1D = colorArray1D;\n"
                               "ColorArray2D& referenceUsingArray2D = colorArray2D;\n"
                               "ColorArray1DTypedef& referenceTypedefArray1D = colorArray1D;\n"
                               "ColorArray2DTypedef& referenceTypedefArray2D = colorArray2D;\n"
                               "namespace { enum InvisibleColor { InvisibleRed, InvisibleGreen, InvisibleBlue }; }\n"
                               "InvisibleColor invisibleColorArray1D[3];\n"
                               "InvisibleColor invisibleColorArray2D[2][3];\n"
                               "InvisibleColor (&referenceToInvisibleGlobalArray1D)[3]    = invisibleColorArray1D;\n"
                               "InvisibleColor (&referenceToInvisibleGlobalArray2D)[2][3] = invisibleColorArray2D;\n"
                               "InvisibleColor (&ReturnInvisibleArray1D(int,double) noexcept)[3] { return invisibleColorArray1D; }\n"
                               "template<typename T> InvisibleColor (&ReturnInvisibleArray2D(int&))[2][3] requires (sizeof(T) == 4) { return invisibleColorArray2D; }\n"
                               "void ArgumentInvisibleArray1D(InvisibleColor (&arg)[3]) {}\n"
                               "void ArgumentInvisibleArray2D(InvisibleColor (&arg)[2][3]) {}\n"
                               "using InvisibleColorArray1D = InvisibleColor[3];\n"
                               "typedef InvisibleColorArray1D InvisibleColorArray1DTypedef;\n"
                               "typedef InvisibleColor InvisibleColorArray2DTypedef[2][3];\n"
                               "using InvisibleColorArray2D = InvisibleColorArray2DTypedef;\n"
                               "InvisibleColorArray1D& referenceUsingInvisibleArray1D = invisibleColorArray1D;\n"
                               "InvisibleColorArray2D& referenceUsingInvisibleArray2D = invisibleColorArray2D;\n"
                               "InvisibleColorArray1DTypedef& referenceTypedefInvisibleArray1D = invisibleColorArray1D;\n"
                               "InvisibleColorArray2DTypedef& referenceTypedefInvisibleArray2D = invisibleColorArray2D;\n"
                               "InvisibleColor (*pointerToInvisibleArray1D)[3] = &invisibleColorArray1D;\n"
                               "InvisibleColor (**pointerToPointerToInvisibleArray1D)[3] = &pointerToInvisibleArray1D;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(18, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 8, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 8, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("Color colorArray1D[3];\n"                                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Color colorArray2D[2][3];\n"                                                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} invisibleColorArray1D[3];\n"                                                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} invisibleColorArray2D[2][3];\n"                                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (*pointerToInvisibleArray1D)[3] = &invisibleColorArray1D;\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (**pointerToPointerToInvisibleArray1D)[3] = &pointerToInvisibleArray1D;\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Color (&referenceToGlobalArray1D)[3] = colorArray1D;\n"                                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Color (&referenceToGlobalArray2D)[2][3] = colorArray2D;\n"                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (&referenceToInvisibleGlobalArray1D)[3] = invisibleColorArray1D;\n"                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (&referenceToInvisibleGlobalArray2D)[2][3] = invisibleColorArray2D;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ColorArray1DTypedef &referenceTypedefArray1D = colorArray1D;\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ColorArray2DTypedef &referenceTypedefArray2D = colorArray2D;\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} &referenceTypedefInvisibleArray1D[3] = invisibleColorArray1D;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} &referenceTypedefInvisibleArray2D[2][3] = invisibleColorArray2D;\n"                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ColorArray1D &referenceUsingArray1D = colorArray1D;\n"                                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ColorArray2D &referenceUsingArray2D = colorArray2D;\n"                                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} &referenceUsingInvisibleArray1D[3] = invisibleColorArray1D;\n"           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} &referenceUsingInvisibleArray2D[2][3] = invisibleColorArray2D;\n"        , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum Color {\n"
                                 "    Red,\n"
                                 "    Green,\n"
                                 "    Blue\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using ColorArray1D = Color[3];\n"           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef ColorArray1D ColorArray1DTypedef;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ColorArray2D = ColorArray2DTypedef;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Color ColorArray2DTypedef[2][3];\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using InvisibleColorArray1D = enum (anonymous namespace)::InvisibleColor {\n"
                                 "                                  InvisibleRed,\n"
                                 "                                  InvisibleGreen,\n"
                                 "                                  InvisibleBlue\n"
                                 "                              }[3];\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::InvisibleColor {\n"
                                 "            InvisibleRed,\n"
                                 "            InvisibleGreen,\n"
                                 "            InvisibleBlue\n"
                                 "        } InvisibleColorArray1DTypedef[3];\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using InvisibleColorArray2D = enum (anonymous namespace)::InvisibleColor {\n"
                                 "                                  InvisibleRed,\n"
                                 "                                  InvisibleGreen,\n"
                                 "                                  InvisibleBlue\n"
                                 "                              }[2][3];\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::InvisibleColor {\n"
                                 "            InvisibleRed,\n"
                                 "            InvisibleGreen,\n"
                                 "            InvisibleBlue\n"
                                 "        } InvisibleColorArray2DTypedef[2][3];\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void ArgumentArray1D(Color (&arg)[3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void ArgumentArray2D(Color (&arg)[2][3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void ArgumentInvisibleArray1D(enum (anonymous namespace)::InvisibleColor {\n"
                                 "                                  InvisibleRed,\n"
                                 "                                  InvisibleGreen,\n"
                                 "                                  InvisibleBlue\n"
                                 "                              } (&arg)[3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void ArgumentInvisibleArray2D(enum (anonymous namespace)::InvisibleColor {\n"
                                 "                                  InvisibleRed,\n"
                                 "                                  InvisibleGreen,\n"
                                 "                                  InvisibleBlue\n"
                                 "                              } (&arg)[2][3]) {\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Color (&ReturnArray1D())[3] {\n"
                                 "    return colorArray1D;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("Color (&ReturnArray2D())[2][3] {\n"
                                 "    return colorArray2D;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (&ReturnInvisibleArray1D(int, double) noexcept)[3] {\n"
                                 "    return invisibleColorArray1D;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> enum (anonymous namespace)::InvisibleColor {\n"
                                 "    InvisibleRed,\n"
                                 "    InvisibleGreen,\n"
                                 "    InvisibleBlue\n"
                                 "} (&ReturnInvisibleArray2D(int &))[2][3] requires (sizeof(T) == 4) {\n"
                                 "    return invisibleColorArray2D;\n"
                                 "}\n"
                              , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"References to functions", []
        {
            std::string code =
                               "void FunctionReferencedByReference() {}\n"
                               "void (&functionReference)() = FunctionReferencedByReference;\n"

                               "namespace { struct AnonymousReturnType {}; struct AnonymousArgumentType {}; }\n"
                               "AnonymousReturnType FunctionWithAnonymousTypes(AnonymousArgumentType) { return {}; }\n"
                               "AnonymousReturnType (&functionReferenceWithAnonymousTypes)(AnonymousArgumentType) = FunctionWithAnonymousTypes;\n"

                               "typedef void FunctionTypedef();\n"
                               "FunctionTypedef& functionTypedefVariable = FunctionReferencedByReference;\n"
                               "void FunctionTakingFunctionTypedef(FunctionTypedef) {}\n"
                               "using FunctionUsing = void();\n"
                               "FunctionUsing& functionUsingVariable = FunctionReferencedByReference;\n"
                               "void FunctionTakingFunctionUsing(FunctionUsing) {}\n"

                               "typedef AnonymousReturnType AnonymousFunctionTypedef(AnonymousArgumentType);\n"
                               "AnonymousFunctionTypedef& functionAnonymousTypedefVariable = FunctionWithAnonymousTypes;\n"
                               "void FunctionTakingAnonymousFunctionTypedef(AnonymousFunctionTypedef) {}\n"
                               "using AnonymousFunctionUsing = AnonymousReturnType(&)(AnonymousArgumentType);\n"
                               "AnonymousFunctionUsing functionAnonymousUsingVariable = FunctionWithAnonymousTypes;\n"
                               "void FunctionTakingAnonymousFunctionUsing(AnonymousFunctionUsing) {}\n"

                               "void (&& functionRvalueReference)() = FunctionReferencedByReference;\n"
                               "void NoexceptFunction() noexcept {}\n"
                               "void (&noexceptFunctionReference)() noexcept = NoexceptFunction;\n"
                               "void VariadicFunction(int, ...) {}\n"
                               "void (&variadicFunctionReference)(int, ...) = VariadicFunction;\n"

                               "namespace { struct AnonymousRvalueReturnType {}; struct AnonymousRvalueArgumentType {}; }\n"
                               "AnonymousRvalueReturnType FunctionWithAnonymousRvalueTypes(AnonymousRvalueArgumentType) { return {}; }\n"
                               "AnonymousRvalueReturnType (&&functionAnonymousRvalueReference)(AnonymousRvalueArgumentType) = FunctionWithAnonymousRvalueTypes;\n"

                               "typedef AnonymousRvalueReturnType AnonymousRvalueFunctionTypedef(AnonymousRvalueArgumentType);\n"
                               "AnonymousRvalueFunctionTypedef&& functionAnonymousRvalueTypedefReference = FunctionWithAnonymousRvalueTypes;\n"
                               "using AnonymousRvalueFunctionUsing = AnonymousRvalueReturnType(AnonymousRvalueArgumentType);\n"
                               "AnonymousRvalueFunctionUsing&& functionAnonymousRvalueUsingReference = FunctionWithAnonymousRvalueTypes;\n"

                               "namespace { struct AnonymousNoexceptReturnType {}; struct AnonymousNoexceptArgumentType {}; }\n"
                               "AnonymousNoexceptReturnType FunctionWithAnonymousNoexceptTypes(AnonymousNoexceptArgumentType) noexcept { return {}; }\n"
                               "AnonymousNoexceptReturnType (&functionAnonymousNoexceptReference)(AnonymousNoexceptArgumentType) noexcept = FunctionWithAnonymousNoexceptTypes;\n"

                               "typedef AnonymousNoexceptReturnType AnonymousNoexceptFunctionTypedef(AnonymousNoexceptArgumentType) noexcept;\n"
                               "AnonymousNoexceptFunctionTypedef& functionAnonymousNoexceptTypedefReference = FunctionWithAnonymousNoexceptTypes;\n"

                               "using AnonymousNoexceptFunctionUsing = AnonymousNoexceptReturnType(AnonymousNoexceptArgumentType) noexcept;\n"
                               "AnonymousNoexceptFunctionUsing& functionAnonymousNoexceptUsingReference = FunctionWithAnonymousNoexceptTypes;\n"

                               "namespace { struct AnonymousVariadicReturnType {}; struct AnonymousVariadicArgumentType {}; }\n"
                               "AnonymousVariadicReturnType FunctionWithAnonymousVariadicTypes(AnonymousVariadicArgumentType, ...) { return {}; }\n"
                               "AnonymousVariadicReturnType (&functionAnonymousVariadicReference)(AnonymousVariadicArgumentType, ...) = FunctionWithAnonymousVariadicTypes;\n"

                               "typedef AnonymousVariadicReturnType AnonymousVariadicFunctionTypedef(AnonymousVariadicArgumentType, ...);\n"
                               "AnonymousVariadicFunctionTypedef& functionAnonymousVariadicTypedefReference = FunctionWithAnonymousVariadicTypes;\n"

                               "using AnonymousVariadicFunctionUsing = AnonymousVariadicReturnType(AnonymousVariadicArgumentType, ...);\n"
                               "AnonymousVariadicFunctionUsing& functionAnonymousVariadicUsingReference = FunctionWithAnonymousVariadicTypes;\n"

                               "namespace { struct AnonymousRvalueNoexceptReturnType {}; struct AnonymousRvalueNoexceptArgumentType {}; }\n"
                               "AnonymousRvalueNoexceptReturnType FunctionWithAnonymousRvalueNoexceptTypes(AnonymousRvalueNoexceptArgumentType) noexcept { return {}; }\n"
                               "using AnonymousRvalueNoexceptFunctionUsing = AnonymousRvalueNoexceptReturnType(AnonymousRvalueNoexceptArgumentType) noexcept;\n"
                               "AnonymousRvalueNoexceptFunctionUsing&& functionAnonymousRvalueNoexceptUsingReference = FunctionWithAnonymousRvalueNoexceptTypes;\n"

                               "typedef AnonymousRvalueNoexceptReturnType AnonymousRvalueNoexceptFunctionTypedef(AnonymousRvalueNoexceptArgumentType) noexcept;\n"
                               "AnonymousRvalueNoexceptFunctionTypedef&& functionAnonymousRvalueNoexceptTypedefReference = FunctionWithAnonymousRvalueNoexceptTypes;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(20, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(12, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(12, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "} (&functionAnonymousNoexceptReference)(struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                        }) noexcept = FunctionWithAnonymousNoexceptTypes;\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "} &functionAnonymousNoexceptTypedefReference(struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                             }) noexcept = FunctionWithAnonymousNoexceptTypes;\n"             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "} &functionAnonymousNoexceptUsingReference(struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                           }) noexcept = FunctionWithAnonymousNoexceptTypes;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueNoexceptReturnType {\n"
                                 "} &&functionAnonymousRvalueNoexceptTypedefReference(struct (anonymous namespace)::AnonymousRvalueNoexceptArgumentType {\n"
                                 "                                                    }) noexcept = FunctionWithAnonymousRvalueNoexceptTypes;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueNoexceptReturnType {\n"
                                 "} &&functionAnonymousRvalueNoexceptUsingReference(struct (anonymous namespace)::AnonymousRvalueNoexceptArgumentType {\n"
                                 "                                                  }) noexcept = FunctionWithAnonymousRvalueNoexceptTypes;\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "} (&&functionAnonymousRvalueReference)(struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                       }) = FunctionWithAnonymousRvalueTypes;\n"                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "} &&functionAnonymousRvalueTypedefReference(struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                            }) = FunctionWithAnonymousRvalueTypes;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "} &&functionAnonymousRvalueUsingReference(struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                          }) = FunctionWithAnonymousRvalueTypes;\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "} &functionAnonymousTypedefVariable(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                    }) = FunctionWithAnonymousTypes;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "} (&functionAnonymousUsingVariable)(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                    }) = FunctionWithAnonymousTypes;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "} (&functionAnonymousVariadicReference)(struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                        },...) = FunctionWithAnonymousVariadicTypes;\n"                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "} &functionAnonymousVariadicTypedefReference(struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                             },...) = FunctionWithAnonymousVariadicTypes;\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "} &functionAnonymousVariadicUsingReference(struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                           },...) = FunctionWithAnonymousVariadicTypes;\n"                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void (&functionReference)() = FunctionReferencedByReference;\n"                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "} (&functionReferenceWithAnonymousTypes)(struct (anonymous namespace)::AnonymousArgumentType {\n"             
                                 "                                         }) = FunctionWithAnonymousTypes;\n"                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void (&&functionRvalueReference)() = FunctionReferencedByReference;\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("FunctionTypedef &functionTypedefVariable = FunctionReferencedByReference;\n"                                  , (*it++).second[0].fullyQualified); 
                Assert::AreEqual("FunctionUsing &functionUsingVariable = FunctionReferencedByReference;\n"                                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void (&noexceptFunctionReference)() noexcept = NoexceptFunction;\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void (&variadicFunctionReference)(int, ...) = VariadicFunction;\n"                                            , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "        } AnonymousFunctionTypedef(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                   });\n"                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousFunctionUsing = struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "                               } (&)(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                     });\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "        } AnonymousNoexceptFunctionTypedef(struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                           }) noexcept;\n"      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousNoexceptFunctionUsing = struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "                                       } (struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                          }) noexcept;\n"       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "        } AnonymousRvalueFunctionTypedef(struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                         });\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousRvalueFunctionUsing = struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "                                     } (struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                        });\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousRvalueNoexceptReturnType {\n"
                                 "        } AnonymousRvalueNoexceptFunctionTypedef(struct (anonymous namespace)::AnonymousRvalueNoexceptArgumentType {\n"
                                 "                                                 }) noexcept;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousRvalueNoexceptFunctionUsing = struct (anonymous namespace)::AnonymousRvalueNoexceptReturnType {\n"
                                 "                                             } (struct (anonymous namespace)::AnonymousRvalueNoexceptArgumentType {\n"
                                 "                                                }) noexcept;\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "        } AnonymousVariadicFunctionTypedef(struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                           },...);\n"           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousVariadicFunctionUsing = struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "                                       } (struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                          },...);\n"            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef void (FunctionTypedef)();\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using FunctionUsing = void ();\n"                               , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void FunctionReferencedByReference() {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void FunctionTakingAnonymousFunctionTypedef(struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "                                            } (struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                               })) {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void FunctionTakingAnonymousFunctionUsing(struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "                                          } (&)(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                                                })) {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void FunctionTakingFunctionTypedef(FunctionTypedef) {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void FunctionTakingFunctionUsing(FunctionUsing) {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousNoexceptReturnType {\n"
                                 "} FunctionWithAnonymousNoexceptTypes(struct (anonymous namespace)::AnonymousNoexceptArgumentType {\n"
                                 "                                     }) noexcept {\n"
                                 "    return {};\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueNoexceptReturnType {\n"
                                 "} FunctionWithAnonymousRvalueNoexceptTypes(struct (anonymous namespace)::AnonymousRvalueNoexceptArgumentType {\n"
                                 "                                           }) noexcept {\n"
                                 "    return {};\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousRvalueReturnType {\n"
                                 "} FunctionWithAnonymousRvalueTypes(struct (anonymous namespace)::AnonymousRvalueArgumentType {\n"
                                 "                                   }) {\n"
                                 "    return {};\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousReturnType {\n"
                                 "} FunctionWithAnonymousTypes(struct (anonymous namespace)::AnonymousArgumentType {\n"
                                 "                             }) {\n"
                                 "    return {};\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousVariadicReturnType {\n"
                                 "} FunctionWithAnonymousVariadicTypes(struct (anonymous namespace)::AnonymousVariadicArgumentType {\n"
                                 "                                     }, ...) {\n"
                                 "    return {};\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void NoexceptFunction() noexcept {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("void VariadicFunction(int, ...) {\n"
                                 "}\n"                                                          , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Ararys of and pointers to functions", []
        {
            std::string code =
                               "typedef int IntTypedef; using IntAlias = int;"
                               "IntAlias Foo(IntTypedef) { return {}; } IntTypedef Bar(IntAlias) { return {}; } int (* table[2])(int) = { &Foo, &Bar };\n"

                               "namespace { enum AnonymousEnumForTypedef { AnonymousEnumValue }; } typedef AnonymousEnumForTypedef AnonymousEnumTypedef;\n"
                               "namespace { struct AnonymousStructForAlias {}; } using AnonymousStructAlias = AnonymousStructForAlias;\n"
                               "AnonymousStructAlias Baz(AnonymousEnumTypedef) { return {}; }\n"
                               "AnonymousEnumTypedef Qux(AnonymousStructAlias) { return {}; }\n"
                               "AnonymousStructAlias (*bazTable[1])(AnonymousEnumTypedef) = { &Baz };\n"
                               "AnonymousEnumTypedef (*quxTable[1])(AnonymousStructAlias) = { &Qux };\n"
                               
                               "struct DeepAliasType {};\n"
                               "typedef DeepAliasType DeepTypedef1; using DeepUsing1 = DeepTypedef1;\n"
                               "typedef DeepUsing1    DeepTypedef2; using DeepUsing2 = DeepTypedef2;\n"
                               "typedef DeepUsing2    DeepTypedef3; using DeepUsing3 = DeepTypedef3;\n"
                               "typedef DeepUsing3    DeepTypedef4; using DeepUsing4 = DeepTypedef4;\n"
                               "typedef DeepUsing4    DeepTypedef5; using DeepUsing5 = DeepTypedef5;\n"
                               "DeepUsing5 deepAliasVariable;\n"

                               "namespace { enum DeepAnonymousEnum { DeepAnonymousRed, DeepAnonymousGreen, DeepAnonymousBlue }; }\n"
                               "typedef DeepAnonymousEnum DeepEnumTypedef1; using DeepEnumUsing1 = DeepEnumTypedef1;\n"
                               "typedef DeepEnumUsing1 DeepEnumTypedef2;    using DeepEnumUsing2 = DeepEnumTypedef2;\n"
                               "typedef DeepEnumUsing2 DeepEnumTypedef3;    using DeepEnumUsing3 = DeepEnumTypedef3;\n"
                               "typedef DeepEnumUsing3 DeepEnumTypedef4;    using DeepEnumUsing4 = DeepEnumTypedef4;\n"
                               "typedef DeepEnumUsing4 DeepEnumTypedef5;    using DeepEnumUsing5 = DeepEnumTypedef5;\n"
                               "DeepEnumUsing5 deepEnumVariable;\n"
                                
                               "AnonymousStructAlias (*bazTable2[2][3])(AnonymousEnumTypedef) = {\n"
                               "    { &Baz, &Baz, &Baz },\n"
                               "    { &Baz, &Baz, &Baz }\n"
                               "};\n"
                               "AnonymousEnumTypedef (*quxTable2[2][3])(AnonymousStructAlias) = {\n"
                               "    { &Qux, &Qux, &Qux },\n"
                               "    { &Qux, &Qux, &Qux }\n"
                               "};\n"

                               "int (&functionReference)(int) = Foo;\n"
                               "typedef int (&FunctionReferenceTypedef)(int); using FunctionReferenceAlias = FunctionReferenceTypedef; FunctionReferenceAlias aliasedFunctionReference = Foo;\n"
                               "int *UniquePointerReturningFunction  (int) { return nullptr;           } int *(*pointerReturningFunctionTable[2])(int)   = { &UniquePointerReturningFunction, &UniquePointerReturningFunction };\n"
                               "int &UniqueReferenceReturningFunction(int) { static int x{}; return x; } int &(*referenceReturningFunctionTable[2])(int) = { &UniqueReferenceReturningFunction, &UniqueReferenceReturningFunction };\n"
                               "using IntArray3 = int[3]; IntArray3 &UniqueFunctionReturningAliasedArray() { static int x[3]{}; return x; }\n"

                               "using ArrayReturningFunctionPointer = IntArray3 &(*)(void); ArrayReturningFunctionPointer arrayReturningFunctionPointer = &UniqueFunctionReturningAliasedArray;\n"

                               "using IntArray3Again = int[3]; IntArray3Again &UniqueFunctionReturningAliasedArray2() { static int x[3]{}; return x; } "
                               "using ArrayReferenceFunctionPointer = IntArray3Again &(*)(void);\n"
                               "ArrayReferenceFunctionPointer arrayReferenceFunctionPointerTable[2] = {\n&UniqueFunctionReturningAliasedArray2,\n&UniqueFunctionReturningAliasedArray2\n};\n"

                               "AnonymousStructAlias *UniquePointerParameterFunction(AnonymousEnumTypedef *) { return nullptr; }\n"
                               "AnonymousStructAlias &UniqueReferenceParameterFunction(AnonymousEnumTypedef &) { static AnonymousStructAlias x{}; return x; }\n"

                               "using UniquePointerParameterFunctionType = AnonymousStructAlias *(*)(AnonymousEnumTypedef *); "
                               "UniquePointerParameterFunctionType uniquePointerParameterFunction = &UniquePointerParameterFunction;\n"

                               "using UniqueReferenceParameterFunctionType = AnonymousStructAlias &(*)(AnonymousEnumTypedef &); "
                               "UniqueReferenceParameterFunctionType uniqueReferenceParameterFunction = &UniqueReferenceParameterFunction;\n"

                               "typedef AnonymousStructAlias (*BazFunctionPointerTypedef)(AnonymousEnumTypedef); "
                               "using BazFunctionPointerAlias1 = BazFunctionPointerTypedef; "
                               "typedef BazFunctionPointerAlias1 BazFunctionPointerTypedef2; "
                               "using BazFunctionPointerAlias2 = BazFunctionPointerTypedef2; "
                               "typedef BazFunctionPointerAlias2 BazFunctionPointerTypedef3; "
                               "using BazFunctionPointerAlias3 = BazFunctionPointerTypedef3; "
                               "BazFunctionPointerAlias3 *****deepFunctionPointer = nullptr;\n"
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(16, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(38, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(10, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct DeepAliasType {\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("FunctionReferenceAlias aliasedFunctionReference = Foo;\n"                                                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ArrayReferenceFunctionPointer arrayReferenceFunctionPointerTable[2] = {&UniqueFunctionReturningAliasedArray2, &UniqueFunctionReturningAliasedArray2};\n"
                                                                                                                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("ArrayReturningFunctionPointer arrayReturningFunctionPointer = &UniqueFunctionReturningAliasedArray;\n"                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} (*bazTable[1])(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                     AnonymousEnumValue\n"
                                 "                 } ) = {&Baz};\n"                                                                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} (*bazTable2[2][3])(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                         AnonymousEnumValue\n"
                                 "                     } ) = {{&Baz, &Baz, &Baz}, {&Baz, &Baz, &Baz}};\n"                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("DeepUsing5 deepAliasVariable;\n"                                                                                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "    DeepAnonymousRed,\n"
                                 "    DeepAnonymousGreen,\n"
                                 "    DeepAnonymousBlue\n"
                                 "} deepEnumVariable;\n"                                                                                                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} (******deepFunctionPointer)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                  AnonymousEnumValue\n"
                                 "                              } ) = nullptr;\n"                                                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (&functionReference)(int) = Foo;\n"                                                                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int *(*pointerReturningFunctionTable[2])(int) = {&UniquePointerReturningFunction, &UniquePointerReturningFunction};\n"      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "    AnonymousEnumValue\n"
                                 "} (*quxTable[1])(struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                 }) = {&Qux};\n"                                                                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "    AnonymousEnumValue\n"
                                 "} (*quxTable2[2][3])(struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                     }) = {{&Qux, &Qux, &Qux}, {&Qux, &Qux, &Qux}};\n"                                                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int &(*referenceReturningFunctionTable[2])(int) = {&UniqueReferenceReturningFunction, &UniqueReferenceReturningFunction};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("int (*table[2])(int) = {&Foo, &Bar};\n"                                                                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} * (*uniquePointerParameterFunction)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                          AnonymousEnumValue\n"
                                 "                                      } *) = &UniquePointerParameterFunction;\n"                                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} & (*uniqueReferenceParameterFunction)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                            AnonymousEnumValue\n"
                                 "                                        } &) = &UniqueReferenceParameterFunction;\n"                                         , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "            AnonymousEnumValue\n"
                                 "        } AnonymousEnumTypedef;\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousStructAlias = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                             };\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ArrayReferenceFunctionPointer = IntArray3Again &(*)();\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ArrayReturningFunctionPointer = IntArray3 &(*)();\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using BazFunctionPointerAlias1 = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                                 } (*)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                           AnonymousEnumValue\n"
                                 "                                       } );\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using BazFunctionPointerAlias2 = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                                 } (*)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                           AnonymousEnumValue\n"
                                 "                                       } );\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using BazFunctionPointerAlias3 = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                                 } (*)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                           AnonymousEnumValue\n"
                                 "                                       } );\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "        } *BazFunctionPointerTypedef(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                         AnonymousEnumValue\n"
                                 "                                     } );\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "        } *BazFunctionPointerTypedef2(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                          AnonymousEnumValue\n"
                                 "                                      } );\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "        } *BazFunctionPointerTypedef3(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                          AnonymousEnumValue\n"
                                 "                                      } );\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "            DeepAnonymousRed,\n"
                                 "            DeepAnonymousGreen,\n"
                                 "            DeepAnonymousBlue\n"
                                 "        } DeepEnumTypedef1;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "            DeepAnonymousRed,\n"
                                 "            DeepAnonymousGreen,\n"
                                 "            DeepAnonymousBlue\n"
                                 "        } DeepEnumTypedef2;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "            DeepAnonymousRed,\n"
                                 "            DeepAnonymousGreen,\n"
                                 "            DeepAnonymousBlue\n"
                                 "        } DeepEnumTypedef3;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "            DeepAnonymousRed,\n"
                                 "            DeepAnonymousGreen,\n"
                                 "            DeepAnonymousBlue\n"
                                 "        } DeepEnumTypedef4;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "            DeepAnonymousRed,\n"
                                 "            DeepAnonymousGreen,\n"
                                 "            DeepAnonymousBlue\n"
                                 "        } DeepEnumTypedef5;\n"                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepEnumUsing1 = enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "                           DeepAnonymousRed,\n"
                                 "                           DeepAnonymousGreen,\n"
                                 "                           DeepAnonymousBlue\n"
                                 "                       };\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepEnumUsing2 = enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "                           DeepAnonymousRed,\n"
                                 "                           DeepAnonymousGreen,\n"
                                 "                           DeepAnonymousBlue\n"
                                 "                       };\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepEnumUsing3 = enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "                           DeepAnonymousRed,\n"
                                 "                           DeepAnonymousGreen,\n"
                                 "                           DeepAnonymousBlue\n"
                                 "                       };\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepEnumUsing4 = enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "                           DeepAnonymousRed,\n"
                                 "                           DeepAnonymousGreen,\n"
                                 "                           DeepAnonymousBlue\n"
                                 "                       };\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepEnumUsing5 = enum (anonymous namespace)::DeepAnonymousEnum {\n"
                                 "                           DeepAnonymousRed,\n"
                                 "                           DeepAnonymousGreen,\n"
                                 "                           DeepAnonymousBlue\n"
                                 "                       };\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef DeepAliasType DeepTypedef1;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef DeepUsing1 DeepTypedef2;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef DeepUsing2 DeepTypedef3;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef DeepUsing3 DeepTypedef4;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef DeepUsing4 DeepTypedef5;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepUsing1 = DeepTypedef1;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepUsing2 = DeepTypedef2;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepUsing3 = DeepTypedef3;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepUsing4 = DeepTypedef4;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using DeepUsing5 = DeepTypedef5;\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using FunctionReferenceAlias = FunctionReferenceTypedef;\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef int (&FunctionReferenceTypedef)(int);\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using IntAlias = int;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using IntArray3 = int[3];\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using IntArray3Again = int[3];\n"                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef int IntTypedef;\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using UniquePointerParameterFunctionType = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                                           } * (*)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                                       AnonymousEnumValue\n"
                                 "                                                   } *);\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using UniqueReferenceParameterFunctionType = struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "                                             } & (*)(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                                         AnonymousEnumValue\n"
                                 "                                                     } &);\n"  , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("IntTypedef Bar(IntAlias) {\n"
                                 "    return {};\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} Baz(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "          AnonymousEnumValue\n"
                                 "      }) {\n"
                                 "    return {};\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("IntAlias Foo(IntTypedef) {\n"
                                 "    return {};\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "    AnonymousEnumValue\n"
                                 "} Qux(struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "      }) {\n"
                                 "    return {};\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("IntArray3 &UniqueFunctionReturningAliasedArray() {\n"
                                 "    static int x[3]{};\n"
                                 "    return x;\n"
                                 "}\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("IntArray3Again &UniqueFunctionReturningAliasedArray2() {\n"
                                 "    static int x[3]{};\n"
                                 "    return x;\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} *UniquePointerParameterFunction(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                      AnonymousEnumValue\n"
                                 "                                  } *) {\n"
                                 "    return nullptr;\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int *UniquePointerReturningFunction(int) {\n"
                                 "    return nullptr;\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct (anonymous namespace)::AnonymousStructForAlias {\n"
                                 "} &UniqueReferenceParameterFunction(enum (anonymous namespace)::AnonymousEnumForTypedef {\n"
                                 "                                        AnonymousEnumValue\n"
                                 "                                    } &) {\n"
                                 "    static AnonymousStructAlias x{};\n"
                                 "    return x;\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int &UniqueReferenceReturningFunction(int) {\n"
                                 "    static int x{};\n"
                                 "    return x;\n"
                                 "}\n"                                          , (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Access specifier changes within single class", []
        {
            std::string code =
                                "namespace { struct AnonymousAccessStruct {}; enum AnonymousAccessEnum { AnonymousAccessValue }; }\n"
                                "class AccessSpecifierChanges\n"
                                "{\n"
                                "public:\n"
                                "    AnonymousAccessStruct publicMember1;\n"
                                "protected:\n"
                                "    AnonymousAccessEnum protectedMember1;\n"
                                "    int protectedMember2;\n"
                                "private:\n"
                                "    AnonymousAccessStruct privateMember1;\n"
                                "public:\n"
                                "    int publicMember2;\n"
                                "private:\n"
                                "    AnonymousAccessEnum privateMember2;\n"
                                "protected:\n"
                                "    AnonymousAccessStruct protectedMember3;\n"
                                "public:\n"
                                "    int publicMember3;\n"
                                "};\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("class AccessSpecifierChanges {\n"
                                 "public:\n"
                                 "    struct (anonymous namespace)::AnonymousAccessStruct {\n"
                                 "    } publicMember1;\n"
                                 "protected:\n"
                                 "    enum (anonymous namespace)::AnonymousAccessEnum {\n"
                                 "        AnonymousAccessValue\n"
                                 "    } protectedMember1;\n"
                                 "    int protectedMember2;\n"
                                 "private:\n"
                                 "    struct (anonymous namespace)::AnonymousAccessStruct {\n"
                                 "    } privateMember1;\n"
                                 "public:\n"
                                 "    int publicMember2;\n"
                                 "private:\n"
                                 "    enum (anonymous namespace)::AnonymousAccessEnum {\n"
                                 "        AnonymousAccessValue\n"
                                 "    } privateMember2;\n"
                                 "protected:\n"
                                 "    struct (anonymous namespace)::AnonymousAccessStruct {\n"
                                 "    } protectedMember3;\n"
                                 "public:\n"
                                 "    int publicMember3;\n"
                                 "};\n"                        , (*it++).second[0].fullyQualified);
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
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Scoped enum with underlying type", []
        {
            std::string code =
                                "enum class E : unsigned short { A, B, C};\n"
                                "enum class NoExplicitUnderlyingType { A, B, C };\n"
                                "enum struct EnumWithStruct { A, B, C };\n"
                                "enum class UnderlyingInt : int { A, B, C };\n"
                                "enum class UnderlyingUnsignedChar : unsigned char { A, B, C };\n"
                                "using SHORT = short; enum struct EnumShort : SHORT { A, B  };\n"
                                "enum class EnumWithExponentialValues : unsigned short { A = 1, B = 10, C = 100 };\n"
                                "enum class EnumWithExpressionValues : unsigned short { A = 1 << 2, B = A + 3, C };\n"
                                "E e = E::B;\n"
                                "namespace { enum class AnonymousE : unsigned short { A, B, C }; } auto ea = AnonymousE::A;\n"
                                "enum class ENegativeValues : int { A = -1, B = 0, C = 1 };\n"
                                "enum class EUnsignedChar : unsigned char { A = 0, B = 255 };\n"
                                "enum class ForwardDeclaration : unsigned short; enum class ForwardDeclaration : unsigned short { A, B, C };\n"
                                "E e2; E* p; E& r = e2; E array[3];\n"
                                "enum class CodeCoverage { A=1, B=A+1 };\n"

                                "enum { AnonymousA, AnonymousB, AnonymousC };\n"
                                "enum { AnonymousValueA = 1, AnonymousValueB = 10, AnonymousValueC = 100 };\n"
                                "enum : unsigned short { AnonymousUnderlyingA, AnonymousUnderlyingB, AnonymousUnderlyingC };\n"
                                "enum { AnonymousVariableA, AnonymousVariableB, AnonymousVariableC } anonymousEnumVariable;\n"
                                "enum : unsigned short { AnonymousUnderlyingVariableA, AnonymousUnderlyingVariableB, AnonymousUnderlyingVariableC } anonymousEnumVariableWithUnderlyingType;\n"
                                "struct AnonymousEnumStruct { enum { MemberA, MemberB, MemberC }; };\n"
                                "struct AnonymousEnumStructWithValue{ enum { MemberValueA, MemberValueB, MemberValueC } value; };\n"
                                "typedef enum { TypedefA, TypedefB, TypedefC } AnonymousEnumTypedef; AnonymousEnumTypedef anonymousEnumTypedefVar;\n"
                                "namespace { enum { AnonymousNamespaceA, AnonymousNamespaceB, AnonymousNamespaceC }; }\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 9, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(18, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 2, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct AnonymousEnumStruct {\n"
                                 "    enum (unnamed enum at input.cc:21:30) {\n"
                                 "        MemberA,\n"
                                 "        MemberB,\n"
                                 "        MemberC\n"
                                 "    };\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct AnonymousEnumStructWithValue {\n"
                                 "    enum (unnamed enum at input.cc:22:38) {\n"
                                 "        MemberValueA,\n"
                                 "        MemberValueB,\n"
                                 "        MemberValueC\n"
                                 "    } value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:23:9) {\n"
                                 "    TypedefA,\n"
                                 "    TypedefB,\n"
                                 "    TypedefC\n"
                                 "} anonymousEnumTypedefVar;\n"                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:19:1) {\n"
                                 "    AnonymousVariableA,\n"
                                 "    AnonymousVariableB,\n"
                                 "    AnonymousVariableC\n"
                                 "} anonymousEnumVariable;\n"                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:20:1) : unsigned short {\n"
                                 "    AnonymousUnderlyingVariableA,\n"
                                 "    AnonymousUnderlyingVariableB,\n"
                                 "    AnonymousUnderlyingVariableC\n"
                                 "} anonymousEnumVariableWithUnderlyingType;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("E array[3];\n"                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("E e = E::B;\n"                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("E e2;\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class (anonymous namespace)::AnonymousE : unsigned short {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "} ea = AnonymousE::A;\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("E *p;\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("E &r = e2;\n"                                , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum (unnamed enum at input.cc:16:1) {\n"
                                 "    AnonymousA,\n"
                                 "    AnonymousB,\n"
                                 "    AnonymousC\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:17:1) {\n"
                                 "    AnonymousValueA = 1,\n"
                                 "    AnonymousValueB = 10,\n"
                                 "    AnonymousValueC = 100\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);

                Assert::AreEqual("enum (unnamed enum at input.cc:18:1) : unsigned short {\n"
                                 "    AnonymousUnderlyingA,\n"
                                 "    AnonymousUnderlyingB,\n"
                                 "    AnonymousUnderlyingC\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:19:1) {\n"
                                 "    AnonymousVariableA,\n"
                                 "    AnonymousVariableB,\n"
                                 "    AnonymousVariableC\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:20:1) : unsigned short {\n"
                                 "    AnonymousUnderlyingVariableA,\n"
                                 "    AnonymousUnderlyingVariableB,\n"
                                 "    AnonymousUnderlyingVariableC\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum (unnamed enum at input.cc:23:9) {\n"
                                 "    TypedefA,\n"
                                 "    TypedefB,\n"
                                 "    TypedefC\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class CodeCoverage {\n"
                                 "    A = 1,\n"
                                 "    B = 2\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class E : unsigned short {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class ENegativeValues : int {\n"
                                 "    A = -1,\n"
                                 "    B = 0,\n"
                                 "    C = 1\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class EUnsignedChar : unsigned char {\n"
                                 "    A = 0,\n"
                                 "    B = 255\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum struct EnumShort : SHORT {\n"
                                 "    A,\n"
                                 "    B\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class EnumWithExponentialValues : unsigned short {\n"
                                 "    A = 1,\n"
                                 "    B = 10,\n"
                                 "    C = 100\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class EnumWithExpressionValues : unsigned short {\n"
                                 "    A = 1 << 2,\n"
                                 "    B = A + 3,\n"
                                 "    C\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum struct EnumWithStruct {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class ForwardDeclaration : unsigned short {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class NoExplicitUnderlyingType {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class UnderlyingInt : int {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum class UnderlyingUnsignedChar : unsigned char {\n"
                                 "    A,\n"
                                 "    B,\n"
                                 "    C\n"
                                 "};\n"   , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef enum (unnamed enum at input.cc:23:9) {\n"
                                 "            TypedefA,\n"
                                 "            TypedefB,\n"
                                 "            TypedefC\n"
                                 "        } AnonymousEnumTypedef;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using SHORT = short;\n"           , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Inline variables", []
        {
            std::string code =
                                "inline int inlineVariable = 0;\n"
                                "inline constexpr int inlineConstexprVariable = 0;\n"
                                "static inline int staticInlineVariable = 0;\n"
                                "static inline constexpr int staticInlineConstexprVariable = 0;\n"
                                "struct StaticInline { static inline int value = 0; };\n"
                                "struct StaticInlineConstexpr { static inline constexpr int value = 0; };\n"

                                "namespace { enum InlineEnum1 { InlineEnum1A, InlineEnum1B }; } inline InlineEnum1 inlineEnumVariable1 = InlineEnum1A; \n"
                                "namespace { enum InlineEnum2 { InlineEnum2A, InlineEnum2B }; } inline constexpr InlineEnum2 inlineEnumVariable2 = InlineEnum2A; \n"
                                "namespace { enum InlineEnum3 { InlineEnum3A, InlineEnum3B }; } typedef InlineEnum3 InlineEnumTypedef3; \ninline InlineEnumTypedef3 inlineEnumVariable3 = InlineEnum3A;\n"
                                "namespace { enum InlineEnum4 { InlineEnum4A, InlineEnum4B }; } using InlineEnumAlias4 = InlineEnum4; \ninline InlineEnumAlias4 inlineEnumVariable4 = InlineEnum4A; \n"

                                "namespace { struct InlineStruct { int value; }; } inline InlineStruct inlineStructVariable = {};\n"
                                "namespace { struct InlineStructTypedefSource { int value; }; } typedef InlineStructTypedefSource InlineStructTypedef; \ninline InlineStructTypedef inlineStructTypedefVariable = {};\n"
                                "namespace { struct InlineStructAliasSource { int value; }; } using InlineStructAlias = InlineStructAliasSource; \ninline InlineStructAlias inlineStructAliasVariable = {};\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(2, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(9, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct StaticInline {\n"
                                 "    static inline int value = 0;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct StaticInlineConstexpr {\n"
                                 "    static inline constexpr int value = 0;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("inline constexpr int inlineConstexprVariable = 0;\n"     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline enum (anonymous namespace)::InlineEnum1 {\n"
                                 "           InlineEnum1A,\n"
                                 "           InlineEnum1B\n"
                                 "       } inlineEnumVariable1 = InlineEnum1A;\n"          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline constexpr enum (anonymous namespace)::InlineEnum2 {\n"
                                 "                     InlineEnum2A,\n"
                                 "                     InlineEnum2B\n"
                                 "                 } inlineEnumVariable2 = InlineEnum2A;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline enum (anonymous namespace)::InlineEnum3 {\n"
                                 "           InlineEnum3A,\n"
                                 "           InlineEnum3B\n"
                                 "       } inlineEnumVariable3 = InlineEnum3A;\n"          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline enum (anonymous namespace)::InlineEnum4 {\n"
                                 "           InlineEnum4A,\n"
                                 "           InlineEnum4B\n"
                                 "       } inlineEnumVariable4 = InlineEnum4A;\n"          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline struct (anonymous namespace)::InlineStructAliasSource {\n"
                                 "           int value;\n"
                                 "       } inlineStructAliasVariable = {};\n"              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline struct (anonymous namespace)::InlineStructTypedefSource {\n"
                                 "           int value;\n"
                                 "       } inlineStructTypedefVariable = {};\n"            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline struct (anonymous namespace)::InlineStruct {\n"
                                 "           int value;\n"
                                 "       } inlineStructVariable = {};\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int inlineVariable = 0;\n"                        , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("using InlineEnumAlias4 = enum (anonymous namespace)::InlineEnum4 {\n"
                                 "                             InlineEnum4A,\n"
                                 "                             InlineEnum4B\n"
                                 "                         };\n"   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef enum (anonymous namespace)::InlineEnum3 {\n"
                                 "            InlineEnum3A,\n"
                                 "            InlineEnum3B\n"
                                 "        } InlineEnumTypedef3;\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using InlineStructAlias = struct (anonymous namespace)::InlineStructAliasSource {\n"
                                 "                              int value;\n"
                                 "                          };\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::InlineStructTypedefSource {\n"
                                 "            int value;\n"
                                 "        } InlineStructTypedef;\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Thread_local variables", []
        {
            std::string code =
                                "thread_local int threadLocalVariable = 0;\n"
                                "namespace { struct ThreadLocalType {}; } thread_local ThreadLocalType threadLocalVariable2;\n"
                                "static thread_local int staticThreadLocalVariable = 0;\n"
                                "extern thread_local int externThreadLocalVariable;\n"
                                "namespace { enum ThreadLocalEnum {A}; } thread_local ThreadLocalEnum threadLocalVariable3;\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(4, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("extern thread_local int externThreadLocalVariable;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("thread_local int threadLocalVariable = 0;\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("thread_local struct (anonymous namespace)::ThreadLocalType {\n"
                                 "             } threadLocalVariable2;\n"              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("thread_local enum (anonymous namespace)::ThreadLocalEnum {\n"
                                 "                 A\n"
                                 "             } threadLocalVariable3;\n"              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Constinit variables", []
        {
            std::string code =  // N.B. "inline" is never printed by clang's DeclPrinter::print() and the standard gives some leeway. 
                                // Copilot and ChatGPT both suggested just before the type, which is not how people write code.
                                // Claude said after thread_local and before constexpr, which is much better. So I'm going with that.
                                "constinit                     int                  constinitVariable = 0;\n"
                                "constinit        const        int             constinitConstVariable = 0;\n"
                                "constinit static              int            staticConstinitVariable = 0;\n"
                                "constinit inline              int            inlineConstinitVariable = 0;\n"
                                "constinit inline const        int       inlineConstinitConstVariable = 0;\n"
                                "constinit        thread_local int       threadLocalConstinitVariable = 0;\n"
                                "constinit inline thread_local int inlineThreadLocalConstinitVariable = 0;\n"

                                "namespace { struct AnonymousConstinitStruct {}; } constinit AnonymousConstinitStruct constinitAnonymousStruct{};\n"
                                "namespace { enum AnonymousConstinitEnum { AnonymousConstinitValue }; } constinit AnonymousConstinitEnum constinitAnonymousEnum = AnonymousConstinitValue;\n"
                                "namespace { struct AnonymousConstinitConstStruct {}; } constinit const AnonymousConstinitConstStruct constinitAnonymousConstStruct{};\n"
                                "namespace { struct AnonymousConstinitTypedefStruct {}; } typedef AnonymousConstinitTypedefStruct AnonymousConstinitTypedef; constinit AnonymousConstinitTypedef constinitTypedefVariable{};\n"
                                "namespace { struct AnonymousConstinitUsingStruct {}; } using AnonymousConstinitUsing = AnonymousConstinitUsingStruct; constinit AnonymousConstinitUsing constinitUsingVariable{};\n"
                                "namespace { enum AnonymousConstinitTypedefEnum { AnonymousConstinitTypedefValue }; } typedef AnonymousConstinitTypedefEnum AnonymousConstinitEnumTypedef; constinit AnonymousConstinitEnumTypedef constinitEnumTypedefVariable = AnonymousConstinitTypedefValue;\n"
                                "namespace { enum AnonymousConstinitUsingEnum { AnonymousConstinitUsingValue }; } using AnonymousConstinitEnumUsing = AnonymousConstinitUsingEnum; constinit AnonymousConstinitEnumUsing constinitEnumUsingVariable = AnonymousConstinitUsingValue;\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 0, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(11, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("constinit enum (anonymous namespace)::AnonymousConstinitEnum {\n"
                                 "              AnonymousConstinitValue\n"
                                 "          } constinitAnonymousEnum = AnonymousConstinitValue;\n"             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit struct (anonymous namespace)::AnonymousConstinitStruct {\n"
                                 "          } constinitAnonymousStruct{};\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit enum (anonymous namespace)::AnonymousConstinitTypedefEnum {\n"
                                 "              AnonymousConstinitTypedefValue\n"
                                 "          } constinitEnumTypedefVariable = AnonymousConstinitTypedefValue;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit enum (anonymous namespace)::AnonymousConstinitUsingEnum {\n"
                                 "              AnonymousConstinitUsingValue\n"
                                 "          } constinitEnumUsingVariable = AnonymousConstinitUsingValue;\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit struct (anonymous namespace)::AnonymousConstinitTypedefStruct {\n"
                                 "          } constinitTypedefVariable{};\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit struct (anonymous namespace)::AnonymousConstinitUsingStruct {\n"
                                 "          } constinitUsingVariable{};\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit int constinitVariable = 0;\n"                                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit inline const int inlineConstinitConstVariable = 0;\n"              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit inline int inlineConstinitVariable = 0;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit thread_local inline int inlineThreadLocalConstinitVariable = 0;\n" , (*it++).second[0].fullyQualified);
                Assert::AreEqual("constinit thread_local int threadLocalConstinitVariable = 0;\n"              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef enum (anonymous namespace)::AnonymousConstinitTypedefEnum {\n"
                                 "            AnonymousConstinitTypedefValue\n"
                                 "        } AnonymousConstinitEnumTypedef;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousConstinitEnumUsing = enum (anonymous namespace)::AnonymousConstinitUsingEnum {\n"
                                 "                                        AnonymousConstinitUsingValue\n"
                                 "                                    };\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (anonymous namespace)::AnonymousConstinitTypedefStruct {\n"
                                 "        } AnonymousConstinitTypedef;\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using AnonymousConstinitUsing = struct (anonymous namespace)::AnonymousConstinitUsingStruct {\n"
                                 "                                };\n"      , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Constexpr static data members", []
        {
            std::string code =  
                                "struct ConstexprStaticMember { static constexpr int value = 42; };\n"
                                "namespace { struct ConstexprStaticAnonymousType1 {}; } struct ConstexprStaticAnonymousType2 { static constexpr ConstexprStaticAnonymousType1 value{};};\n"
                                "struct ConstexprStaticArray { static constexpr int value[3] = { 1, 2, 3 }; };\n"
                                "constexpr int constexprStaticPointerTarget = 42; struct ConstexprStaticPointer { static constexpr const int* value = &constexprStaticPointerTarget; };\n"
                                "struct ConstexprLiteralType { constexpr ConstexprLiteralType(int value) : value(value) {} int value; }; struct ConstexprStaticLiteral { static constexpr ConstexprLiteralType member{42}; };\n"
                                "struct StaticConstMember { static const int value; };\n"
                                "struct ExplicitInlineConstexpr { inline static constexpr int value = 42; };\n"

                                "namespace { struct ConstexprStaticMemberAnonymousType {}; } struct ConstexprStaticMemberAnonymous { static constexpr ConstexprStaticMemberAnonymousType value{}; };\n"
                                "namespace { enum ConstexprStaticMemberAnonymousEnum { ConstexprStaticMemberAnonymousValue }; } struct ConstexprStaticMemberAnonymousEnumHolder { static constexpr ConstexprStaticMemberAnonymousEnum value = ConstexprStaticMemberAnonymousValue; };\n"
                                "namespace { struct ConstexprStaticNestedAnonymousType {}; } struct ConstexprStaticNestedAnonymousHolder { struct Nested { static constexpr ConstexprStaticNestedAnonymousType value{}; }; };\n"

                                "typedef int ConstexprStaticTypedefInt; struct ConstexprStaticTypedefMember { static constexpr ConstexprStaticTypedefInt value = 42; };\n"
                                "using ConstexprStaticUsingInt = int; struct ConstexprStaticUsingMember { static constexpr ConstexprStaticUsingInt value = 42; };\n"
                                "namespace { struct ConstexprStaticTypedefAnonymousType {}; } typedef ConstexprStaticTypedefAnonymousType ConstexprStaticTypedefAnonymousAlias; struct ConstexprStaticTypedefAnonymousMember { static constexpr ConstexprStaticTypedefAnonymousAlias value{}; };\n"
                                "namespace { struct ConstexprStaticUsingAnonymousType {}; } using ConstexprStaticUsingAnonymousAlias = ConstexprStaticUsingAnonymousType; struct ConstexprStaticUsingAnonymousMember { static constexpr ConstexprStaticUsingAnonymousAlias value{}; };\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(15, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 4, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct ConstexprLiteralType {\n"
                                 "    constexpr ConstexprLiteralType(int value) : value(value) {\n"
                                 "    }\n"
                                 "    int value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticAnonymousType2 {\n"
                                 "    static constexpr struct (anonymous namespace)::ConstexprStaticAnonymousType1 {\n"
                                 "                     } value{};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticArray {\n"
                                 "    static constexpr int value[3] = {1, 2, 3};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticLiteral {\n"
                                 "    static constexpr ConstexprLiteralType member{42};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticMember {\n"
                                 "    static constexpr int value = 42;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticMemberAnonymous {\n"
                                 "    static constexpr struct (anonymous namespace)::ConstexprStaticMemberAnonymousType {\n"
                                 "                     } value{};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticMemberAnonymousEnumHolder {\n"
                                 "    static constexpr enum (anonymous namespace)::ConstexprStaticMemberAnonymousEnum {\n"
                                 "                         ConstexprStaticMemberAnonymousValue\n"
                                 "                     } value = ConstexprStaticMemberAnonymousValue;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticNestedAnonymousHolder {\n"
                                 "    struct Nested {\n"
                                 "        static constexpr struct (anonymous namespace)::ConstexprStaticNestedAnonymousType {\n"
                                 "                         } value{};\n"
                                 "    };\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticPointer {\n"
                                 "    static constexpr const int *value = &constexprStaticPointerTarget;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticTypedefAnonymousMember {\n"
                                 "    static constexpr struct (anonymous namespace)::ConstexprStaticTypedefAnonymousType {\n"
                                 "                     } value{};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticTypedefMember {\n"
                                 "    static constexpr ConstexprStaticTypedefInt value = 42;\n" // TODO: REVIEW: should I inline this typedef?
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticUsingAnonymousMember {\n"
                                 "    static constexpr struct (anonymous namespace)::ConstexprStaticUsingAnonymousType {\n"
                                 "                     } value{};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ConstexprStaticUsingMember {\n"
                                 "    static constexpr ConstexprStaticUsingInt value = 42;\n" // TODO: REVIEW: should I inline this using alias?
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct ExplicitInlineConstexpr {\n"
                                 "    static inline constexpr int value = 42;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct StaticConstMember {\n"
                                 "    static const int value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef struct (anonymous namespace)::ConstexprStaticTypedefAnonymousType {\n"
                                 "        } ConstexprStaticTypedefAnonymousAlias;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef int ConstexprStaticTypedefInt;\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ConstexprStaticUsingAnonymousAlias = struct (anonymous namespace)::ConstexprStaticUsingAnonymousType {\n"
                                 "                                           };\n"  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using ConstexprStaticUsingInt = int;\n"           , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Attributes", []
        {
            std::string code =  
                                "struct NoUniqueAddressTest { struct Empty {}; [[msvc::no_unique_address]] Empty empty; int value; };\n"
                                "[[nodiscard]] int NodiscardFunction() { return 0; }\n"
                                "struct [[deprecated]] DeprecatedType {};\n"
                                "struct [[deprecated(\"use something else\")]] DeprecatedTypeWithMessage {};\n"
                                "struct MaybeUnusedFieldTest { [[maybe_unused]] int value; };\n"
                                "void MaybeUnusedParameterTest([[maybe_unused]] int value) {}\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(4, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(2, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct [[deprecated(\"\")]] DeprecatedType {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct [[deprecated(\"use something else\")]] DeprecatedTypeWithMessage {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct MaybeUnusedFieldTest {\n"
                                 "    [[maybe_unused]] int value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NoUniqueAddressTest {\n"
                                 "    struct Empty {\n"
                                 "    };\n"
                                 "    [[msvc::no_unique_address]] Empty empty;\n"
                                 "    int value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
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
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void MaybeUnusedParameterTest([[maybe_unused]] int value) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("[[nodiscard(\"\")]] int NodiscardFunction() {\n"
                                 "    return 0;\n"
                                 "}\n", (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Friend function templates", []
        {
            std::string code =  
                                "struct FriendFunctionTemplate { template<typename T> friend void f(T); };\n"
                                "struct FriendClassTemplate { template<typename T> friend class F; };\n"
                                "struct FriendFunctionTemplateRequires { template<typename T> requires (sizeof(T) > 0) friend void friendFunctionTemplateRequires(T); };\n"
                                "struct FriendClassTemplateRequires { template<typename T> requires (sizeof(T) > 0) friend class FriendClassTemplateRequiresFriend; };\n"

                                "struct FriendFunction { friend void friendFunction(); };\n"
                                "struct FriendFunctionWithParameter { friend void friendFunctionWithParameter(int); };\n"
                                "struct FriendClass { friend class FriendClassFriend; };\n"
                                "struct FriendStruct { friend struct FriendStructFriend; };\n"
                                "struct FriendUnion { friend union FriendUnionFriend; };\n"

                                "struct FriendFunctionDefinition { friend void friendFunctionDefinition() {} };\n"
                                "struct FriendFunctionWithReturnType { friend int friendFunctionWithReturnType(); };\n"
                                "enum FriendEnumFriend { A }; struct FriendEnum { friend FriendEnumFriend; };\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(12, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 1, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 1, maps.functionMap.size(), "wrong number of functions in map");
            
            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct FriendClass {\n"
                                 "    friend class FriendClassFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplate {\n"
                                 "    template <typename T> friend class F;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateRequires {\n"
                                 "    template <typename T> requires (sizeof(T) > 0) friend class FriendClassTemplateRequiresFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendEnum {\n"
                                 "    friend FriendEnumFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunction {\n"
                                 "    friend void friendFunction();\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunctionDefinition {\n"
                                 "    friend void friendFunctionDefinition() {\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunctionTemplate {\n"
                                 "    template <typename T> friend void f(T);\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunctionTemplateRequires {\n"
                                 "    template <typename T> requires (sizeof(T) > 0) friend void friendFunctionTemplateRequires(T);\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunctionWithParameter {\n"
                                 "    friend void friendFunctionWithParameter(int);\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendFunctionWithReturnType {\n"
                                 "    friend int friendFunctionWithReturnType();\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendStruct {\n"
                                 "    friend struct FriendStructFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendUnion {\n"
                                 "    friend union FriendUnionFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum FriendEnumFriend {\n"
                                 "    A\n"
                                 "};\n", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void friendFunctionDefinition() {\n"
                                "}\n", (*it++).second[0].fullyQualified);
            }
        }
    },
    {"Friend class templates", []
        {
            std::string code =  
                                "template<typename T> class Wrapper; class A { template<typename T> friend class Wrapper; }; template<typename T> class Wrapper { public: T value; };\n"
                                "template<typename T> class FriendClassTemplateForwardDeclared; struct FriendClassTemplateForwardDeclaration { template<typename T> friend class FriendClassTemplateForwardDeclared; }; template<typename T> class FriendClassTemplateForwardDeclared { public: T value; };\n"
                                "template<typename T> class FriendClassTemplateAlreadyDefined { public: T value; }; struct FriendClassTemplateAlreadyDefinedFriend { template<typename T> friend class FriendClassTemplateAlreadyDefined; };\n"
                                "template<typename T, typename U> class FriendClassTemplateTwoParameters; struct FriendClassTemplateTwoParameterFriend { template<typename T, typename U> friend class FriendClassTemplateTwoParameters; }; template<typename T, typename U> class FriendClassTemplateTwoParameters { public: T value1; U value2; };\n"
                                "template<typename T, int N> class FriendClassTemplateNonTypeParameter; struct FriendClassTemplateNonTypeParameterFriend { template<typename T, int N> friend class FriendClassTemplateNonTypeParameter; }; template<typename T, int N> class FriendClassTemplateNonTypeParameter { public: T value[N]; };\n"

                                "template<template<typename> class C> class FriendClassTemplateTemplateParameter; struct FriendClassTemplateTemplateParameterFriend { template<template<typename> class C> friend class FriendClassTemplateTemplateParameter; }; template<template<typename> class C> class FriendClassTemplateTemplateParameter {};\n"
                                "template<typename T, typename U> requires (sizeof(T) > 0 && sizeof(U) > 0) class FriendClassTemplateMultipleRequires; struct FriendClassTemplateMultipleRequiresFriend { template<typename T, typename U> requires (sizeof(T) > 0 && sizeof(U) > 0) friend class FriendClassTemplateMultipleRequires; }; template<typename T, typename U> requires (sizeof(T) > 0 && sizeof(U) > 0) class FriendClassTemplateMultipleRequires {};\n"
                                "template<typename T> struct FriendClassTemplateOuter { template<typename U> friend class FriendClassTemplateNestedFriend; }; template<typename U> class FriendClassTemplateNestedFriend {};\n"
                                "template<typename T> struct FriendClassTemplateOuterWithParameter { template<typename U> friend class FriendClassTemplateOuterWithParameterFriend; }; template<typename U> class FriendClassTemplateOuterWithParameterFriend {};\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(18, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual( 0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("class A {\n"
                                 "    template <typename T> friend class Wrapper;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> class FriendClassTemplateAlreadyDefined {\n"
                                 "public:\n"
                                 "    T value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateAlreadyDefinedFriend {\n"
                                 "    template <typename T> friend class FriendClassTemplateAlreadyDefined;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateForwardDeclaration {\n"
                                 "    template <typename T> friend class FriendClassTemplateForwardDeclared;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> class FriendClassTemplateForwardDeclared {\n"
                                 "public:\n"
                                 "    T value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> requires (sizeof(T) > 0 && sizeof(U) > 0) class FriendClassTemplateMultipleRequires {\n"
                                "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateMultipleRequiresFriend {\n"
                                 "    template <typename T, typename U> requires (sizeof(T) > 0 && sizeof(U) > 0) friend class FriendClassTemplateMultipleRequires;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename U> class FriendClassTemplateNestedFriend {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, int N> class FriendClassTemplateNonTypeParameter {\n"
                                 "public:\n"
                                 "    T value[N];\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateNonTypeParameterFriend {\n"
                                 "    template <typename T, int N> friend class FriendClassTemplateNonTypeParameter;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct FriendClassTemplateOuter {\n"
                                 "    template <typename U> friend class FriendClassTemplateNestedFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct FriendClassTemplateOuterWithParameter {\n"
                                 "    template <typename U> friend class FriendClassTemplateOuterWithParameterFriend;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename U> class FriendClassTemplateOuterWithParameterFriend {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <template <typename> class C> class FriendClassTemplateTemplateParameter {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateTemplateParameterFriend {\n"
                                 "    template <template <typename> class C> friend class FriendClassTemplateTemplateParameter;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct FriendClassTemplateTwoParameterFriend {\n"
                                 "    template <typename T, typename U> friend class FriendClassTemplateTwoParameters;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> class FriendClassTemplateTwoParameters {\n"
                                 "public:\n"
                                 "    T value1;\n"
                                 "    U value2;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> class Wrapper {\n"
                                 "public:\n"
                                 "    T value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
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
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Inline namespaces", []
        {
            std::string code =  
                                "namespace InlineNamespaceTest { inline namespace v1 { struct Type {}; inline int value = 0; } Type object; } int inlineNamespaceValue1 = InlineNamespaceTest::value; int inlineNamespaceValue2 = InlineNamespaceTest::v1::value;\n"
                                "namespace InlineNamespaceExplicit { inline namespace v1 { inline int explicitValue = 0; } int explicitValueAlias = InlineNamespaceExplicit::v1::explicitValue; }\n"
                                "namespace InlineNamespaceNested { inline namespace v1 { namespace details { struct NestedType {}; } } details::NestedType nestedObject; }\n"
                                "namespace InlineNamespaceTypeUse { inline namespace v1 { struct InlineType {}; } InlineType inlineTypeObject; InlineNamespaceTypeUse::InlineType qualifiedInlineTypeObject; }\n"
                                "namespace InlineNamespaceSibling { inline namespace v1 { inline int inlineSiblingValue = 0; } namespace details { inline int nonInlineSiblingValue = 0; } }\n"
                                "namespace InlineNamespaceMultiple { inline namespace v1 { inline int multipleV1Value = 0; } inline namespace v2 { inline int multipleV2Value = 0; } }\n"
                                "namespace InlineNamespaceClassMember { inline namespace v1 { struct InlineMemberType { int inlineMemberValue; }; } InlineMemberType inlineMemberObject; }\n"
                                "namespace InlineNamespaceNestedOuter { namespace Inner { inline namespace v1 { inline int nestedInlineValue = 0; } } }\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual( 4, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(15, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual( 0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 0, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct InlineMemberType {\n"
                                 "    int inlineMemberValue;\n"
                                 "};\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NestedType {\n"
                                 "};\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("InlineNamespaceTest::Type", (*it  ).first);
                Assert::AreEqual("struct Type {\n"
                                 "};\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct InlineType {\n"
                                 "};\n"                     , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("InlineMemberType inlineMemberObject;\n"                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int explicitValue = 0;\n"                                       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("int explicitValueAlias = InlineNamespaceExplicit::v1::explicitValue;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int multipleV1Value = 0;\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int multipleV2Value = 0;\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("details::NestedType nestedObject;\n"                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int nestedInlineValue = 0;\n"                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int nonInlineSiblingValue = 0;\n"                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inline int inlineSiblingValue = 0;\n"                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("InlineNamespaceTest::object"                                           , (*it  ).first);
                Assert::AreEqual("Type object;\n"                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("InlineNamespaceTest::value"                                            , (*it  ).first);
                Assert::AreEqual("inline int value = 0;\n"                                               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("InlineType inlineTypeObject;\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("InlineNamespaceTypeUse::InlineType qualifiedInlineTypeObject;\n"       , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inlineNamespaceValue1"                                                 , (*it  ).first);
                Assert::AreEqual("int inlineNamespaceValue1 = InlineNamespaceTest::value;\n"             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("inlineNamespaceValue2"                                                 , (*it  ).first);
                Assert::AreEqual("int inlineNamespaceValue2 = InlineNamespaceTest::v1::value;\n"         , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
            }
            {
                auto it = maps.typedefMap.begin();
            }
            {
                auto it = maps.conceptMap.begin();
            }
            {
                auto it = maps.functionMap.begin();
            }
        }
    },
    {"Namespace aliases", []
        {
            std::string code =  
                                "namespace OriginalNamespace { struct NamespaceAliasTest {}; } namespace Alias = OriginalNamespace; Alias::NamespaceAliasTest namespaceAliasVariable;\n"
                                "namespace OriginalNamespace { enum NamespaceAliasTestEnum { NamespaceAliasTestRed, NamespaceAliasTestGreen, NamespaceAliasTestBlue }; } namespace Alias = OriginalNamespace; Alias::NamespaceAliasTestEnum namespaceAliasEnumVariable;\n"
                                "struct NamespaceAliasTestStruct { Alias::NamespaceAliasTest namespaceAliasField; };\n"

                                "namespace OriginalNamespace { void namespaceAliasParameterFunction(Alias::NamespaceAliasTest value) {} }\n"
                                "namespace OriginalNamespace { Alias::NamespaceAliasTest namespaceAliasReturnFunction() { return {}; } }\n"
                                "namespace OriginalNamespace { struct NamespaceAliasDerived : Alias::NamespaceAliasTest {}; }\n"
                                "namespace OriginalNamespace { Alias::NamespaceAliasTest* namespaceAliasPointerVariable; }\n"
                                "namespace OriginalNamespace { Alias::NamespaceAliasTest& namespaceAliasReferenceVariable = *namespaceAliasPointerVariable; }\n"
                                "namespace OriginalNamespace { Alias::NamespaceAliasTest namespaceAliasArrayVariable[3]; }\n"
                                "namespace OriginalNamespace { Alias::NamespaceAliasTest namespaceAliasArray2DVariable[2][3]; }\n"
                                "template<typename T> struct NamespaceAliasTemplate { T value; }; NamespaceAliasTemplate<Alias::NamespaceAliasTest> namespaceAliasTemplateVariable;\n"
                                "using NamespaceAliasUsingTest = Alias::NamespaceAliasTest; NamespaceAliasUsingTest namespaceAliasUsingVariable;\n"
                                "typedef Alias::NamespaceAliasTest NamespaceAliasTypedefTest; NamespaceAliasTypedefTest namespaceAliasTypedefVariable;\n"
                                "namespace NestedNamespace { using NestedNamespaceAliasTest = Alias::NamespaceAliasTest; } NestedNamespace::NestedNamespaceAliasTest namespaceAliasNestedVariable;\n"
                                "namespace AliasOfAliasNamespace = Alias; AliasOfAliasNamespace::NamespaceAliasTest namespaceAliasChainedVariable;\n"
                                "template<typename T> void namespaceAliasTemplateFunction(T) {} void namespaceAliasTemplateInstantiation(Alias::NamespaceAliasTest value) {}\n"

                                "namespace AliasTestUsing { using Level1 = Alias::NamespaceAliasTest; using Level2 = Level1; AliasTestUsing::Level2 namespaceAliasUsingTwoLevels; }\n"
                                "namespace AliasTestTypedef { typedef Alias::NamespaceAliasTest Level1; typedef Level1 Level2; Level2 namespaceAliasTypedefTwoLevels; }\n"
                                "using NamespaceAliasMixedLevel1 = Alias::NamespaceAliasTest; typedef NamespaceAliasMixedLevel1 NamespaceAliasMixedLevel2; using NamespaceAliasMixedLevel3 = NamespaceAliasMixedLevel2; NamespaceAliasMixedLevel3 namespaceAliasMixedAliasLevels;\n"
                                "namespace NamespaceAliasUsingOuter { using Level1 = Alias::NamespaceAliasTest; namespace Inner { using Level2 = Level1; } } NamespaceAliasUsingOuter::Inner::Level2 namespaceAliasUsingNestedUsingLevels;\n"
                                "struct NamespaceAliasFieldTest { Alias::NamespaceAliasTest direct; Alias::NamespaceAliasTest* pointer; Alias::NamespaceAliasTest& reference; Alias::NamespaceAliasTest array[2]; };\n"
                                "using NamespaceAliasBaseUsingAlias = Alias::NamespaceAliasTest; struct NamespaceAliasUsingAliasDerived : NamespaceAliasBaseUsingAlias {};\n"
                                "typedef Alias::NamespaceAliasTest NamespaceAliasBaseTypedefAlias; struct NamespaceAliasTypedefDerived : NamespaceAliasBaseTypedefAlias {};\n"
                                "template<typename T> using NamespaceAliasTemplateAlias = T; NamespaceAliasTemplateAlias<Alias::NamespaceAliasTest> namespaceAliasTemplateAliasVariable;\n"
                                "template<typename T> struct NamespaceAliasNestedTemplateAlias { using Type = T; }; NamespaceAliasNestedTemplateAlias<Alias::NamespaceAliasTest>::Type namespaceAliasNestedTemplateAliasVariable;\n"
                                "using NamespaceAliasFunctionUsingAlias = Alias::NamespaceAliasTest; void namespaceAliasUsingAliasParameter(NamespaceAliasFunctionUsingAlias value) {}\n"
                                "typedef Alias::NamespaceAliasTest NamespaceAliasFunctionTypedefAlias; NamespaceAliasFunctionTypedefAlias namespaceAliasTypedefAliasReturn() { return {}; }\n"
                                "namespace AliasLevel1 = Alias; namespace AliasLevel2 = AliasLevel1; AliasLevel2::NamespaceAliasTest namespaceAliasThreeLevelNamespaceChain;\n"
                                "using NamespaceAliasEnumUsingAlias = Alias::NamespaceAliasTestEnum; NamespaceAliasEnumUsingAlias namespaceAliasEnumUsingAliasVariable;\n"
                                "typedef Alias::NamespaceAliasTestEnum NamespaceAliasEnumTypedefAlias; NamespaceAliasEnumTypedefAlias namespaceAliasEnumTypedefAliasVariable;\n"
                                "decltype(Alias::NamespaceAliasTest{}) namespaceAliasDecltypeVariable;\n"
                                "template<typename T> struct NamespaceAliasTemplateTypeArgumentTest {}; NamespaceAliasTemplateTypeArgumentTest<Alias::NamespaceAliasTest*> namespaceAliasTemplatePointerArgument;\n"
                                "template<typename T> struct NamespaceAliasTemplateTypeArgumentReferenceTest {}; NamespaceAliasTemplateTypeArgumentReferenceTest<Alias::NamespaceAliasTest&> namespaceAliasTemplateReferenceArgument;\n"
                                "template<typename T> struct NamespaceAliasOuterTemplate {}; template<typename T> struct NamespaceAliasInnerTemplate {}; NamespaceAliasOuterTemplate<NamespaceAliasInnerTemplate<Alias::NamespaceAliasTest>> namespaceAliasNestedTemplateArgument;\n"
                                "using NamespaceAliasDecltypeUsing = decltype(Alias::NamespaceAliasTest{}); NamespaceAliasDecltypeUsing namespaceAliasDecltypeUsingVariable;\n"
                                "auto namespaceAliasAutoVariable = Alias::NamespaceAliasTest{};\n"
                                "auto namespaceAliasSizeofValue = sizeof(Alias::NamespaceAliasTest);\n"
                                "auto namespaceAliasAlignofValue = alignof(Alias::NamespaceAliasTest);\n"

                                "void namespaceAliasFunction(Alias::NamespaceAliasTest) {}\n"
                                "struct NamespaceAliasConstructorTest { NamespaceAliasConstructorTest(Alias::NamespaceAliasTest) {} };\n"
                                "struct NamespaceAliasDestructorTest { ~NamespaceAliasDestructorTest() {} Alias::NamespaceAliasTest namespaceAliasDestructorField; };\n"
                                "struct NamespaceAliasConversionTest { operator Alias::NamespaceAliasTest() const { return {}; } };\n"
                                "struct NamespaceAliasMethodTest { Alias::NamespaceAliasTest namespaceAliasMethod(Alias::NamespaceAliasTest) { return {}; } };\n"
                                "struct NamespaceAliasConstMethodTest { Alias::NamespaceAliasTest namespaceAliasConstMethod(Alias::NamespaceAliasTest) const { return {}; } };\n"
                                "struct NamespaceAliasRefQualifierMethodTest { Alias::NamespaceAliasTest namespaceAliasRefQualifierMethod(Alias::NamespaceAliasTest) & { return {}; } };\n"
                                "struct NamespaceAliasRValueRefQualifierMethodTest { Alias::NamespaceAliasTest namespaceAliasRValueRefQualifierMethod(Alias::NamespaceAliasTest) && { return {}; } };\n"
                                "struct NamespaceAliasOperatorPlusTest { Alias::NamespaceAliasTest operator+(const Alias::NamespaceAliasTest&) const { return {}; } };\n"
                                "struct NamespaceAliasOperatorCallTest { Alias::NamespaceAliasTest operator()(const Alias::NamespaceAliasTest&) const { return {}; } };\n"
                                "struct NamespaceAliasOperatorSubscriptTest { Alias::NamespaceAliasTest& operator[](const Alias::NamespaceAliasTest&) { static Alias::NamespaceAliasTest value; return value; } };\n"
                                "struct NamespaceAliasOperatorEqualTest { bool operator==(const Alias::NamespaceAliasTest&) const { return true; } };\n"
                                "struct NamespaceAliasOperatorArrowTest { Alias::NamespaceAliasTest* operator->() { return nullptr; } };\n"
                                "template<typename T> Alias::NamespaceAliasTest namespaceAliasFunctionTemplate(T) { return {}; }\n"
                                "template<typename T> Alias::NamespaceAliasTest namespaceAliasFunctionTemplateWithAlias(Alias::NamespaceAliasTest, T) { return {}; }\n"
                                "template<typename T> requires (sizeof(T) > 0) Alias::NamespaceAliasTest namespaceAliasConstrainedFunctionTemplate(T) { return {}; }\n"
                                "template<> Alias::NamespaceAliasTest namespaceAliasFunctionTemplate<Alias::NamespaceAliasTest>(Alias::NamespaceAliasTest) { return {}; }\n"
                                "template<typename T> struct NamespaceAliasClassTemplate { Alias::NamespaceAliasTest value; };\n"
                                "template<typename T> struct NamespaceAliasClassTemplateWithAliasBase : T { Alias::NamespaceAliasTest value; };\n"
                                "template<typename T> struct NamespaceAliasClassTemplateSpecializationTest { Alias::NamespaceAliasTest value; }; template<> struct NamespaceAliasClassTemplateSpecializationTest<Alias::NamespaceAliasTest> { Alias::NamespaceAliasTest value; };\n"
                                "template<typename T> struct NamespaceAliasPartialClassTemplate { Alias::NamespaceAliasTest value; }; template<typename T> struct NamespaceAliasPartialClassTemplate<T*> { Alias::NamespaceAliasTest value; };\n"
                                "template<typename T> Alias::NamespaceAliasTest namespaceAliasVariableTemplate = {};\n"
                                "template<typename T> Alias::NamespaceAliasTest namespaceAliasVariableTemplateSpecializationTest = {}; template<> Alias::NamespaceAliasTest namespaceAliasVariableTemplateSpecializationTest<Alias::NamespaceAliasTest> = {};\n"
                                "using NamespaceAliasUsingTest2 = NamespaceAliasUsingTest;\n"
                                "typedef NamespaceAliasUsingTest NamespaceAliasTypedefFromUsingTest;\n"
                                "template<typename T> using NamespaceAliasAliasTemplate = Alias::NamespaceAliasTest;\n"
                                "template<typename T> using NamespaceAliasDependentAliasTemplate = T;\n"
                                "struct NamespaceAliasFieldTest2 { Alias::NamespaceAliasTest namespaceAliasField; };\n"
                                "struct NamespaceAliasConstFieldTest { const Alias::NamespaceAliasTest namespaceAliasConstField; };\n"
                                "struct NamespaceAliasPointerFieldTest { Alias::NamespaceAliasTest* namespaceAliasPointerField; };\n"
                                "struct NamespaceAliasReferenceFieldTest { Alias::NamespaceAliasTest& namespaceAliasReferenceField; };\n"
                                "struct NamespaceAliasArrayFieldTest { Alias::NamespaceAliasTest namespaceAliasArrayField[3]; };\n"
                                "void namespaceAliasParameterTest(Alias::NamespaceAliasTest namespaceAliasParameter) {}\n"
                                "void namespaceAliasPointerParameterTest(Alias::NamespaceAliasTest* namespaceAliasPointerParameter) {}\n"
                                "void namespaceAliasReferenceParameterTest(const Alias::NamespaceAliasTest& namespaceAliasReferenceParameter) {}\n"
                                "void namespaceAliasArrayParameterTest(Alias::NamespaceAliasTest namespaceAliasArrayParameter[3]) {}\n"
                                "enum NamespaceAliasValueEnumTest { NamespaceAliasValueEnumTestA, NamespaceAliasValueEnumTestB };\n"
                                "struct NamespaceAliasStructTest { Alias::NamespaceAliasTest value; };\n"
                                "class NamespaceAliasClassTest { Alias::NamespaceAliasTest value; };\n"
                                "union NamespaceAliasUnionTest { Alias::NamespaceAliasTest value; };\n"
                                "enum NamespaceAliasEnumTest { NamespaceAliasEnumTestA, NamespaceAliasEnumTestB };\n"
                                "struct NamespaceAliasNestedTypeTest { struct Nested { Alias::NamespaceAliasTest value; }; };\n"
                                "struct NamespaceAliasNestedEnumTest { enum Nested { NamespaceAliasNestedEnumA, NamespaceAliasNestedEnumB }; Alias::NamespaceAliasTest value; };\n"
                                "struct NamespaceAliasNestedAliasTest { using NestedAlias = Alias::NamespaceAliasTest; };\n"
                                "template<typename T> concept NamespaceAliasConceptTest = requires { sizeof(Alias::NamespaceAliasTest); };\n"
                                "template<typename T> concept NamespaceAliasConceptTypeTest = __is_same(T, Alias::NamespaceAliasTest);\n"
                                "template<typename NamespaceAliasTemplateTypeParameterTest> struct NamespaceAliasTemplateTypeParameterHolder {};\n"
                                "template<Alias::NamespaceAliasTest* NamespaceAliasNonTypeParameterTest> struct NamespaceAliasNonTypeParameterHolder {};\n"
                                "struct NamespaceAliasFriendTest { friend void namespaceAliasFriend(Alias::NamespaceAliasTest); };\n"
                                "struct NamespaceAliasFriendClassTest { friend struct NamespaceAliasFriendClass; Alias::NamespaceAliasTest value; };\n"
                                "struct NamespaceAliasFriendTemplateTest { template<typename T> friend void namespaceAliasFriendTemplate(Alias::NamespaceAliasTest, T); };\n"
                                "namespace NamespaceAliasUsingDeclarationNamespace { using Alias::NamespaceAliasTest; }\n"
                                "namespace NamespaceAliasUsingDeclarationNamespace2 { using Alias::NamespaceAliasTestEnum; }\n"
                                "namespace NamespaceAliasUsingFunctionSource { void namespaceAliasUsingFunction(Alias::NamespaceAliasTest) {} } using NamespaceAliasUsingFunctionSource::namespaceAliasUsingFunction;\n"
                                "namespace NamespaceAliasSourceTest {} namespace NamespaceAliasTargetTest = NamespaceAliasSourceTest;\n"
                                "namespace OriginalNamespace { using NamespaceAliasInt = unsigned int; } namespace Alias = OriginalNamespace; struct NamespaceAliasBitFieldTest { Alias::NamespaceAliasInt value : 3; };\n"
                                "struct NamespaceAliasFieldInitializerTest { Alias::NamespaceAliasTest value{}; };\n"
                                "void namespaceAliasDefaultArgumentTest(Alias::NamespaceAliasTest value = Alias::NamespaceAliasTest{}) {}\n"
                                "void namespaceAliasDefaultReferenceTest(const Alias::NamespaceAliasTest& value = Alias::NamespaceAliasTest{}) {}\n"
                                "struct NamespaceAliasStaticFieldTest { static Alias::NamespaceAliasTest value; }; Alias::NamespaceAliasTest NamespaceAliasStaticFieldTest::value{};\n"
                                "struct NamespaceAliasInlineStaticFieldTest { inline static Alias::NamespaceAliasTest value{}; };\n"
                                "struct NamespaceAliasConstexprStaticFieldTest { static constexpr Alias::NamespaceAliasTest* value = nullptr; };\n"
                                "struct NamespaceAliasVolatileFieldTest { volatile Alias::NamespaceAliasTest* value; };\n"
                                "struct NamespaceAliasMethodTemplateTest { template<typename T> Alias::NamespaceAliasTest method(T) { return {}; } };\n"
                                "struct NamespaceAliasStaticMethodTest { static Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) { return {}; } };\n"
                                "struct NamespaceAliasVirtualMethodTest { virtual Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) { return {}; } };\n"
                                "struct NamespaceAliasPureVirtualMethodTest { virtual Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) = 0; };\n"
                                "struct NamespaceAliasOverrideMethodTestBase { virtual Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) { return {}; } }; struct NamespaceAliasOverrideMethodTest : NamespaceAliasOverrideMethodTestBase { Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) override { return {}; } };\n"
                                "struct NamespaceAliasFinalMethodTestBase { virtual Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) { return {}; } }; struct NamespaceAliasFinalMethodTest : NamespaceAliasFinalMethodTestBase { Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) final { return {}; } };\n"
                                "struct NamespaceAliasDeletedMethodTest { Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) = delete; };\n"
                                "struct NamespaceAliasDefaultedConstructorTest { NamespaceAliasDefaultedConstructorTest() = default; NamespaceAliasDefaultedConstructorTest(Alias::NamespaceAliasTest value) : value(value) {} Alias::NamespaceAliasTest value; };\n"
                                "struct NamespaceAliasExplicitConstructorTest { explicit NamespaceAliasExplicitConstructorTest(Alias::NamespaceAliasTest) {} };\n"
                                "struct NamespaceAliasNoexceptMethodTest { Alias::NamespaceAliasTest method(Alias::NamespaceAliasTest) noexcept { return {}; } };\n"
                                "struct NamespaceAliasTrailingReturnTest { auto method(Alias::NamespaceAliasTest) -> Alias::NamespaceAliasTest { return {}; } };\n"

                                "template<typename T> concept NamespaceAliasConceptTypeRequirementTest = requires { typename Alias::NamespaceAliasTest; };\n"
                                "template<typename T> concept NamespaceAliasConceptParameterTest = requires (Alias::NamespaceAliasTest value) { sizeof(value); };\n"
                                "template<typename T> concept NamespaceAliasConceptNestedRequirementTest = requires { requires (sizeof(Alias::NamespaceAliasTest) > 0); };\n"
                                "template<typename T> concept NamespaceAliasConceptCompoundRequirementTest = requires { { Alias::NamespaceAliasTest{} }; };\n"
                                //"template<typename T> concept NamespaceAliasConceptSizeTest = (sizeof(Alias::NamespaceAliasTest) > 0);\n"
                                //"template<typename T> concept NamespaceAliasConceptAlignmentTest = (alignof(Alias::NamespaceAliasTest) > 0);\n"
                                //"template<typename T> concept NamespaceAliasConceptCombinedConstraintTest = (__is_same(T, Alias::NamespaceAliasTest) && sizeof(Alias::NamespaceAliasTest) > 0);\n"
                                //"template<typename T> requires (__is_same(T, Alias::NamespaceAliasTest)) void namespaceAliasRequiresClauseFunctionTest(T) {}\n"
                                //"template<typename T> requires requires { typename Alias::NamespaceAliasTest; } void namespaceAliasRequiresExpressionClauseFunctionTest(T) {}\n"
                                //"template<typename T> concept NamespaceAliasConceptHelperTest = __is_same(T, Alias::NamespaceAliasTest);\n"
                                //"template<typename T> requires NamespaceAliasConceptHelperTest<T> void namespaceAliasConceptIdConstraintTest(T) {}\n"
                                //"template<typename T> requires (NamespaceAliasConceptHelperTest<T> && sizeof(Alias::NamespaceAliasTest) > 0) void namespaceAliasCombinedConceptConstraintTest(T) {}\n"
                                //"template<typename T> concept NamespaceAliasConceptNestedConceptTest = requires { requires NamespaceAliasConceptHelperTest<Alias::NamespaceAliasTest>; };\n"
                                //"template<Alias::NamespaceAliasInt N> concept NamespaceAliasNonTypeConceptParameterTest = (N > 0);\n"
                                //"template<typename T> concept NamespaceAliasConceptTemplateArgumentTest = NamespaceAliasConceptHelperTest<Alias::NamespaceAliasTest>;\n"
                                //"template<typename T> concept NamespaceAliasConceptRequiresExpressionConceptArgumentTest = requires { requires NamespaceAliasConceptHelperTest<Alias::NamespaceAliasTest>; };\n"
                                //"namespace NamespaceAliasConceptUsingDeclarationTest { using Alias::NamespaceAliasTest; NamespaceAliasTest value; }\n"
                                    ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(65, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(32, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual( 3, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(25, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual( 6, maps.conceptMap.size(), "wrong number of comcepts in map");
            Assert::AreEqual(18, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct NamespaceAliasArrayFieldTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasArrayField[3];\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasBitFieldTest {\n"
                                 "    OriginalNamespace::NamespaceAliasInt value : 3;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasClassTemplate {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasClassTemplateSpecializationTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> struct NamespaceAliasClassTemplateSpecializationTest<OriginalNamespace::NamespaceAliasTest> {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasClassTemplateWithAliasBase : T {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("class NamespaceAliasClassTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasConstFieldTest {\n"
                                 "    const OriginalNamespace::NamespaceAliasTest namespaceAliasConstField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasConstMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasConstMethod(OriginalNamespace::NamespaceAliasTest) const {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasConstexprStaticFieldTest {\n"
                                 "    static constexpr OriginalNamespace::NamespaceAliasTest *value = nullptr;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasConstructorTest {\n"
                                 "    NamespaceAliasConstructorTest(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasConversionTest {\n"
                                 "    operator OriginalNamespace::NamespaceAliasTest() const {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasDefaultedConstructorTest {\n"
                                 "    NamespaceAliasDefaultedConstructorTest() = default;\n"
                                 "    NamespaceAliasDefaultedConstructorTest(OriginalNamespace::NamespaceAliasTest value) : value(value) {\n"
                                 "    }\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasDeletedMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) = delete;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasDestructorTest {\n"
                                 "    ~NamespaceAliasDestructorTest() noexcept {\n"
                                 "    }\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasDestructorField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasExplicitConstructorTest {\n"
                                 "    explicit NamespaceAliasExplicitConstructorTest(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFieldInitializerTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value {};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFieldTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest direct;\n"
                                 "    OriginalNamespace::NamespaceAliasTest *pointer;\n"
                                 "    OriginalNamespace::NamespaceAliasTest &reference;\n"
                                 "    OriginalNamespace::NamespaceAliasTest array[2];\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFieldTest2 {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFinalMethodTest : NamespaceAliasFinalMethodTestBase {\n"
                                 "    OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) final {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFinalMethodTestBase {\n"
                                 "    virtual OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFriendClassTest {\n"
                                 "    friend struct NamespaceAliasFriendClass;\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFriendTemplateTest {\n"
                                 "    template <typename T> friend void namespaceAliasFriendTemplate(OriginalNamespace::NamespaceAliasTest, T);\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasFriendTest {\n"
                                 "    friend void namespaceAliasFriend(OriginalNamespace::NamespaceAliasTest);\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasInlineStaticFieldTest {\n"
                                 "    static inline OriginalNamespace::NamespaceAliasTest value{};\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasInnerTemplate {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasMethodTemplateTest {\n"
                                 "    template <typename T> OriginalNamespace::NamespaceAliasTest method(T) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasMethod(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasNestedAliasTest {\n"
                                 "    using NestedAlias = Alias::NamespaceAliasTest;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasNestedEnumTest {\n"
                                 "    enum Nested {\n"
                                 "        NamespaceAliasNestedEnumA,\n"
                                 "        NamespaceAliasNestedEnumB\n"
                                 "    };\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasNestedTemplateAlias {\n"
                                 "    using Type = T;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasNestedTypeTest {\n"
                                 "    struct Nested {\n"
                                 "        OriginalNamespace::NamespaceAliasTest value;\n"
                                 "    };\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasNoexceptMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) noexcept {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <OriginalNamespace::NamespaceAliasTest *NamespaceAliasNonTypeParameterTest> struct NamespaceAliasNonTypeParameterHolder {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOperatorArrowTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest *operator->() {\n"
                                 "        return nullptr;\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOperatorCallTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest operator()(const OriginalNamespace::NamespaceAliasTest &) const {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOperatorEqualTest {\n"
                                 "    bool operator==(const OriginalNamespace::NamespaceAliasTest &) const {\n"
                                 "        return true;\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOperatorPlusTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest operator+(const OriginalNamespace::NamespaceAliasTest &) const {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOperatorSubscriptTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest &operator[](const OriginalNamespace::NamespaceAliasTest &) {\n"
                                 "        static OriginalNamespace::NamespaceAliasTest value;\n"
                                 "        return value;\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasOuterTemplate {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOverrideMethodTest : NamespaceAliasOverrideMethodTestBase {\n"
                                 "    OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) override {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasOverrideMethodTestBase {\n"
                                 "    virtual OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasPartialClassTemplate {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasPartialClassTemplate<T *> {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasPointerFieldTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest *namespaceAliasPointerField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasPureVirtualMethodTest {\n"
                                 "    virtual OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) = 0;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasRValueRefQualifierMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasRValueRefQualifierMethod(OriginalNamespace::NamespaceAliasTest) && {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasRefQualifierMethodTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasRefQualifierMethod(OriginalNamespace::NamespaceAliasTest) & {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasReferenceFieldTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest &namespaceAliasReferenceField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasStaticFieldTest {\n"
                                 "    static OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasStaticMethodTest {\n"
                                 "    static OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasStructTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasTemplate {\n"
                                 "    T value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasTemplateTypeArgumentReferenceTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> struct NamespaceAliasTemplateTypeArgumentTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename NamespaceAliasTemplateTypeParameterTest> struct NamespaceAliasTemplateTypeParameterHolder {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasTestStruct {\n"
                                 "    OriginalNamespace::NamespaceAliasTest namespaceAliasField;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasTrailingReturnTest {\n"
                                 "    auto method(OriginalNamespace::NamespaceAliasTest) -> OriginalNamespace::NamespaceAliasTest {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasTypedefDerived : OriginalNamespace::NamespaceAliasTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("union NamespaceAliasUnionTest {\n"
                                 "    OriginalNamespace::NamespaceAliasTest value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasUsingAliasDerived : OriginalNamespace::NamespaceAliasTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasVirtualMethodTest {\n"
                                 "    virtual OriginalNamespace::NamespaceAliasTest method(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "        return {};\n"
                                 "    }\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasVolatileFieldTest {\n"
                                 "    volatile OriginalNamespace::NamespaceAliasTest *value;\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasDerived : OriginalNamespace::NamespaceAliasTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("struct NamespaceAliasTest {\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasTypedefTwoLevels;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasUsingTwoLevels;\n"                                                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest NamespaceAliasStaticFieldTest::value{};\n"                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasArray2DVariable[2][3];\n"                                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasArrayVariable[3];\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest *namespaceAliasPointerVariable;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest &namespaceAliasReferenceVariable = *namespaceAliasPointerVariable;\n"                             , (*it++).second[0].fullyQualified);
                Assert::AreEqual("unsigned long long namespaceAliasAlignofValue = alignof(OriginalNamespace::NamespaceAliasTest);\n"                                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasAutoVariable = OriginalNamespace::NamespaceAliasTest{};\n"                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasChainedVariable;\n"                                                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasDecltypeUsingVariable;\n"                                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasDecltypeVariable;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTestEnum namespaceAliasEnumTypedefAliasVariable;\n"                                                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTestEnum namespaceAliasEnumUsingAliasVariable;\n"                                                      , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTestEnum namespaceAliasEnumVariable;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasMixedAliasLevels;\n"                                                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasNestedTemplateAliasVariable;\n"                                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("NamespaceAliasOuterTemplate<NamespaceAliasInnerTemplate<OriginalNamespace::NamespaceAliasTest>> namespaceAliasNestedTemplateArgument;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasNestedVariable;\n"                                                                  , (*it++).second[0].fullyQualified);
                Assert::AreEqual("unsigned long long namespaceAliasSizeofValue = sizeof(OriginalNamespace::NamespaceAliasTest);\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasTemplateAliasVariable;\n"                                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("NamespaceAliasTemplateTypeArgumentTest<OriginalNamespace::NamespaceAliasTest *> namespaceAliasTemplatePointerArgument;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("NamespaceAliasTemplateTypeArgumentReferenceTest<OriginalNamespace::NamespaceAliasTest &> namespaceAliasTemplateReferenceArgument;\n"    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("NamespaceAliasTemplate<OriginalNamespace::NamespaceAliasTest> namespaceAliasTemplateVariable;\n"                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasThreeLevelNamespaceChain;\n"                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasTypedefVariable;\n"                                                                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasUsingNestedUsingLevels;\n"                                                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasUsingVariable;\n"                                                                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasVariable;\n"                                                                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> OriginalNamespace::NamespaceAliasTest namespaceAliasVariableTemplate = {};\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> OriginalNamespace::NamespaceAliasTest namespaceAliasVariableTemplateSpecializationTest = {};\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> OriginalNamespace::NamespaceAliasTest namespaceAliasVariableTemplateSpecializationTest<OriginalNamespace::NamespaceAliasTest> = {};\n"   , (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.enumMap.begin();
                Assert::AreEqual("enum NamespaceAliasEnumTest {\n"
                                 "    NamespaceAliasEnumTestA,\n"
                                 "    NamespaceAliasEnumTestB\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum NamespaceAliasValueEnumTest {\n"
                                 "    NamespaceAliasValueEnumTestA,\n"
                                 "    NamespaceAliasValueEnumTestB\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("enum NamespaceAliasTestEnum {\n"
                                 "    NamespaceAliasTestRed,\n"
                                 "    NamespaceAliasTestGreen,\n"
                                 "    NamespaceAliasTestBlue\n"
                                 "};\n", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.typedefMap.begin();
                Assert::AreEqual("typedef Alias::NamespaceAliasTest Level1;\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Level1 Level2;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Level1 = Alias::NamespaceAliasTest;\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Level2 = Level1;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using NamespaceAliasAliasTemplate = Alias::NamespaceAliasTest;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Alias::NamespaceAliasTest NamespaceAliasBaseTypedefAlias;\n"                   , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasBaseUsingAlias = Alias::NamespaceAliasTest;\n"                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasDecltypeUsing = decltype(Alias::NamespaceAliasTest{});\n"          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using NamespaceAliasDependentAliasTemplate = T;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Alias::NamespaceAliasTestEnum NamespaceAliasEnumTypedefAlias;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasEnumUsingAlias = Alias::NamespaceAliasTestEnum;\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Alias::NamespaceAliasTest NamespaceAliasFunctionTypedefAlias;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasFunctionUsingAlias = Alias::NamespaceAliasTest;\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasMixedLevel1 = Alias::NamespaceAliasTest;\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef NamespaceAliasMixedLevel1 NamespaceAliasMixedLevel2;\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasMixedLevel3 = NamespaceAliasMixedLevel2;\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> using NamespaceAliasTemplateAlias = T;\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef NamespaceAliasUsingTest NamespaceAliasTypedefFromUsingTest;\n"                 , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef Alias::NamespaceAliasTest NamespaceAliasTypedefTest;\n"                        , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Level2 = Level1;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using Level1 = Alias::NamespaceAliasTest;\n"                                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasUsingTest = Alias::NamespaceAliasTest;\n"                          , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasUsingTest2 = NamespaceAliasUsingTest;\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NestedNamespaceAliasTest = Alias::NamespaceAliasTest;\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using NamespaceAliasInt = unsigned int;\n"                                             , (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.conceptMap.begin();
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptCompoundRequirementTest = requires { { OriginalNamespace::NamespaceAliasTest{} }; };\n"                           , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptNestedRequirementTest = requires { requires (sizeof(OriginalNamespace::NamespaceAliasTest) > 0); };\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptParameterTest = requires (Alias::NamespaceAliasTest value) { sizeof (value); };\n"                    , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptTest = requires { sizeof(OriginalNamespace::NamespaceAliasTest); };\n"                                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptTypeRequirementTest = requires { typename OriginalNamespace::NamespaceAliasTest; };\n"                , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept NamespaceAliasConceptTypeTest = __is_same(T, OriginalNamespace::NamespaceAliasTest);\n"                                    , (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.functionMap.begin();
                Assert::AreEqual("void namespaceAliasUsingFunction(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasParameterFunction(OriginalNamespace::NamespaceAliasTest value) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasReturnFunction() {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasArrayParameterTest(OriginalNamespace::NamespaceAliasTest namespaceAliasArrayParameter[3]) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> requires (sizeof(T) > 0) OriginalNamespace::NamespaceAliasTest namespaceAliasConstrainedFunctionTemplate(T) {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasDefaultArgumentTest(OriginalNamespace::NamespaceAliasTest value = OriginalNamespace::NamespaceAliasTest{}) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasDefaultReferenceTest(const OriginalNamespace::NamespaceAliasTest &value = OriginalNamespace::NamespaceAliasTest{}) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasFunction(OriginalNamespace::NamespaceAliasTest) {\n"
                                "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template<> OriginalNamespace::NamespaceAliasTest namespaceAliasFunctionTemplate<OriginalNamespace::NamespaceAliasTest>(OriginalNamespace::NamespaceAliasTest) {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> OriginalNamespace::NamespaceAliasTest namespaceAliasFunctionTemplate(T) {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> OriginalNamespace::NamespaceAliasTest namespaceAliasFunctionTemplateWithAlias(OriginalNamespace::NamespaceAliasTest, T) {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasParameterTest(OriginalNamespace::NamespaceAliasTest namespaceAliasParameter) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasPointerParameterTest(OriginalNamespace::NamespaceAliasTest *namespaceAliasPointerParameter) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasReferenceParameterTest(const OriginalNamespace::NamespaceAliasTest &namespaceAliasReferenceParameter) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> void namespaceAliasTemplateFunction(T) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasTemplateInstantiation(OriginalNamespace::NamespaceAliasTest value) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("OriginalNamespace::NamespaceAliasTest namespaceAliasTypedefAliasReturn() {\n"
                                 "    return {};\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("void namespaceAliasUsingAliasParameter(OriginalNamespace::NamespaceAliasTest value) {\n"
                                 "}\n", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
                //Assert::AreEqual("boo", (*it++).second[0].fullyQualified);
            }
        }
    },

};
/* some missing test cases

14. Deduction guides
            A(int)->A<int>;


/////////////////////////////////////////////////////////////////////////// namespace

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