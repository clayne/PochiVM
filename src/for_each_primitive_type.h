#pragma once

#define FOR_EACH_PRIMITIVE_INT_TYPE \
F(bool)     \
F(int8_t)   \
F(int16_t)  \
F(int32_t)  \
F(int64_t)  \
F(uint8_t)  \
F(uint16_t) \
F(uint32_t) \
F(uint64_t)

#define FOR_EACH_PRIMITIVE_FLOAT_TYPE \
F(float)    \
F(double)

#define FOR_EACH_PRIMITIVE_TYPE \
    FOR_EACH_PRIMITIVE_INT_TYPE \
    FOR_EACH_PRIMITIVE_FLOAT_TYPE

// All allowed widening conversions, which are the only implicit conversions allowed.
//
#define FOR_EACH_PRIMITIVE_INT_TYPE_WIDENING_CONVERSION \
F(bool, uint8_t)       \
F(bool, uint16_t)      \
F(bool, uint32_t)      \
F(bool, uint64_t)      \
F(bool, int8_t)        \
F(bool, int16_t)       \
F(bool, int32_t)       \
F(bool, int64_t)       \
F(uint8_t, uint16_t)   \
F(uint8_t, uint32_t)   \
F(uint8_t, uint64_t)   \
F(uint16_t, uint32_t)  \
F(uint16_t, uint64_t)  \
F(uint32_t, uint64_t)  \
F(int8_t, int16_t)     \
F(int8_t, int32_t)     \
F(int8_t, int64_t)     \
F(int16_t, int32_t)    \
F(int16_t, int64_t)    \
F(int32_t, int64_t)

namespace Ast
{

#define F(type) +1
const static int x_num_primitive_types = FOR_EACH_PRIMITIVE_TYPE;
const static int x_num_primitive_int_types = FOR_EACH_PRIMITIVE_INT_TYPE;
const static int x_num_primitive_float_types = FOR_EACH_PRIMITIVE_FLOAT_TYPE;
#undef F

}   // namespace Ast

