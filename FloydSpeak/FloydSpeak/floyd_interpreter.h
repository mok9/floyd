//
//  parser_evaluator.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef floyd_interpreter_hpp
#define floyd_interpreter_hpp


#include "quark.h"

#include <string>
#include <vector>
#include <map>
#include "ast_value.h"
#include "host_functions.hpp"
#include "bytecode_gen.h"

namespace floyd {
	struct expression_t;
	struct bc_value_t;
	struct statement_t;
	struct interpreter_t;
	struct bc_program_t;
	struct bc_instruction_t;


	bc_value_t construct_value_from_typeid(interpreter_t& vm, const typeid_t& type, const typeid_t& arg0_type, const std::vector<bc_value_t>& arg_values);







	//////////////////////////////////////		interpret_stack_element_t



	struct interpret_stack_element_t {
		private: union value_internals_t {
			bool _bool;
			int _int;
			float _float;
			int _function_id;
			bc_value_object_t* _ext;
		};
		public: value_internals_t _internals;
	};


inline interpret_stack_element_t make_object_element(const bc_value_t& value, const typeid_t& type){
	interpret_stack_element_t temp;
	value._value_internals._ext->_rc++;
	temp._internals._ext = value._value_internals._ext;
	return temp;
}

inline interpret_stack_element_t make_element(const bc_value_t& value, const typeid_t& type){
	const auto basettype = type.get_base_type();
	if(basettype == base_type::k_internal_undefined){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_internal_dynamic){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_void){
		interpret_stack_element_t temp;
		temp._internals._ext = nullptr;
		return temp;
	}
	else if(basettype == base_type::k_bool){
		interpret_stack_element_t temp;
		temp._internals._bool = value.get_bool_value();
		return temp;
	}
	else if(basettype == base_type::k_int){
		interpret_stack_element_t temp;
		temp._internals._int = value.get_int_value();
		return temp;
	}
	else if(basettype == base_type::k_float){
		interpret_stack_element_t temp;
		temp._internals._float = value.get_float_value();
		return temp;
	}
	else if(basettype == base_type::k_function){
		interpret_stack_element_t temp;
		temp._internals._function_id = value.get_function_value();
		return temp;
	}

	else if(bc_value_t::is_bc_ext(basettype)){
		return make_object_element(value, type);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}
//???? stupid-slow. Also not needed really.
inline bc_value_t element_to_value(const interpret_stack_element_t& e, const typeid_t& type){
	if(type.is_undefined()){
		return bc_value_t::make_undefined();
	}
	else if(type.is_internal_dynamic()){
		return bc_value_t::make_internal_dynamic();
	}
	else if(type.is_void()){
		return bc_value_t::make_void();
	}
	else if(type.is_bool()){
		return bc_value_t::make_bool(e._internals._bool);
	}
	else if(type.is_int()){
		return bc_value_t::make_int(e._internals._int);
	}
	else if(type.is_float()){
		return bc_value_t::make_float(e._internals._float);
	}
	else if(type.is_string()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_json_value()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_typeid()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_struct()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_vector()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_dict()){
		return bc_value_t::make_object(e._internals._ext);
	}
	else if(type.is_function()){
		return bc_value_t::make_function_value(type, e._internals._function_id);
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





	//////////////////////////////////////		interpreter_stack_t


	struct interpreter_stack_t {
		public: interpreter_stack_t(){
			_value_stack.reserve(1024);
		}
		public: ~interpreter_stack_t(){
		}

		public: interpreter_stack_t(const interpreter_stack_t& other) :
			_value_stack(other._value_stack)
		{
			//??? Cannot bump RC:s because we don't know which values!
			QUARK_ASSERT(false);
		}

		public: interpreter_stack_t& operator=(const interpreter_stack_t& other){
			interpreter_stack_t temp = other;
			temp.swap(*this);
			return *this;
		}

		public: void swap(interpreter_stack_t& other) throw() {
			other._value_stack.swap(_value_stack);
		}

		int size() const {
			return static_cast<int>(_value_stack.size());
		}
		inline void push_value(const bc_value_t& value, const typeid_t& type){
			const auto e = make_element(value, type);
			_value_stack.push_back(e);
		}
		inline void push_intq(int value){
			interpret_stack_element_t e;
			e._internals._int = value;
			_value_stack.push_back(e);
		}

		inline bc_value_t load_value(int pos, const typeid_t& type) const{
			const auto& e = _value_stack[pos];
			const auto result = element_to_value(e, type);
			return result;
		}

		inline int load_intq(int pos) const{
			return _value_stack[pos]._internals._int;
		}

		inline void replace_value_same_type(int pos, const bc_value_t& value, const typeid_t& type){
			//	Unpack existing value so it's RC:ed OK.
			const auto prev_value = element_to_value(_value_stack[pos], type);

			const auto e = make_element(value, type);
			_value_stack[pos] = e;
		}

		inline void replace_int(int pos, const int& value){
			_value_stack[pos]._internals._int = value;
		}

		inline void pop_value(const typeid_t& type){
			//	Unpack existing value so it's RC:ed OK.
			const auto prev_value = element_to_value(_value_stack.back(), type);
			_value_stack.pop_back();
		}


		public: std::vector<interpret_stack_element_t> _value_stack;
	};



	//////////////////////////////////////		statement_result_t



	struct interpreter_context_t {
		public: quark::trace_context_t _tracer;
	};


	//////////////////////////////////////		statement_result_t


	struct statement_result_t {
		enum output_type {

			//	_output != nullptr
			k_returning,

			//	_output != nullptr
			k_passive_expression_output,


			//	_output == nullptr
			k_complete_without_result_value
		};

		public: static statement_result_t make_return_unwind(const bc_value_t& return_value){
			return { statement_result_t::k_returning, return_value };
		}
		public: static statement_result_t make_passive_expression_output(const bc_value_t& output_value){
			return { statement_result_t::k_passive_expression_output, output_value };
		}
		public: static statement_result_t make__complete_without_value(){
			return { statement_result_t::k_complete_without_result_value, bc_value_t::make_undefined() };
		}

		private: statement_result_t(output_type type, const bc_value_t& output) :
			_type(type),
			_output(output)
		{
		}

		public: output_type _type;
		public: bc_value_t _output;
	};

/*
	inline bool operator==(const statement_result_t& lhs, const statement_result_t& rhs){
		return true
			&& lhs._type == rhs._type
			&& lhs._output == rhs._output;
	}
*/



	//////////////////////////////////////		environment_t

	/*
		Runtime scope, similar to a stack frame.
	*/

	struct environment_t {
//		public: const bc_body_t* _body_ptr;
		public: std::vector<environment_t>::size_type _values_offset;
	};


	//////////////////////////////////////		interpreter_imm_t


	struct interpreter_imm_t {
		////////////////////////		STATE
		public: const std::chrono::time_point<std::chrono::high_resolution_clock> _start_time;
		public: const bc_program_t _program;
		public: const std::map<int, HOST_FUNCTION_PTR> _host_functions;
	};


	//////////////////////////////////////		interpreter_t

	/*
		Complete runtime state of the interpreter.
		MUTABLE
	*/

	struct interpreter_t {
		public: explicit interpreter_t(const bc_program_t& program);
		public: interpreter_t(const interpreter_t& other) = delete;
		public: const interpreter_t& operator=(const interpreter_t& other)= delete;
#if DEBUG
		public: bool check_invariant() const;
#endif
		public: void swap(interpreter_t& other) throw();

		////////////////////////		STATE
		public: std::shared_ptr<interpreter_imm_t> _imm;

		public: bc_value_t _internal_placeholder_object;


		//	Holds all values for all environments.
		//	Notice: stack holds refs to RC-counted objects!
		public: interpreter_stack_t _value_stack;
		public: int _current_stack_frame;
		public: std::vector<std::string> _print_output;
	};


	value_t call_host_function(interpreter_t& vm, int function_id, const std::vector<value_t>& args);
	value_t call_function(interpreter_t& vm, const value_t& f, const std::vector<value_t>& args);
	json_t interpreter_to_json(const interpreter_t& vm);


	/*
		Return value:
			null = statements were all executed through.
			value = return statement returned a value.
	*/
	statement_result_t execute_statements(interpreter_t& vm, const std::vector<bc_instruction_t>& statements);




	//////////////////////////		run_main()

	struct value_entry_t {
		bc_value_t _value;
		std::string _symbol_name;
		symbol_t _symbol;
		variable_address_t _address;
	};

	value_t find_global_symbol(const interpreter_t& vm, const std::string& s);
	value_t get_global(const interpreter_t& vm, const std::string& name);
	std::shared_ptr<value_entry_t> find_global_symbol2(const interpreter_t& vm, const std::string& s);

	/*
		Quickie that compiles a program and calls its main() with the args.
	*/
	std::pair<std::shared_ptr<interpreter_t>, value_t> run_main(
		const interpreter_context_t& context,
		const std::string& source,
		const std::vector<value_t>& args
	);

	std::pair<std::shared_ptr<interpreter_t>, value_t> run_program(const interpreter_context_t& context, const bc_program_t& program, const std::vector<floyd::value_t>& args);

	bc_program_t program_to_ast2(const interpreter_context_t& context, const std::string& program);


	std::shared_ptr<interpreter_t> run_global(const interpreter_context_t& context, const std::string& source);

	void print_vm_printlog(const interpreter_t& vm);

} //	floyd


#endif /* floyd_interpreter_hpp */
