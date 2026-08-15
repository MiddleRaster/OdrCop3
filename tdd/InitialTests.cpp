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
                               ;

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            Assert::AreEqual(1, maps.udtMap.size(), "wrong number of UDTs in map");
            Assert::AreEqual(0, maps.varMap.size(),  "wrong number of vars in map");
            Assert::AreEqual(0, maps.enumMap.size(),  "wrong number of enums in map");
            Assert::AreEqual(0, maps.typedefMap.size(),"wrong number of typedefs in map");
            Assert::AreEqual(0, maps.conceptMap.size(), "wrong number of concepts in map");
            Assert::AreEqual(6, maps.functionMap.size(), "wrong number of functions in map");

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
                             "    inline static int y{0};\n" // Decl::print() improperly drops the "inline"; my serializer is more correct than print()
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
                Assert::AreEqual("template <typename T> T GlobalValue{};\n"                            , (*it++).second[0].fullyQualified);
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
                                 "    friend void f(B &) {\n"
                                 "    }\n"
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
                                 "                   }[10]);\n"
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
                Assert::AreEqual("template <enum class (anonymous namespace)::Mode : int {\n"
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
                Assert::AreEqual("static const int Outer::Inner::counter = 0;\n", (*it++).second[0].fullyQualified);
                Assert::AreEqual("static int Soo::counter = 0;\n"               , (*it++).second[0].fullyQualified);
                Assert::AreEqual("static const char *Soo::name = \"soo\";\n"    , (*it++).second[0].fullyQualified);
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
                                 "    inline static LambdaHolder::(lambda at input.cc:5:54) LambdaField{[](int, double) {\n"
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
                Assert::AreEqual("void FooWithAnonArg(typedef enum A::B::C::(anonymous namespace)::EInvisible {\n"
                                 "                                Zero\n"
                                 "                            } MyInvisible) {\n"
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
                Assert::AreEqual("enum class Color : int {\n    Red,\n    Green\n};\n", (*it++).second[0].fullyQualified);
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
                Assert::AreEqual("UnnamedStructAlias<typedef enum (unnamed enum at input.cc:27:9) {\n"
                                 "                               One,\n"
                                 "                               Two\n"
                                 "                           } UnnamedEnum> GlobalVariableUnnamedEnum;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("UnnamedStructAlias<typedef struct (unnamed struct at input.cc:25:9) {\n"
                                 "                               int x;\n"
                                 "                           } UnnamedStruct> GlobalVariableUnnamedStruct;\n"
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
                Assert::AreEqual("using F = typedef struct (unnamed struct at input.cc:24:9) {\n"
                                 "                      int x;\n"
                                 "                  } E;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("typedef struct (unnamed struct at input.cc:24:9) {\n"
                                 "            int x;\n"
                                 "        } G;\n"
                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("using H = typedef struct (unnamed struct at input.cc:24:9) {\n"
                                 "                      int x;\n"
                                 "                  } G;\n"
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

//template<typename T> concept C = sizeof(T) == 4; template<C T> void f(T);
//template<typename T> requires C<T> void g(T);
//template<typename T> requires (C<T> && sizeof(T) > 1) void h(T);

//template<C T> void f(T x);
//void g(C auto x);
//void h(C auto&& x);

//template<typename T> concept HasFoo = requires(T t) { { t.foo() } noexcept -> std::same_as<int>; };
//template<typename T> concept Valid = HasFoo<T> && requires { typename T::value_type; } && sizeof(T) == 4;
//template<typename T> requires Valid<T> struct S { T value; template<typename U> requires HasFoo<U> void f(U u) { u.foo(); } };
                               ;
            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);

            //Assert::AreEqual(13, maps.udtMap.size(), "wrong number of UDTs in map");
            //Assert::AreEqual( 5, maps.varMap.size(),  "wrong number of vars in map");
            //Assert::AreEqual( 1, maps.enumMap.size(),  "wrong number of enums in map");
            //Assert::AreEqual(11, maps.typedefMap.size(),"wrong number of typedefs in map");
            //Assert::AreEqual(27, maps.conceptMap.size(), "wrong number of comcepts in map");
            //Assert::AreEqual( 0, maps.functionMap.size(), "wrong number of functions in map");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("struct X18 {\n"
                                 "    static int value;\n"
                                 "};\n"
                              , (*it++).second[0].fullyQualified);
            }
            {
                auto it = maps.varMap.begin();
                Assert::AreEqual("template <typename T> constexpr bool is_integral_v = false;\n"         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> constexpr bool is_pointer_v = __is_pointer(T);\n", (*it++).second[0].fullyQualified);
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
            {
                auto it = maps.conceptMap.begin();
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
                Assert::AreEqual("template <typename T, typename U> concept HasMember = requires (T t, U u) { t.foo(u); };\n"                         , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasNestedType = requires { typename T::nested::type; };\n"                            , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept HasValueType = requires { typename T::value_type; };\n"                               , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template <typename T> concept C19 = sizeof(T) == 4;\n"                                                              , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T> concept D19 = C19<T> && requires (T t) { t.foo(); };\n"                                       , (*it++).second[0].fullyQualified);

                Assert::AreEqual("template <typename T, typename U> concept SameSize = sizeof(T) == sizeof(U);\n"                                     , (*it++).second[0].fullyQualified);
                Assert::AreEqual("template <typename T, typename U> concept same_as = __is_same(T, U) && __is_same(U, T);\n"                          , (*it++).second[0].fullyQualified);

            }
        }
    },
};
/* some missing test cases

7. Concepts
            template<typename T> concept C = sizeof(T) == 4;
         and
            template<C T> void f(); 

10. Constrained auto
            C auto x = ...


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