#pragma once

#include <clang\AST\AST.h>
#include <clang\AST\RecursiveASTVisitor.h>
#include <clang\Frontend\FrontendActions.h>
#include <clang\Frontend\CompilerInstance.h>
#include <clang\Tooling\Tooling.h>
#include <clang\Tooling\CompilationDatabase.h>
#include <clang\AST\Mangle.h>
#include <clang\AST\Decl.h>
#include <clang\AST\GlobalDecl.h>
#include <clang\AST\RecordLayout.h>
#include <llvm\Support\raw_ostream.h>

#include <vector>

#include "SerializationUtils.h"
#include "magic_enum.h"

namespace OdrCop3
{
    namespace Serialize
    {
        template<auto SerializeDecl, auto SerializeAttr>
        static std::string Types(const ContextItems& contextItems, const clang::Type* type)
        {
            if (!type)
                throw std::invalid_argument("type argument was null");

            switch (type->getTypeClass())
            {
            case clang::Type::TypeClass::Builtin:
            case clang::Type::TypeClass::Pointer:
            case clang::Type::TypeClass::Record:
            case clang::Type::TypeClass::Enum:
            case clang::Type::TypeClass::Typedef:
            case clang::Type::TypeClass::TemplateSpecialization:
         // case clang::Type::TypeClass::Elaborated:
            case clang::Type::TypeClass::InjectedClassName:
            case clang::Type::TypeClass::PackExpansion:
            default:
                break;
            };
            throw OdrCop3::UnhandledException(std::string("unhandled type::getTypeClass: ") + enum_name(type->getTypeClass()));
        }
    }
}