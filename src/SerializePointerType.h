#pragma once

#include "SerializePointerAndLValueReferenceTypes.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class PointerTypeSerializer : public PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
    public:
        PointerTypeSerializer (const ContextItems& contextItems, QualType qt, const PointerType *)
            : PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt)
        {}
        std::string Serialize() const
        {
            return PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize("*");
        }
    };
}