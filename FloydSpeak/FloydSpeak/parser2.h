
//
//  parser2.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 25/08/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser2_h
#define parser2_h


#include "quark.h"
#include "text_parser.h"
#include "utils.h"

#include <string>
#include <memory>


namespace parser2 {
/*
	C99-language constants.
	http://en.cppreference.com/w/cpp/language/operator_precedence
*/
const std::string k_c99_number_chars = "0123456789.";
const std::string k_c99_whitespace_chars = " \n\t\r";
	const std::string k_c99_identifier_chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

/*
	White-space policy:
	All function SUPPORT leading whitespace.
	No need to filter when you return for next function.
	Why: only one function entry, often many function exists.
*/


///////////////////////////////////			eoperator_precedence


/*
	Operator precedence is the same as C99.
	Lower number gives stronger precedence.
*/
enum class eoperator_precedence {
	k_super_strong = 0,

	//	(xyz)
	k_parentesis = 0,

	//	a()
	k_function_call = 2,

	//	a[], aka subscript
	k_looup = 2,

	//	a.b
	k_member_access = 2,


	k_multiply_divider_remainder = 5,

	k_add_sub = 6,

	//	<   <=	For relational operators < and ≤ respectively
	//	>   >=
	k_larger_smaller = 8,


	k_equal__not_equal = 9,

	k_logical_and = 13,
	k_logical_or = 14,

	k_comparison_operator = 15,

	k_super_weak
};


///////////////////////////////////			eoperation


/*
	These are the operations generated by parsing the C-style expression.
	The order of constants inside enum not important.

	The number tells how many operands it needs.
*/
enum class eoperation {
	k_0_number_constant = 100,

	//	This is string specifying a local variable, member variable, argument, global etc. Only the first entry in a chain.
	k_0_resolve,

	k_0_string_literal,

	k_x_member_access,

	k_2_looup,

	k_2_add,
	k_2_subtract,
	k_2_multiply,
	k_2_divide,
	k_2_remainder,

	k_2_smaller_or_equal,
	k_2_smaller,

	k_2_larger_or_equal,
	k_2_larger,

	//	a == b
	k_2_logical_equal,

	//	a != b
	k_2_logical_nonequal,

	//	a && b
	k_2_logical_and,

	//	a || b
	k_2_logical_or,

	//	cond ? a : b
	k_3_conditional_operator,

	k_n_call,

	//	!a
//	k_1_logical_not

	//	-a
	k_1_unary_minus,


//	k_0_identifier

	k_1_vector_definition
};



///////////////////////////////////			constant_value_t


/*
	??? rename to "literal_t"?
	Used to store a constant, of one of the built-in C-types.
*/
struct constant_value_t {
	explicit constant_value_t(bool value) :
		_type(etype::k_bool),
		_bool(value)
	{
	}

	explicit constant_value_t(int value) :
		_type(etype::k_int),
		_int(value)
	{
	}

	explicit constant_value_t(float value) :
		_type(etype::k_float),
		_float(value)
	{
	}

	explicit constant_value_t(const std::string& value) :
		_type(etype::k_string),
		_string(value)
	{
	}

	enum class etype {
		k_bool,
		k_int,
		k_float,
		k_string
	};


	/////////////////		STATE
	etype _type;

	bool _bool = false;
	int _int = 0;
	float _float = 0.0f;
	std::string _string;
};

inline bool operator==(const constant_value_t& a, const constant_value_t& b){
	return a._type == b._type
		&& a._bool == b._bool
		&& a._int == b._int
		&& a._float == b._float
		&& a._string == b._string;
}


///////////////////////////////////			expr_t


//	Node in expression tree


struct expr_t {
	eoperation _op;
	std::vector<expr_t> _exprs;
	std::shared_ptr<constant_value_t> _constant;
	std::string _identifier;
};

inline bool operator==(const expr_t& a, const expr_t& b){
	return a._op == b._op
		&& a._exprs == b._exprs
		&& compare_shared_values(a._constant, b._constant)
		&& a._identifier == b._identifier;
}

///////////////////////////////////			FUNCTIONS



bool is_valid_expr_chars(const std::string& s);
seq_t skip_expr_whitespace(const seq_t& p);

std::pair<std::string, seq_t> parse_string_literal(const seq_t& p);


// [0-9] and "."  => numeric constant.
std::pair<constant_value_t, seq_t> parse_numeric_constant(const seq_t& p);

/*
	Constant literal or identifier.
		3
		3.0
		"three"
		true
		false
		hello2
		x
*/
std::pair<expr_t, seq_t> parse_terminal(const seq_t& p);

/*
	Atom = standalone expression, like a constant, a function call.
	It can be composed of subexpressions

	It may then be chained rightwards using operations like "+" etc.

	Examples:
		123
		-123
		--+-123
		(123 + 123 * x + f(y*3))
		[ 1, 2, calc_exp(3) ]
*/
std::pair<expr_t, seq_t> parse_lhs_atom(const seq_t& p);

/*
	lhs()
	lhs(1)
	lhs(a + b)
	lhs(a, b)

	rhs("start", get_playlist())
		used like this: convert_to_json("start", get_playlist())
*/
std::pair<std::vector<expr_t>, seq_t> parse_function_call_operation(const seq_t& p1);

/*
	hello.func(x)
	lhs = ["@", "hello"]
*/
std::pair<expr_t, seq_t> parse_member_access_operation(const seq_t& p, const expr_t& lhs, const eoperator_precedence prev_precedence);

/*

	lhs "+" EXPR
	lhs "==" EXPR
*/
std::pair<expr_t, seq_t> parse_lookup_operation(const seq_t& p, const expr_t& lhs, const eoperator_precedence prev_precedence);

std::pair<expr_t, seq_t> parse_optional_operation_rightward(const seq_t& p0, const expr_t& lhs, const eoperator_precedence precedence);

std::pair<expr_t, seq_t> parse_expression_int(const seq_t& p, const eoperator_precedence precedence);

//	Top-level function that parses entire expression into expr_t.
std::pair<expr_t, seq_t> parse_expression(const seq_t& p);


}	//	parser2

#endif /* parser2_h */
