//
//  parser_value.h
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 26/07/16.
//  Copyright © 2016 Marcus Zetterquist. All rights reserved.
//

#ifndef parser_value_hpp
#define parser_value_hpp

#include "quark.h"
#include <vector>
#include <string>
#include <map>
#include "ast_basics.h"
#include "ast_typeid.h"


namespace floyd {
	struct body_t;
	struct value_t;
	struct value_ext_t;

#if DEBUG
	std::string make_value_debug_str(const value_t& v);
#endif


	//////////////////////////////////////////////////		struct_instance_t

	/*
		An instance of a struct-type = a value of this struct.
	*/
	struct struct_instance_t {
		public: struct_instance_t(const std::shared_ptr<const struct_definition_t>& def, const std::vector<value_t>& member_values) :
			_def(def),
			_member_values(member_values)
		{
			QUARK_ASSERT(_def && _def->check_invariant());

			QUARK_ASSERT(check_invariant());
		}
		public: bool check_invariant() const;
		public: bool operator==(const struct_instance_t& other) const;


		public: const std::shared_ptr<const struct_definition_t> _def;
		public: std::vector<value_t> _member_values;
	};


	//////////////////////////////////////////////////		function_definition_t


	struct function_definition_t {
		public: bool check_invariant() const {
			if(_host_function_id != 0){
				QUARK_ASSERT(!_body);
			}
			else{
				QUARK_ASSERT(_body);
			}
			return true;
		}


		const typeid_t _function_type;
		const std::vector<member_t> _args;
		const std::shared_ptr<body_t> _body;
		const int _host_function_id;
	};

	bool operator==(const function_definition_t& lhs, const function_definition_t& rhs);
	ast_json_t function_def_to_ast_json(const function_definition_t& v);
	const typeid_t& get_function_type(const function_definition_t& f);





	//////////////////////////////////////////////////		value_ext_t


	struct value_ext_t {
		public: bool check_invariant() const{
			QUARK_ASSERT(_rc > 0);
			QUARK_ASSERT(_type.check_invariant());
			QUARK_ASSERT(_typeid_value.check_invariant());

			const auto base_type = _type.get_base_type();
			if(base_type == base_type::k_string){
//				QUARK_ASSERT(_string);
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_json_value){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value != nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

				QUARK_ASSERT(_json_value->check_invariant());
			}

			else if(base_type == base_type::k_typeid){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
		//		QUARK_ASSERT(_typeid_value != typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

				QUARK_ASSERT(_typeid_value.check_invariant());
			}
			else if(base_type == base_type::k_struct){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct != nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);

				QUARK_ASSERT(_struct && _struct->check_invariant());
			}
			else if(base_type == base_type::k_vector){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
		//		QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_dict){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector_elements.empty());
		//		QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id == -1);
			}
			else if(base_type == base_type::k_function){
				QUARK_ASSERT(_string.empty());
				QUARK_ASSERT(_json_value == nullptr);
				QUARK_ASSERT(_typeid_value == typeid_t::make_null());
				QUARK_ASSERT(_struct == nullptr);
				QUARK_ASSERT(_vector_elements.empty());
				QUARK_ASSERT(_dict_entries.empty());
				QUARK_ASSERT(_function_id != -1);
			}
			else {
				QUARK_ASSERT(false);
			}
			return true;
		}

		public: bool operator==(const value_ext_t& other) const;



		public: value_ext_t(const std::string& s) :
			_rc(1),
			_type(typeid_t::make_string()),
			_string(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_ext_t(const std::shared_ptr<json_t>& s) :
			_rc(1),
			_type(typeid_t::make_json_value()),
			_json_value(s)
		{
			QUARK_ASSERT(check_invariant());
		}

		public: value_ext_t(const typeid_t& s);

		public: value_ext_t(const typeid_t& type, std::shared_ptr<struct_instance_t>& s);
		public: value_ext_t(const typeid_t& type, const std::vector<value_t>& s);
		public: value_ext_t(const typeid_t& type, const std::map<std::string, value_t>& s);
		public: value_ext_t(const typeid_t& type, int function_id);



		public: int _rc;
		public: typeid_t _type;
		public: std::string _string;
		public: std::shared_ptr<json_t> _json_value;
		public: typeid_t _typeid_value = typeid_t::make_null();
		public: std::shared_ptr<struct_instance_t> _struct;
		public: std::vector<value_t> _vector_elements;
		public: std::map<std::string, value_t> _dict_entries;
		public: int _function_id = -1;
	};



	//////////////////////////////////////////////////		value_t

	/*
		Hold a value with an explicit type.
		Immutable

		- Values of basic types like ints and strings
		- Struct instances
		- Vector instances
		- Dictionary instances
		- Function "pointers"

		NOTICE: Encoding is very inefficient at the moment.
	*/

	struct value_t {

		//////////////////////////////////////////////////		PUBLIC - SPECIFIC TO TYPE

		public: value_t() :
			_basetype(base_type::k_null)
		{
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		//	Used internally in check_invariant() -- don't call check_invariant().
		public: typeid_t get_type() const;
		public: base_type get_basetype() const{
			return _basetype;
		}


		//------------------------------------------------		null


		public: static value_t make_null();
		public: bool is_null() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_null;
		}


		//------------------------------------------------		bool


		public: static value_t make_bool(bool value);
		public: bool is_bool() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_bool;
		}
		public: bool get_bool_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_bool()){
				throw std::runtime_error("Type mismatch!");
			}

			return _value_internals._bool;
		}
		public: bool get_bool_value_quick() const{
			return _value_internals._bool;
		}


		//------------------------------------------------		int


static value_t make_int(int value){
	return value_t(value);
}
		public: bool is_int() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_int;
		}
		public: int get_int_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_int()){
				throw std::runtime_error("Type mismatch!");
			}

			return _value_internals._int;
		}

		public: int get_int_value_quick() const{
			return _value_internals._int;
		}

		//------------------------------------------------		float


		public: static value_t make_float(float value);
		public: bool is_float() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_float;
		}
		public: float get_float_value() const{
			QUARK_ASSERT(check_invariant());
			if(!is_float()){
				throw std::runtime_error("Type mismatch!");
			}

			return _value_internals._float;
		}


		//------------------------------------------------		string


		public: static value_t make_string(const std::string& value){
			return value_t(value);
		}
		public: static inline value_t make_string(const char value[]){
			return make_string(std::string(value));
		}
		public: bool is_string() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_string;
		}
		public: std::string get_string_value() const;


		//------------------------------------------------		json_value


		public: static value_t make_json_value(const json_t& v);
		public: bool is_json_value() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_json_value;
		}
		public: json_t get_json_value() const;


		//------------------------------------------------		typeid


		public: static value_t make_typeid_value(const typeid_t& type_id);
		public: bool is_typeid() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_typeid;
		}
		public: typeid_t get_typeid_value() const;


		//------------------------------------------------		struct


		public: static value_t make_struct_value(const typeid_t& struct_type, const std::vector<value_t>& values);
		public: bool is_struct() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_struct;
		}
		public: std::shared_ptr<struct_instance_t> get_struct_value() const;


		//------------------------------------------------		vector


		public: static value_t make_vector_value(const typeid_t& element_type, const std::vector<value_t>& elements);
		public: bool is_vector() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_vector;
		}
		public: const std::vector<value_t>& get_vector_value() const;


		//------------------------------------------------		dict


		public: static value_t make_dict_value(const typeid_t& value_type, const std::map<std::string, value_t>& entries);
		public: bool is_dict() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_dict;
		}
		public: const std::map<std::string, value_t>& get_dict_value() const;


		//------------------------------------------------		function


		public: static value_t make_function_value(const typeid_t& function_type, int function_id);
		public: bool is_function() const {
			QUARK_ASSERT(check_invariant());

			return _basetype == base_type::k_function;
		}
		public: int get_function_value() const;


		//////////////////////////////////////////////////		PUBLIC - TYPE INDEPENDANT

bool is_ext_slow(base_type basetype) const{
	return false
		|| basetype == base_type::k_string
		|| basetype == base_type::k_json_value
		|| basetype == base_type::k_typeid
		|| basetype == base_type::k_struct
		|| basetype == base_type::k_vector
		|| basetype == base_type::k_dict
		|| basetype == base_type::k_function;
}

bool is_ext(base_type basetype) const{
/*
		k_null,
		k_bool,
		k_int,
		k_float,
*/
	bool ext = basetype > base_type::k_float;

	//	Make sure above assumtion about order of base types is valid.
	QUARK_ASSERT(ext == is_ext_slow(basetype));
	return ext;
}

		public: bool check_invariant() const;

		public: value_t(const value_t& other):
			_basetype(other._basetype),
			_value_internals(other._value_internals)
		{
			QUARK_ASSERT(other.check_invariant());

			if(is_ext(_basetype)){
				_value_internals._ext->_rc++;
			}

#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		public: ~value_t(){
			QUARK_ASSERT(check_invariant());

			if(is_ext(_basetype)){
				_value_internals._ext->_rc--;
				if(_value_internals._ext->_rc == 0){
					delete _value_internals._ext;
				}
				_value_internals._ext = nullptr;
			}
		}

		public: value_t& operator=(const value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

			value_t temp(other);
			temp.swap(*this);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
			return *this;
		}

		public: bool operator==(const value_t& other) const{
			QUARK_ASSERT(check_invariant());
			QUARK_ASSERT(other.check_invariant());

			if(!(_basetype == other._basetype)){
				return false;
			}

			if(_basetype == base_type::k_null){
				return true;
			}
			else if(_basetype == base_type::k_bool){
				return _value_internals._bool == other._value_internals._bool;
			}
			else if(_basetype == base_type::k_int){
				return _value_internals._int == other._value_internals._int;
			}
			else if(_basetype == base_type::k_float){
				return _value_internals._float == other._value_internals._float;
			}
			else{
				QUARK_ASSERT(is_ext(_basetype));
				return compare_shared_values(_value_internals._ext, other._value_internals._ext);
			}
		}

		public: bool operator!=(const value_t& other) const{
			return !(*this == other);
		}

		/*
			diff == 0: equal
			diff == 1: left side is bigger
			diff == -1: right side is bigger.

			Think (left_struct - right_struct).

			This technique lets us do most comparison operations *ontop* of compare_value_true_deep() with
			only a single compare function.
		*/
		public: static int compare_value_true_deep(const value_t& left, const value_t& right);

		public: void swap(value_t& other){
			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());

#if DEBUG
			std::swap(DEBUG_STR, other.DEBUG_STR);
#endif

			std::swap(_basetype, other._basetype);

			std::swap(_value_internals, other._value_internals);

			QUARK_ASSERT(other.check_invariant());
			QUARK_ASSERT(check_invariant());
		}


		//////////////////////////////////////////////////		INTERNALS


		private: explicit value_t(bool value) :
			_basetype(base_type::k_bool)
		{
			_value_internals._bool = value;
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(int value) :
			_basetype(base_type::k_int)
		{
			_value_internals._int = value;
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: value_t(float value) :
			_basetype(base_type::k_float)
		{
			_value_internals._float = value;
#if DEBUG
			DEBUG_STR = make_value_debug_str(*this);
#endif

			QUARK_ASSERT(check_invariant());
		}

		private: explicit value_t(const char s[]);
		private: explicit value_t(const std::string& s);

		private: explicit value_t(const std::shared_ptr<json_t>& s);
		private: explicit value_t(const typeid_t& type);
		private: explicit value_t(const typeid_t& struct_type, std::shared_ptr<struct_instance_t>& instance);
		private: explicit value_t(const typeid_t& element_type, const std::vector<value_t>& elements);
		private: explicit value_t(const typeid_t& value_type, const std::map<std::string, value_t>& entries);
		private: explicit value_t(const typeid_t& type, int function_id);


		//////////////////////////////////////////////////		STATE

		private: union value_internals_t {
			bool _bool;
			int _int;
			float _float;
			value_ext_t* _ext;
		};

#if DEBUG
		private: std::string DEBUG_STR;
#endif
		private: base_type _basetype;
		private: value_internals_t _value_internals;
	};



	//////////////////////////////////////////////////		Helpers


	/*
		"true"
		"0"
		"1003"
		"Hello, world"
		Notice, strings don't get wrapped in "".
	*/
	std::string to_compact_string2(const value_t& value);

	//	Special handling of strings, we want to wrap in "".
	std::string to_compact_string_quote_strings(const value_t& value);

	/*
		bool: "true"
		int: "0"
		string: "1003"
		string: "Hello, world"
	*/
	std::string value_and_type_to_string(const value_t& value);

	ast_json_t value_to_ast_json(const value_t& v);
	ast_json_t value_and_type_to_ast_json(const value_t& v);

	json_t values_to_json_array(const std::vector<value_t>& values);


}	//	floyd

#endif /* parser_value_hpp */
