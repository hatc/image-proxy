// dumbassert.h
#pragma once

#ifndef __CPCL_DUMBASSERT_H
#define __CPCL_DUMBASSERT_H

namespace cpcl {

#ifdef _MSC_VER
typedef void (__stdcall *AssertHandlerPtr)(char const *s, char const *file, unsigned int line);
#else
typedef void (*AssertHandlerPtr)(char const *s, char const *file, unsigned int line);
#endif
void SetAssertHandler(AssertHandlerPtr v);

void dumbassert(char const *s, char const *file, unsigned int line);

} // namespace cpcl

#define DUMBASS_CHECK(Expr) (void)((!!(Expr)) || (cpcl::dumbassert(#Expr, __FILE__, __LINE__),0))
#define DUMBASS_CHECK_EXPLANATION(Expr, Explanation) (void)((!!(Expr)) || (cpcl::dumbassert(Explanation, __FILE__, __LINE__),0))

#endif // __CPCL_DUMBASSERT_H
