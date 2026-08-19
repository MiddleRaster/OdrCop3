#pragma once

#include "SerializePointerAndLValueReferenceTypes.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class RValueReferenceTypeSerializer : public PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
    public:
        RValueReferenceTypeSerializer(const ContextItems& contextItems, QualType qt, const RValueReferenceType *)
            : PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt)
        {}
        std::string Serialize() const
        {
            return PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize("&&");
        }
    };
}