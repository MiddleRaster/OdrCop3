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
                               "struct Foo : Bar"
                               "{"
                               "   struct { int x;  };"
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
                                "   struct  { // sizeof=4\n"
                                "   int x;\n"
                                "};\n"
                                "   int i;\n"
                                "   explicit void __cdecl Foo(int i) : x(7), i(i) { i++; }\n"
                                "   [[nodiscard]] const Bar * __cdecl GetBar() const & override { return this; }\n"
                                "   auto __cdecl make_lambda() const {\n"
                                "    return [this](int x) {\n"
                                "        return x + i;\n"
                                "    };\n"
                                "}\n"
                                "   template <typename T> T __cdecl doTemplateyStuff(const T & value) requires requires { typename T::value_type; } const { return value; }\n"
                                "   explicit int __cdecl operator int() const { return 7; }\n"
                                "};\n"
                              , (*it++).second[0].fullyQualified, "should have gotten the struct");
            }
        }
    },

    {"Testing CXXRecordDeclSerializer on UDTs", []
        {
            std::string code = "struct Qux {}; struct Bar {}; struct Baz{}; struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : Baz, virtual private Bar, protected Qux {};";

            OdrCop3::AllMaps maps;
            bool ok = clang::tooling::runToolOnCodeWithArgs(std::make_unique<OdrCop3::VisitorAction>(maps), code, { "-x", "c++", "-std=c++23" });
            Assert::IsTrue(ok);
            Assert::AreEqual(4, maps.udtMap.size() + maps.varMap.size() + maps.enumMap.size() + maps.typedefMap.size() + maps.functionMap.size(), "should have found a map entry");

            auto it = maps.udtMap.begin();
            Assert::AreEqual("Bar",      it->first,        "should have gotten correct key");
            Assert::AreEqual("input.cc", it->second[0].TU, "should have gotten the TU name");

            Assert::AreEqual("struct Bar { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Baz { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct [[deprecated(\"use Bar instead\")]] alignas(32) Foo final : public Baz, virtual private Bar, protected Qux { // sizeof=32\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
            Assert::AreEqual("struct Qux { // sizeof=1\n"
                             "};\n"
                           , (*it++).second[0].fullyQualified, "should have gotten the struct");
        }
    },
};
