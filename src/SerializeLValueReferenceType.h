#pragma once

#include "SerializePointerAndLValueReferenceTypes.h"

namespace OdrCop3
{
    template<auto SerializeDecl, auto SerializeType, auto SerializeExpr> class LValueReferenceTypeSerializer : public PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>
    {
    public:
        LValueReferenceTypeSerializer(const ContextItems& contextItems, QualType qt, const LValueReferenceType* )
            : PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>(contextItems, qt)
        {}
        std::string Serialize() const
        {
            return PointerAndLValueReferenceTypesSerializer<SerializeDecl, SerializeType, SerializeExpr>::Serialize("&");
        }
    };
}