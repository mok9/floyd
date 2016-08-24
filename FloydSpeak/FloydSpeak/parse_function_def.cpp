//
//  parser_function.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 30/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#include "parse_function_def.h"

#include "parse_expression.h"
#include "text_parser.h"
#include "statements.h"
#include "floyd_parser.h"
#include "parser_primitives.h"

#include <cmath>

namespace floyd_parser {
	using std::pair;
	using std::string;
	using std::vector;
	using std::shared_ptr;
	using std::make_shared;

	/*
		()
		(int a)
		(int x, int y)
	*/
vector<member_t> parse_functiondef_arguments(const string& s2){
	const auto s(s2.substr(1, s2.length() - 2));
	vector<member_t> args;
	auto str = s;
	while(!str.empty()){
		const auto arg_type = read_type(str);
		const auto arg_name = read_required_single_symbol(arg_type.second);
		const auto optional_comma = read_optional_char(arg_name.second, ',');

		const auto a = member_t{ type_identifier_t::make(arg_type.first), arg_name.first };
		args.push_back(a);
		str = skip_whitespace(optional_comma.second);
	}

	trace_vec("parsed arguments:", args);
	return args;
}

QUARK_UNIT_TEST("", "", "", ""){
	QUARK_TEST_VERIFY((parse_functiondef_arguments("()") == vector<member_t>{}));
}

QUARK_UNIT_TEST("", "", "", ""){
	const auto r = parse_functiondef_arguments("(int x, string y, float z)");
	QUARK_TEST_VERIFY((r == vector<member_t>{
		{ type_identifier_t::make_int(), "x" },
		{ type_identifier_t::make_string(), "y" },
		{ type_identifier_t::make_float(), "z" }
	}
	));
}


std::pair<scope_ref_t, std::string> parse_function_definition(const ast_t& ast, const string& pos){
	const auto return_type_pos = read_required_type_identifier(pos);
	const auto function_name_pos = read_required_single_symbol(return_type_pos.second);

	//	Skip whitespace.
	const auto rest = skip_whitespace(function_name_pos.second);

	if(!peek_compare_char(rest, '(')){
		throw std::runtime_error("expected function argument list enclosed by (),");
	}

	const auto arg_list_pos = get_balanced(rest);
	const auto args = parse_functiondef_arguments(arg_list_pos.first);
	const auto body_rest_pos = skip_whitespace(arg_list_pos.second);

	if(!peek_compare_char(body_rest_pos, '{')){
		throw std::runtime_error("expected function body enclosed by {}.");
	}
	const auto body_pos = get_balanced(body_rest_pos);
	const auto function_name = type_identifier_t::make(function_name_pos.first);

	{
		//	Makes both function-scope and its body-scope.
		const auto function_def1 = make_function_def(
			function_name,
			return_type_pos.first,
			args,
			executable_t({}),
			{},
			{}
		);

		auto function_body_def = resolve_function_type(function_def1->_types_collector, "___body");
		QUARK_ASSERT(function_body_def);

		//	temp will get all statements.
		const auto temp = read_statements_into_scope_def(ast, function_body_def, body_pos.first.substr(1, body_pos.first.size() - 2));

		const auto function_def2 = make_function_def(
			function_name,
			return_type_pos.first,
			args,
			temp.first->_executable,
			temp.first->_types_collector,
			temp.first->_members
		);

		return { function_def2, body_pos.second };
	}
}

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	try{
		const auto ast = ast_t();
		const auto result = parse_function_definition(ast, "int f()");
		QUARK_TEST_VERIFY(false);
	}
	catch(...){
	}
}

//??? Check that all function paths return a value.

QUARK_UNIT_TESTQ("parse_function_definition()", ""){
	const auto ast = ast_t();
	const auto result = parse_function_definition(ast, "int f(){ return 3; }");
	QUARK_TEST_VERIFY(result.first->_name == type_identifier_t::make("f"));
	QUARK_TEST_VERIFY(result.first->_return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY(result.first->_members.empty());
	QUARK_TEST_VERIFY(result.first->_executable._statements.size() == 1);
	QUARK_TEST_VERIFY(result.second == "");

	const auto body_f = resolve_function_type(result.first->_types_collector, "___body");
	QUARK_UT_VERIFY(body_f && body_f->check_invariant());
	QUARK_UT_VERIFY(body_f->_type == scope_def_t::k_subscope);
	QUARK_UT_VERIFY(body_f->_return_type.to_string() == "int");
	QUARK_UT_VERIFY(body_f->_executable._statements.size() == 1);
	QUARK_UT_VERIFY(body_f->_return_type == type_identifier_t::make_int());
}

QUARK_UNIT_TESTQ("parse_function_definition()", "Test many arguments of different types"){
	const auto ast = ast_t();
	const auto result = parse_function_definition(ast, "int printf(string a, float barry, int c){}");
	QUARK_TEST_VERIFY(result.first->_name == type_identifier_t::make("printf"));
	QUARK_TEST_VERIFY(result.first->_return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY((result.first->_members == vector<member_t>{
		{ type_identifier_t::make_string(), "a" },
		{ type_identifier_t::make_float(), "barry" },
		{ type_identifier_t::make_int(), "c" },
	}));
//	QUARK_TEST_VERIFY(result.first->_executable._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}

/*
QUARK_UNIT_TEST("", "parse_function_definition()", "Test exteme whitespaces", ""){
	const auto result = parse_function_definition("    int    printf   (   string    a   ,   float   barry  ,   int   c  )  {  }  ");
	QUARK_TEST_VERIFY(result.first.first == "printf");
	QUARK_TEST_VERIFY(result.first.second._return_type == type_identifier_t::make_int());
	QUARK_TEST_VERIFY((result.first.second._args == vector<member_t>{
		{ type_identifier_t::make_string(), "a" },
		{ type_identifier_t::make_float(), "barry" },
		{ type_identifier_t::make_int(), "c" },
	}));
	QUARK_TEST_VERIFY(result.first.second._body._statements.empty());
	QUARK_TEST_VERIFY(result.second == "");
}
*/

scope_ref_t make_test_function1(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_function1"),
		type_identifier_t::make_int(),
		{},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(100)))
		}),
		{},
		{}
	);
}

scope_ref_t make_test_function2(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_function2"),
		type_identifier_t::make_string(),
		{
			{ type_identifier_t::make_int(), "a" },
			{ type_identifier_t::make_float(), "b" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant("sdf")))
		}),
		{},
		{}
	);
}

scope_ref_t make_log_function(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_log_function"),
		type_identifier_t::make_float(),
		{
			{type_identifier_t::make_float(), "value"}
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(123.f)))
		}),
		{},
		{}
	);
}

scope_ref_t make_log2_function(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("test_log2_function"),
		type_identifier_t::make_float(),
		{
			{ type_identifier_t::make_string(), "s" },
			{ type_identifier_t::make_float(), "v" }
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(456.7f)))
		}),
		{},
		{}
	);
}

scope_ref_t make_return5(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("return5"),
		type_identifier_t::make_float(),
		{
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant(5)))
		}),
		{},
		{}
	);
}

scope_ref_t make_return_hello(scope_ref_t scope){
	return make_function_def(
		type_identifier_t::make("hello"),
		type_identifier_t::make_int(),
		{
		},
		executable_t({
			make_shared<statement_t>(make__return_statement(expression_t::make_constant("hello")))
		}),
		{},
		{}
	);
}


}	//	floyd_parser
