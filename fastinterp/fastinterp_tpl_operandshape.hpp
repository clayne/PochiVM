#pragma once

#include "fastinterp_tpl_operandshape.h"
#include "fastinterp_tpl_common.hpp"

namespace PochiVM
{

struct FISimpleOperandShapeCategoryHelper
{
#define OSCHELPER_GENERATE_METHOD(meth_name, placeholder1)                                                      \
    template<typename OperandType, FISimpleOperandShapeCategory osc>                                            \
    static OperandType WARN_UNUSED __attribute__((__always_inline__)) meth_name(uintptr_t stackframe) noexcept  \
    {                                                                                                           \
        if constexpr(osc == FISimpleOperandShapeCategory::LITERAL_NONZERO)                                      \
        {                                                                                                       \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, OperandType);                                    \
            return CONSTANT_PLACEHOLDER_ ## placeholder1;                                                       \
        }                                                                                                       \
        else if constexpr(osc == FISimpleOperandShapeCategory::ZERO)                                            \
        {                                                                                                       \
            constexpr OperandType v = PochiVM::get_all_bits_zero_value<OperandType>();                          \
            return v;                                                                                           \
        }                                                                                                       \
        else if constexpr(osc == FISimpleOperandShapeCategory::VARIABLE)                                        \
        {                                                                                                       \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, uint32_t);                                       \
            return *GetLocalVarAddress<OperandType>(stackframe, CONSTANT_PLACEHOLDER_ ## placeholder1);         \
        }                                                                                                       \
        else                                                                                                    \
        {                                                                                                       \
            static_assert(type_dependent_false<OperandType>::value, "unexpected literal category");             \
        }                                                                                                       \
    }

    OSCHELPER_GENERATE_METHOD(get_0, 0)
    OSCHELPER_GENERATE_METHOD(get_1, 1)

#undef OSCHELPER_GENERATE_METHOD
};

struct FIOperandShapeCategoryHelper
{
    template<typename OscIndexType, FIOperandShapeCategory osc>
    static constexpr bool cond()
    {
        if (!is_valid_index_type<OscIndexType>())
        {
            return false;
        }
        // If osc is not an array-element shape, we should always pass in the fake oscIndexType of int32_t
        //
        if (!(osc == FIOperandShapeCategory::VARPTR_VAR ||
            osc == FIOperandShapeCategory::VARPTR_LIT_NONZERO) &&
            !std::is_same<OscIndexType, int32_t>::value)
        {
            return false;
        }
        return true;
    }
#if 0
    // Generate a method which get the operand using a pair of placeholders of specified ordinals
    // As is all helprs in fastinterp_tpl, always_inline is required.
    //
#define OSCHELPER_GENERATE_METHOD(meth_name, placeholder1, placeholder2)                                    \
    template<typename OperandType, typename OscIndexType, FIOperandShapeCategory osc>                       \
    static OperandType WARN_UNUSED __attribute__((__always_inline__)) meth_name() noexcept                  \
    {                                                                                                       \
        if constexpr(osc == FIOperandShapeCategory::COMPLEX)                                                \
        {                                                                                                   \
            INTERNAL_DEFINE_BOILERPLATE_FNPTR_PLACEHOLDER(placeholder1, OperandType(*)() noexcept);         \
            return BOILERPLATE_FNPTR_PLACEHOLDER_ ## placeholder1();                                        \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::LITERAL_NONZERO)                                   \
        {                                                                                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, OperandType);                                \
            return CONSTANT_PLACEHOLDER_ ## placeholder1;                                                   \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::ZERO)                                              \
        {                                                                                                   \
            constexpr OperandType v = PochiVM::get_all_bits_zero_value<OperandType>();                      \
            return v;                                                                                       \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::VARIABLE)                                          \
        {                                                                                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, uint32_t);                                   \
            return *GetLocalVarAddress<OperandType>(CONSTANT_PLACEHOLDER_ ## placeholder1);                 \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::VARPTR_DEREF)                                      \
        {                                                                                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, uint32_t);                                   \
            return **GetLocalVarAddress<OperandType*>(CONSTANT_PLACEHOLDER_ ## placeholder1);               \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::VARPTR_VAR)                                        \
        {                                                                                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, uint32_t);                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder2, uint32_t);                                   \
            OperandType* varPtr = *GetLocalVarAddress<OperandType*>(CONSTANT_PLACEHOLDER_ ## placeholder1); \
            OscIndexType index = *GetLocalVarAddress<OscIndexType>(CONSTANT_PLACEHOLDER_ ## placeholder2);  \
            return varPtr[index];                                                                           \
        }                                                                                                   \
        else if constexpr(osc == FIOperandShapeCategory::VARPTR_LIT_NONZERO)                                \
        {                                                                                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder1, uint32_t);                                   \
            INTERNAL_DEFINE_CONSTANT_PLACEHOLDER(placeholder2, OscIndexType);                               \
            OperandType* varPtr = *GetLocalVarAddress<OperandType*>(CONSTANT_PLACEHOLDER_ ## placeholder1); \
            return varPtr[CONSTANT_PLACEHOLDER_ ## placeholder2];                                           \
        }                                                                                                   \
        else                                                                                                \
        {                                                                                                   \
            static_assert(type_dependent_false<OperandType>::value, "unexpected literal category");         \
        }                                                                                                   \
    }

    OSCHELPER_GENERATE_METHOD(get_0_1, 0, 1)
    OSCHELPER_GENERATE_METHOD(get_2_3, 2, 3)

    OSCHELPER_GENERATE_METHOD(get_1_2, 1, 2)
    OSCHELPER_GENERATE_METHOD(get_3_4, 3, 4)

#undef OSCHELPER_GENERATE_METHOD
#endif
};

}   // namespace PochiVM
