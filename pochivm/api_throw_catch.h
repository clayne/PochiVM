#pragma once

#include "ast_catch_throw.h"
#include "api_base.h"
#include "api_function_proto.h"

namespace PochiVM
{

// Throw out an exception. This may be a fundamental type, or a CPP type.
// For CPP types, it must be a value returned by a function call (since it is the only way
// to get a Value<T> where T is CPP type).
//
// In interp mode, the return value is always copy-initialized to the exception object,
// so copy-elision *never* happens. However in LLVM mode, copy-elision *always* happens:
// we always directly construct the exception object in-place (this is possible since it must be
// returned by a C++ function call).
//
// Although the behavior of whether copy-elision happens is different in interp mode and LLVM mode,
// both behaviors are explicitly allowed by C++ standard, since by standard C++17 guaranteed
// copy-elision does not apply to throw.
//
template<typename T>
inline Value<void> Throw(const Value<T>& expr)
{
    static_assert(!std::is_same<T, void>::value, "Cannot throw void expression");
    return Value<void>(new AstThrowStmt(expr.m_ptr, false /*isCtor*/, false /*isLValueObject*/));
}

// Throw out an exception by calling a constructor to construct the exception object.
//
template<typename T>
Value<void> Throw(const Constructor<T>& constructorParams)
{
    static_assert(AstTypeHelper::is_cpp_class_type<T>::value, "must be a CPP class type");
    T* value = nullptr;
    AstLiteralExpr* placeholder = new AstLiteralExpr(TypeId::Get<T>().AddPointer(), &value);
    AstCallExpr* callExpr = internal::GetCallExprFromConstructor(placeholder, constructorParams);
    return Value<void>(new AstThrowStmt(callExpr, true /*isCtor*/, false /*isLValueObject*/));
}

// Throw out a CPP class object LValue.
// The object is copy-initialized to the exception object.
// We only enable this function for CPP class types, as for other types Reference inherits Value already.
//
template<typename T, typename = std::enable_if_t<AstTypeHelper::is_cpp_class_type<T>::value> >
Value<void> Throw(const Reference<T>& expr)
{
    // TODO: static_assert that the copy constructor is registered.
    //
    static_assert(AstTypeHelper::is_cpp_class_type<T>::value, "must be a CPP class type");
    return Value<void>(new AstThrowStmt(expr.m_refPtr, false /*isCtor*/, true /*isLValueObject*/));
}

}   // namespace PochiVM
