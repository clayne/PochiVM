#pragma once

#include "common_expr.h"
#include "error_context.h"
#include "codegen_context.hpp"
#include "ast_type_helper.hpp"

namespace Ast
{

using namespace llvm;

Value* WARN_UNUSED AstDereferenceExpr::EmitIRImpl()
{
    Value* op = m_operand->EmitIR();
    Value* inst = thread_llvmContext->m_builder.CreateLoad(op);
    CHECK_REPORT_BUG(inst != nullptr, "llvm internal error");
    return inst;
}

Value* WARN_UNUSED AstLiteralExpr::EmitIRImpl()
{
    Value* inst = nullptr;
    TypeId typeId = GetTypeId();
    if (typeId.IsPrimitiveIntType())
    {
        // Integer type cases: bool needs to be handled specially
        //
        if (typeId.IsBool())
        {
            if (m_as_bool)
            {
                inst = ConstantInt::getTrue(thread_llvmContext->m_llvmContext);
            }
            else
            {
                inst = ConstantInt::getFalse(thread_llvmContext->m_llvmContext);
            }
        }
        else
        {
            inst = ConstantInt::get(thread_llvmContext->m_llvmContext,
                                    APInt(static_cast<unsigned>(typeId.Size()) * 8 /*numBits*/,
                                          StaticCastIntTypeValueToUInt64(),
                                          typeId.IsSigned() /*isSigned*/));
        }
    }
    else if (typeId.IsPrimitiveFloatType())
    {
        // Float type cases
        // APFloat has different constructors for float and double parameter,
        // thus giving us the correct type for float and double
        //
        if (typeId.IsFloat())
        {
            inst = ConstantFP::get(thread_llvmContext->m_llvmContext, APFloat(GetFloat()));
        }
        else if (typeId.IsDouble())
        {
            inst = ConstantFP::get(thread_llvmContext->m_llvmContext, APFloat(GetDouble()));
        }
    }
    // TODO: handle pointer
    CHECK_REPORT_BUG(inst != nullptr, "unhandled literal type or llvm internal error");
    return inst;
}

Value* WARN_UNUSED AstAssignExpr::EmitIRImpl()
{
    CHECK_REPORT_BUG(false, "unimplemented");
}

Value* WARN_UNUSED AstNullptrExpr::EmitIRImpl()
{
    CHECK_REPORT_BUG(false, "unimplemented");
}

Value* WARN_UNUSED AstTrashPtrExpr::EmitIRImpl()
{
    CHECK_REPORT_BUG(false, "unimplemented");
}

}   // namespace Ast
