/* Test pragma region directive from
   http://msdn.microsoft.com/en-us/library/b6xkz944(v=vs.80).aspx */

// Editor-only pragma, just skipped by compiler.
// Syntax:
// #pragma region optional name
// #pragma endregion optional comment
//
// RUN: %clang_cc1 -fsyntax-only -verify -Wall -fms-extensions %s

#pragma region
/* inner space */
#pragma endregion

#pragma region long name
/* inner space */
void foo(void){}
#pragma endregion long comment

void inner();

__pragma(region) // expected-warning {{the #pragma region unclosed at end of file}}
_Pragma("region") // expected-warning {{the #pragma region unclosed at end of file}}

#pragma region2 // expected-warning {{unknown pragma ignored}}

#pragma region one // expected-warning {{the #pragma region unclosed at end of file}}
#pragma region inner
//#pragma endregion inner

#pragma endregion end

// {{unclosed pragma region}} - region mismatches is not detected yet
