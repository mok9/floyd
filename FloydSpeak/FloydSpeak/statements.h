//
//  parser_statement.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_statement_hpp
#define parser_statement_hpp


#include "quark.h"
#include <vector>
#include <string>
#include <map>

#include "parser_ast.h"
#include "expressions.h"
#include "utils.h"

struct json_t;

namespace floyd_ast {
	struct statement_t;
	struct expression_t;


/*
	//////////////////////////////////////		funcdef_statement_t


	struct function_definition_statement_t {
		bool operator==(const funcdef_statement_t& other) const {
			return
				_name == other._name
				&& _args == other._args
				&& compare_shared_value_vectors(_statements, other._statements)
				&& _return_type == other._return_type;
		}

		std::string _name;
		std::vector<member_t> _args;
		std::vector<std::shared_ptr<statement_t>> _statements;
		typeid_t _return_type;
	};
*/


	//////////////////////////////////////		return_statement_t


	struct return_statement_t {
		bool operator==(const return_statement_t& other) const {
			return _expression == other._expression;
		}

		expression_t _expression;
	};


	//////////////////////////////////////		bind_statement_t


	struct bind_statement_t {
		bool operator==(const bind_statement_t& other) const {
			return _new_variable_name == other._new_variable_name && _bindtype == other._bindtype && _expression == other._expression;
		}

		std::string _new_variable_name;
		typeid_t _bindtype;
		expression_t _expression;
	};


	//////////////////////////////////////		block_statement_t


	struct block_statement_t {
		bool operator==(const block_statement_t& other) const {
			return compare_shared_value_vectors(_statements, other._statements);
		}

		std::vector<std::shared_ptr<statement_t>> _statements;
	};


	//////////////////////////////////////		for_statement_t


	struct for_statement_t {
		bool operator==(const for_statement_t& other) const {
			return compare_shared_values(_init, other._init) && _condition == other._condition && _post_expression == other._post_expression && _block_id == other._block_id;
		}


		std::shared_ptr<statement_t> _init;
		expression_t _condition;
		expression_t _post_expression;
		int _block_id;
	};



	//////////////////////////////////////		statement_t

	/*
		Defines a statement, like "return" including any needed expression trees for the statement.
	*/
	struct statement_t {
		public: statement_t(const statement_t& other) = default;
		public: statement_t& operator=(const statement_t& other) = default;
		public: bool check_invariant() const;

        public: statement_t(const return_statement_t& value) :
			_return(std::make_shared<return_statement_t>(value))
		{
		}
        public: statement_t(const bind_statement_t& value) :
			_bind(std::make_shared<bind_statement_t>(value))
		{
		}
        public: statement_t(const block_statement_t& value) :
			_block(std::make_shared<block_statement_t>(value))
		{
		}
        public: statement_t(const for_statement_t& value) :
			_for(std::make_shared<for_statement_t>(value))
		{
		}

		public: bool operator==(const statement_t& other) const {
			if(_return){
				return other._return && *_return == *other._return;
			}
			else if(_bind){
				return other._bind && *_bind == *other._bind;
			}
			else if(_block){
				return other._block && *_block == *other._block;
			}
			else if(_for){
				return other._for && *_for == *other._for;
			}
			else{
				QUARK_ASSERT(false);
				return false;
			}
		}

		public: std::shared_ptr<return_statement_t> _return;
		public: std::shared_ptr<bind_statement_t> _bind;
		public: std::shared_ptr<block_statement_t> _block;
		public: std::shared_ptr<for_statement_t> _for;
	};


	//////////////////////////////////////		Makers



			statement_t make__return_statement(const return_statement_t& value);
	statement_t make__return_statement(const expression_t& expression);
	statement_t make__bind_statement(const std::string& new_variable_name, const typeid_t& bindtype, const expression_t& expression);
	statement_t make__block_statement(const std::vector<std::shared_ptr<statement_t>>& statements);
	statement_t make__for_statement(const statement_t& init, const expression_t& condition, const expression_t& post_expression, int block_id);


	void trace(const statement_t& s);
	json_t statement_to_json(const statement_t& e);
	json_t statements_to_json(const std::vector<std::shared_ptr<statement_t>>& e);

}	//	floyd_ast


#endif /* parser_statement_hpp */
