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
                               "struct S {}; struct A { S* p; const S& q; S r[2]; const Foo* a; Foo& b; Foo c[2]; Foo**& d; void (*callback)(S*); void (*callback2)(Foo*, int, double); };";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(3, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found an enum entry");

            {
                auto it = maps.udtMap.begin();
                Assert::AreEqual("A",        it->first,        "should have gotten correct key");
                Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

                Assert::AreEqual("struct A { // sizeof=80\n"
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

    /*

    4. Function types

    struct S {};
    struct A
    {
        void (*callback)(S*);
    };

    The graph:

    FieldDecl
     |
     PointerType
     |
     FunctionProtoType
           |
           +-- return type
           |
           +-- parameter type


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

    Exercises:

    MemberPointerType

    A lot of serializers forget this one.

    */


};
