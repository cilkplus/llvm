
//==--- DiagnosticSemaKinds.td - libsema diagnostics ----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// Semantic Analysis
//===----------------------------------------------------------------------===//

let Component = "Sema" in {
let CategoryName = "Semantic Issue" in {

// if INTEL_SPECIFIC_CILKPLUS
def warn_empty_cilk_for_body : Warning<
  "Cilk for loop has empty body">, InGroup<EmptyBody>;
def warn_empty_simd_for_body : Warning<
  "SIMD for loop has empty body">, InGroup<EmptyBody>;
def err_pragma_simd_reuse_var : Error<
  "%select{private|lastprivate|firstprivate|linear|reduction}0 variable shall "
  "not appear in multiple simd clauses">;
def err_pragma_simd_reduction_maxmin : Error<
  "reduction operator %select{min|max}0 requires arithmetic type">;
def note_pragma_simd_used_here : Note<
  "first used here">;
def err_pragma_simd_conflict_step : Error<
  "linear %select{variable|step}0 shall not be used"
  " as a linear %select{step|variable}0">;
def err_pragma_simd_vectorlength_conflict : Error<
  "cannot specify both vectorlength%select{for|}0 and vectorlength%select{|for}0">;
def err_pragma_simd_vectorlength_multiple : Error<
  "cannot specify multiple vectorlength%select{for|}0 clauses">;
def note_pragma_simd_specified_here : Note<
  "vectorlength%select{for|}0 first specified here">;
def err_invalid_vectorlength_expr : Error<
  "vectorlength expression must be %select{an integer constant|a power of 2}0">;
def err_pragma_simd_invalid_linear_step : Error<
  "invalid linear step: expected integral constant or variable reference">;
def warn_pragma_simd_linear_expr_zero : Warning<
  "linear step is zero">, InGroup<SourceUsesCilkPlus>, DefaultIgnore;
def err_pragma_simd_invalid_private_var : Error<
  "invalid variable in %0 clause">;
def err_pragma_simd_var_incomplete : Error<
  "variable in %0 clause shall not be incomplete: %1">;
def err_pragma_simd_var_reference : Error<
  "variable in %0 clause shall not be a reference: %1">;
def err_pragma_simd_var_const: Error<
  "variable in %0 clause shall not be const-qualified">;
def err_pragma_simd_var_array: Error<
  "variable in %0 clause shall not be of array type">;
def note_pragma_simd_var: Note<
  "variable declared here">;
def err_pragma_simd_var_no_member: Error<
  "variable in %0 clause has no "
  "%select{default constructor|default copy constructor|copy assignment operator}1 "
  "or it is deleted: %2">;
def err_pragma_simd_var_ambiguous_member: Error<
  "variable in %0 clause has ambiguous "
  "%select{default constructors|default copy constructors|copy assignment operators}1: %2">;
def err_pragma_simd_reduction_invalid_var : Error<
  "invalid reduction variable">;

// Cilk Plus warnings and errors
// cilk_spawn
def err_not_a_call : Error<
  "the argument to _Cilk_spawn must be a function call">;
def err_spawn_invalid_scope : Error<
  "_Cilk_spawn not allowed in this scope">;
def err_spawn_invalid_decl : Error<
  "_Cilk_spawn not supported in a '%0Decl'">;
def err_spawn_spawn : Error<
  "consecutive _Cilk_spawn tokens not allowed">;
def err_spawn_not_whole_expr : Error<
  "_Cilk_spawn is not at statement level">;
def err_cannot_spawn_function: Error<
  "function cannot be spawned">;
def err_cannot_init_static_variable: Error<
  "cannot spawn initialize a 'static' variable">;
def err_multiple_spawns: Error<
  "multiple spawn calls within a full expression">;
def note_multiple_spawns: Note<
  "another spawn here">;
def err_cilk_spawn_disable: Error<
  "_Cilk_spawn disabled in statement expression">;

// cilk_for
def err_cilk_for_initializer_expected_variable : Error<
  "expected a variable for control variable in '_Cilk_for'">;
def err_cilk_for_initializer_expected_decl : Error<
  "expected control variable declaration in initializer in '_Cilk_for'">;
def err_cilk_for_decl_multiple_variables : Error<
  "cannot declare more than one loop control variable in '_Cilk_for'">;
def err_cilk_for_init_multiple_variables : Error<
  "cannot initialize more than one loop control variable in '_Cilk_for'">;
def err_cilk_for_control_variable_not_initialized : Error<
  "_Cilk_for loop control variable must be initialized">;
def err_cilk_for_initialization_must_be_decl : Error<
  "loop initialization must be a declaration in '_Cilk_for'">;

def err_cilk_for_control_variable_storage_class : Error<
  "loop control variable cannot have storage class '%0' in '_Cilk_for'">;
def err_cilk_for_control_variable_qualifier : Error<
  "loop control variable cannot be '%0' in '_Cilk_for'">;
def err_cilk_for_control_variable_not_local : Error<
  "non-local loop control variable in '_Cilk_for'">;
def err_cilk_for_control_variable_type : Error<
  "loop control variable must have an integral, pointer, or class type "
  "in '_Cilk_for'">;

def err_cilk_for_invalid_cond_expr: Error<
  "expected binary comparison operator in '_Cilk_for' loop condition">;
def err_cilk_for_invalid_cond_operator: Error<
  "loop condition operator must be one of '<', '<=', '>', '>=', or '!=' "
  "in '_Cilk_for'">;
def err_cilk_for_cond_test_control_var: Error<
  "loop condition does not test control variable %0 in '_Cilk_for'">;
def note_cilk_for_cond_allowed: Note<
  "allowed forms are %0 OP expr, and expr OP %0">;
def err_cilk_for_difference_ill_formed: Error<
  "end - begin must be well-formed in '_Cilk_for'">;
def err_non_integral_cilk_for_difference_type: Error<
  "end - begin must have integral type in '_Cilk_for' - got %0">;

def err_cilk_for_increment_not_control_var: Error<
  "loop increment does not modify control variable %0 in '_Cilk_for'">;
def err_cilk_for_invalid_increment : Error<
  "loop increment operator must be one of operators '++', '--', '+=', or '-=' in '_Cilk_for'">;
def err_cilk_for_invalid_increment_rhs : Error<
  "right-hand side of '%0' must have integral or enum type in '_Cilk_for' increment">;
def err_cilk_for_increment_inconsistent : Error<
  "loop increment is inconsistent with condition in '_Cilk_for': expected %select{negative|positive}0 stride">;
def err_cilk_for_increment_zero : Error<
  "loop increment must be non-zero in '_Cilk_for'">;
def err_cilk_for_loop_modifies_control_var : Error<
  "Modifying the loop control variable inside a '_Cilk_for' has undefined behavior">;
def warn_cilk_for_loop_control_var_func : Warning<
  "Modifying the loop control variable inside a '_Cilk_for' using a function call has undefined behavior">, InGroup<CilkPlusLoopControlVarModification>, DefaultIgnore;
def warn_cilk_for_loop_control_var_aliased: Warning<
  "Modifying the loop control variable '%0' through an alias in '_Cilk_for' has undefined behavior">, InGroup<CilkPlusLoopControlVarModification>, DefaultIgnore;
def note_cilk_for_loop_control_var_declared_here: Note<
  "'_Cilk_for' loop control variable declared here">;

def note_cilk_for_begin_expr: Note<
  "loop begin expression here">;
def note_cilk_for_end_expr: Note<
  "loop end expression here">;
def note_constant_stride: Note<
  "constant stride is %0">;
def warn_cilk_for_cond_user_defined_conv: Warning<
  "user-defined conversion from %0 to %1 will not be used when calculating the "
  "number of iterations in '_Cilk_for'">, InGroup<Conversion>, DefaultWarn;
def note_cilk_for_conversion_here : Note<
  "conversion to type %0 declared here">;
def err_cilk_for_cannot_return: Error<
  "cannot return from within a '_Cilk_for' loop">;
def err_cilk_for_cannot_break: Error<
  "cannot break from a '_Cilk_for' loop">;
def warn_cilk_for_loop_count_downcast: Warning<
  "implicit loop count downcast from %0 to %1 in '_Cilk_for'">,
  InGroup<Conversion>, DefaultWarn;
def note_local_variable_declared_here : Note<
  "%0 declared here">;

def err_cilk_for_grainsize_negative: Error<
  "the behavior of Cilk for is unspecified for a negative grainsize">;
def note_cilk_for_grainsize_conversion : Note<
  "grainsize must evaluate to a type convertible to %0">;

def warn_cilk_for_wraparound: Warning<
  "%0 stride causes %1 wraparound">, InGroup<SourceUsesCilkPlus>, DefaultWarn;

def note_cilk_for_wraparound_undefined: Note<
  "wraparounds cause undefined behavior in Cilk for">;

def err_cilk_elemental_not_function_parameter: Error<
  "not a function parameter">;
def err_cilk_elemental_vectorlength: Error<
  "vectorlength must be positive">;
def err_cilk_elemental_unrecognized_processor: Error<
  "unrecognized processor">;
def err_cilk_elemental_linear_parameter_type: Error<
  "linear parameter type %0 is not an integer or pointer type">;
def warn_cilk_elemental_inconsistent_processor: Warning<
  "inconsistent processor attribute">,
  InGroup<SourceUsesCilkPlus>, DefaultWarn;
def err_cilk_elemental_exception_spec : Error<
  "exception specifications are not allowed on elemental functions">;
def err_cilk_elemental_step_not_uniform : Error<
  "linear step parameter must also be uniform">;
def err_cilk_elemental_subject: Error<
  "parameter %0 cannot be the subject of two elemental clauses">;
def err_cilk_elemental_this_subject: Error<
  "'this' cannot be the subject of two elemental clauses">;
def note_cilk_elemental_subject_clause: Note<
  "another %select{uniform|linear}0 clause here">;
def note_cilk_elemental_subject_parameter: Note<
  "parameter here">;
def err_cilk_elemental_linear_step_not_integral : Error<
  "linear step has non-integer type %0">;
def err_cilk_elemental_linear_step_not_constant : Error<
  "linear step is not constant">;
def err_cilk_elemental_aligned_not_integral : Error<
  "aligned has non-integer type %0">;
def err_cilk_elemental_aligned_not_constant : Error<
  "aligned is not a constant">;
def err_cilk_elemental_repeated_vectorlength: Error<
  "repeated %select{vectorlength|vectorlengthfor}0 attribute is not allowed">;
def err_cilk_elemental_both_mask_nomask : Error<
  "elemental function cannot have both mask and nomask attributes">;
def note_cilk_elemental_mask_here: Note<
  "%select{nomask|mask}0 attribute here">;

// simd for
def err_simd_for_body_no_construct: Error<
  "%0 is not allowed within %select{simd for|an elemental function}1">;
def err_simd_for_nested: Error<
  "nested simd for not supported">;
def note_simd_for_nested: Note<
  "outer simd for here">;
def err_simd_for_body_no_nontrivial_destructor: Error<
  "non-static variable with a non-trivial destructor is not allowed within %select{simd for|an elemental function}0">;
def err_simd_for_decl_multiple_variables : Error<
  "cannot declare more than one loop control variable in simd for">;
def err_simd_for_initializer_expected_decl : Error<
  "expected control variable declaration in simd for initialization">;
def err_simd_for_invalid_lcv_type : Error<
  "loop control variable shall have an integer or pointer type, %0 type here">;
def err_simd_for_control_variable_not_initialized : Error<
  "simd for loop control variable must be initialized">;
def err_simd_for_initialization_must_be_decl : Error<
  "loop initialization must be a declaration in simd for">;
def err_simd_for_init_multiple_variables : Error<
  "cannot initialize more than one loop control variable in simd for">;
def err_simd_for_expect_loop_control_variable : Error<
  "expect a loop control variable in simd for">;
def err_simd_for_control_variable_storage_class : Error<
  "loop control variable cannot have storage class '%0' in simd for">;
def err_simd_for_control_variable_qualifier : Error<
  "loop control variable cannot be '%0' in simd for">;
def err_simd_for_control_variable_not_local : Error<
  "non-local loop control variable in simd for">;
def err_simd_for_invalid_cond_expr: Error<
  "expected binary comparison operator in simd for loop condition">;
def err_simd_for_invalid_cond_operator: Error<
  "loop condition operator must be one of '<', '<=', '>', '>=', or '!=' "
  "in simd for">;
def err_simd_for_cond_test_control_var: Error<
  "loop condition does not test control variable %0 in simd for">;
def note_simd_for_cond_allowed: Note<
  "allowed forms are %0 OP expr, and expr OP %0">;
def err_simd_for_increment_not_control_var: Error<
  "loop increment does not modify control variable %0 in simd for">;
def err_simd_for_invalid_increment : Error<
  "loop increment operator must be one of operators '++', '--', '+=', or '-=' in simd for">;
def err_simd_for_invalid_increment_rhs : Error<
  "right-hand side of '%0' must have integral or enum type in simd for increment">;
def err_simd_for_increment_zero : Error<
  "loop increment must be non-zero in simd for">;
def err_simd_for_increment_inconsistent : Error<
  "loop increment is inconsistent with condition in simd for: expected %select{negative|positive}0 stride">;
def err_simd_for_cannot_return: Error<
  "cannot return from within a simd for loop">;
def err_simd_for_cannot_break: Error<
  "cannot break from a simd for loop">;
def err_simd_for_lcv_invalid_clause: Error<
  "the simd loop control variable may not be the subject of a %select{reduction|firstprivate}0 clause">;
def warn_simd_for_variable_lcv: Warning<
  "ignoring simd clause applied to simd loop control variable">,
  InGroup<SourceUsesCilkPlus>, DefaultWarn;

def err_cean_incorrect_form : Error<
  "incorrect form of extended array notation">;
def err_cean_no_length_for_non_array : Error<
  "cannot define default length for non-array type %0">;
def err_cean_lower_bound_not_integer : Error<
  "lower bound expression has non-integer type %0">;
def err_cean_length_not_integer : Error<
  "length expression has non-integer type %0">;
def err_cean_stride_not_integer : Error<
  "stride expression has non-integer type %0">;
def err_cean_not_in_statement : Error<
  "extended array notation is not allowed">;
def warn_cean_wrong_length : Warning<
  "length is %select{negative|zero}0">, InGroup<CilkPlusCEAN>, DefaultWarn;
def err_cean_rank_mismatch : Error<
  "rank mismatch in array section expression">;
def note_cean_if_rank : Note<
  "rank is defined by condition">;
def err_cean_different_length : Error<
  "different length value in extended array notation expression">;
def note_cean_different_length : Note<
  "length specified here">;
def err_cean_rank_zero : Error<
  "argument of the function is not an array section">;
def err_cean_rank_not_one : Error<
  "rank of the argument must be exactly one">;
def err_cean_rank_not_zero : Error<
  "rank of the argument must be zero">;
def err_cean_expr_rank_not_zero : Error<
  "rank of the expression must be zero">;
def err_cean_rank_not_zero_or_one : Error<
  "rank of the argument must be not greater than one">;
def err_cean_not_scalar_or_complex : Error<
  "type of argument is not scalar or complex">;
def err_cean_not_scalar : Error<
  "type of argument is not scalar">;
def err_cean_not_integer : Error<
  "type of argument is not integer">;
def err_cean_less_zero : Error<
  "value of argument is negative integer">;
def err_cean_arg_not_lvalue : Error<
  "argument expression must be an l-value">;
// endif INTEL_SPECIFIC_CILKPLUS

} // end of "Semantic Issue"
} // end of sema component.
