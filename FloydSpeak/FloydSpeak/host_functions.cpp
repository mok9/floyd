//
//  host_functions.cpp
//  FloydSpeak
//
//  Created by Marcus Zetterquist on 2018-02-23.
//  Copyright © 2018 Marcus Zetterquist. All rights reserved.
//

#include "host_functions.hpp"



#include "parser_primitives.h"
#include "parse_expression.h"
#include "parse_statement.h"
#include "statement.h"
#include "floyd_parser.h"
#include "ast_value.h"
#include "pass2.h"
#include "pass3.h"
#include "json_support.h"
#include "json_parser.h"

#include <cmath>
#include <sys/time.h>

#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <fstream>
#include "text_parser.h"

#include "floyd_interpreter.h"

namespace floyd {

using std::vector;
using std::string;
using std::pair;
using std::shared_ptr;
using std::unique_ptr;
using std::make_shared;





value_t flatten_to_json(const value_t& value){
	const auto j = value_to_ast_json(value);
	value_t json_value = value_t::make_json_value(j._value);
	return json_value;
}




value_t unflatten_json_to_specific_type(const json_t& v, const typeid_t& target_type){
	QUARK_ASSERT(v.check_invariant());

	if(target_type.is_null()){
		throw std::runtime_error("Invalid json schema, found null - unsupported by Floyd.");
	}
	else if(target_type.is_bool()){
		if(v.is_true()){
			return value_t::make_bool(true);
		}
		else if(v.is_false()){
			return value_t::make_bool(false);
		}
		else{
			throw std::runtime_error("Invalid json schema, expected true or false.");
		}
	}
	else if(target_type.is_int()){
		if(v.is_number()){
			return value_t::make_int((int)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_float()){
		if(v.is_number()){
			return value_t::make_float((float)v.get_number());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected number.");
		}
	}
	else if(target_type.is_string()){
		if(v.is_string()){
			return value_t::make_string(v.get_string());
		}
		else{
			throw std::runtime_error("Invalid json schema, expected string.");
		}
	}
	else if(target_type.is_json_value()){
		return value_t::make_json_value(v);
	}
	else if(target_type.is_typeid()){
		const auto typeid_value = typeid_from_ast_json(ast_json_t({v}));
		return value_t::make_typeid_value(typeid_value);
	}
	else if(target_type.is_struct()){
		if(v.is_object()){
			const auto struct_def = target_type.get_struct();
			vector<value_t> members2;
			for(const auto member: struct_def._members){
				const auto member_value0 = v.get_object_element(member._name);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, member._type);
				members2.push_back(member_value1);
			}
			const auto result = value_t::make_struct_value(target_type, members2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd struct, expected JSON object.");
		}
	}
	else if(target_type.is_vector()){
		if(v.is_array()){
			const auto target_element_type = target_type.get_vector_element_type();
			vector<value_t> elements2;
			for(int i = 0 ; i < v.get_array_size() ; i++){
				const auto member_value0 = v.get_array_n(i);
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, target_element_type);
				elements2.push_back(member_value1);
			}
			const auto result = value_t::make_vector_value(target_element_type, elements2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema for Floyd vector, expected JSON array.");
		}
	}
	else if(target_type.is_dict()){
		if(v.is_object()){
			const auto value_type = target_type.get_dict_value_type();
			const auto source_obj = v.get_object();
			std::map<std::string, value_t> obj2;
			for(const auto member: source_obj){
				const auto member_name = member.first;
				const auto member_value0 = member.second;
				const auto member_value1 = unflatten_json_to_specific_type(member_value0, value_type);
				obj2[member_name] = member_value1;
			}
			const auto result = value_t::make_dict_value(value_type, obj2);
			return result;
		}
		else{
			throw std::runtime_error("Invalid json schema, expected JSON object.");
		}
	}
	else if(target_type.is_function()){
		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else if(target_type.is_unresolved_type_identifier()){
		QUARK_ASSERT(false);
		throw std::exception();
//		throw std::runtime_error("Invalid json schema, cannot unflatten functions.");
	}
	else{
		QUARK_ASSERT(false);
		throw std::exception();
	}
}





//	Records all output to interpreter
std::pair<interpreter_t, value_t> host__print(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	auto vm2 = vm;
	const auto& value = args[0];

#if 0
	if(value.is_struct() && value.get_struct_value()->_def == *json_value___struct_def){
		const auto s = json_value__to_compact_string(value);
		vm2._print_output.push_back(s);
	}
	else
#endif
	{
		const auto s = to_compact_string2(value);
		printf("%s\n", s.c_str());
		vm2._print_output.push_back(s);
	}

	return {vm2, value_t::make_null() };
}

std::pair<interpreter_t, value_t> host__assert(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("assert() requires 1 argument!");
	}

	auto vm2 = vm;
	const auto& value = args[0];
	if(value.is_bool() == false){
		throw std::runtime_error("First argument to assert() must be of type bool.");
	}
	bool ok = value.get_bool_value();
	if(!ok){
		vm2._print_output.push_back("Assertion failed.");
		throw std::runtime_error("Floyd assertion failed.");
	}
	return {vm2, value_t::make_null() };
}

//	string to_string(value_t)
std::pair<interpreter_t, value_t> host__to_string(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto a = to_compact_string2(value);
	return {vm, value_t::make_string(a) };
}
std::pair<interpreter_t, value_t> host__to_pretty_string(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("to_pretty_string() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto json = value_to_ast_json(value);
	const auto s = json_to_pretty_string(json._value, 0, pretty_t{80, 4});
	return {vm, value_t::make_string(s) };
}

std::pair<interpreter_t, value_t> host__typeof(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("typeof() requires 1 argument!");
	}

	const auto& value = args[0];
	const auto type = value.get_type();
	const auto result = value_t::make_typeid_value(type);
	return {vm, result };
}

std::pair<interpreter_t, value_t> host__get_time_of_day(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_time_of_day() requires 0 arguments!");
	}

	std::chrono::time_point<std::chrono::high_resolution_clock> t = std::chrono::high_resolution_clock::now();
	std::chrono::duration<double> elapsed_seconds = t - vm._start_time;
	const auto ms = elapsed_seconds.count() * 1000.0;
	return {vm, value_t::make_int(int(ms)) };
}

QUARK_UNIT_TESTQ("sizeof(int)", ""){
	QUARK_TRACE(std::to_string(sizeof(int)));
	QUARK_TRACE(std::to_string(sizeof(int64_t)));
}

QUARK_UNIT_TESTQ("get_time_of_day_ms()", ""){
	const auto a = std::chrono::high_resolution_clock::now();
    std::this_thread::sleep_for(std::chrono::milliseconds(7));
	const auto b = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> elapsed_seconds = b - a;
	const int ms = static_cast<int>((static_cast<double>(elapsed_seconds.count()) * 1000.0));

	QUARK_UT_VERIFY(ms >= 7)
}






	value_t update_struct_member_shallow(const interpreter_t& vm, const value_t& obj, const std::string& member_name, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(member_name.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		const auto s = obj.get_struct_value();
		const auto def = s->_def;

		int member_index = find_struct_member_index(def, member_name);
		if(member_index == -1){
			throw std::runtime_error("Unknown member.");
		}

		const auto struct_typeid = obj.get_type();
		const auto values = s->_member_values;


		QUARK_TRACE(typeid_to_compact_string(new_value.get_type()));
		QUARK_TRACE(typeid_to_compact_string(def._members[member_index]._type));

		const auto dest_member_entry = def._members[member_index];
		auto dest_member_resolved_type = dest_member_entry._type;
		dest_member_resolved_type = resolve_type_using_env(vm, dest_member_entry._type);

		if(!(new_value.get_type() == dest_member_resolved_type)){
			throw std::runtime_error("Value type not matching struct member type.");
		}

		auto values2 = values;
		values2[member_index] = new_value;

		auto s2 = value_t::make_struct_value(struct_typeid, values2);
		return s2;
	}

	value_t update_struct_member_deep(const interpreter_t& vm, const value_t& obj, const std::vector<std::string>& path, const value_t& new_value){
		QUARK_ASSERT(obj.check_invariant());
		QUARK_ASSERT(path.empty() == false);
		QUARK_ASSERT(new_value.check_invariant());

		if(path.size() == 1){
			return update_struct_member_shallow(vm, obj, path[0], new_value);
		}
		else{
			vector<string> subpath = path;
			subpath.erase(subpath.begin());

			const auto s = obj.get_struct_value();
			const auto def = s->_def;
			int member_index = find_struct_member_index(def, path[0]);
			if(member_index == -1){
				throw std::runtime_error("Unknown member.");
			}

			const auto child_value = s->_member_values[member_index];
			if(child_value.is_struct() == false){
				throw std::runtime_error("Value type not matching struct member type.");
			}

			const auto child2 = update_struct_member_deep(vm, child_value, subpath, new_value);
			const auto obj2 = update_struct_member_shallow(vm, obj, path[0], child2);
			return obj2;
		}
	}

std::pair<interpreter_t, value_t> host__update(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	QUARK_TRACE(json_to_pretty_string(interpreter_to_json(vm)));

	const auto& obj1 = args[0];
	const auto& lookup_key = args[1];
	const auto& new_value = args[2];

	if(args.size() != 3){
		throw std::runtime_error("update() needs 3 arguments.");
	}
	else{
		if(obj1.is_string()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("String lookup using integer index only.");
			}
			else{
				const auto obj = obj1;
				const auto v = obj.get_string_value();

				if((new_value.get_type().is_string() && new_value.get_string_value().size() == 1) == false){
					throw std::runtime_error("Update element must be a 1-character string.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v.size()){
						throw std::runtime_error("String lookup out of bounds.");
					}
					else{
						string v2 = v;
						v2[lookup_index] = new_value.get_string_value()[0];
						const auto s2 = value_t::make_string(v2);
						return {vm, s2 };
					}
				}
			}
		}
		else if(obj1.is_json_value()){
			const auto json_value0 = obj1.get_json_value();
			if(json_value0.is_array()){
				assert(false);
			}
			else if(json_value0.is_object()){
				assert(false);
			}
			else{
				throw std::runtime_error("Can only update string, vector, dict or struct.");
			}
		}
		else if(obj1.is_vector()){
			if(lookup_key.is_int() == false){
				throw std::runtime_error("Vector lookup using integer index only.");
			}
			else{
				const auto obj = obj1;
				const auto v = obj.get_vector_value();
				auto v2 = v->_elements;

				if((v->_element_type == new_value.get_type()) == false){
					throw std::runtime_error("Update element must match vector type.");
				}
				else{
					const int lookup_index = lookup_key.get_int_value();
					if(lookup_index < 0 || lookup_index >= v->_elements.size()){
						throw std::runtime_error("Vector lookup out of bounds.");
					}
					else{
						v2[lookup_index] = new_value;
						const auto s2 = value_t::make_vector_value(v->_element_type, v2);
						return {vm, s2 };
					}
				}
			}
		}
		else if(obj1.is_dict()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("Dict lookup using string key only.");
			}
			else{
				const auto obj = obj1;
				const auto v = obj.get_dict_value();
				auto v2 = v->_elements;

				if((v->_value_type == new_value.get_type()) == false){
					throw std::runtime_error("Update element must match dict value type.");
				}
				else{
					const string key = lookup_key.get_string_value();
					auto elements2 = v->_elements;
					elements2[key] = new_value;
					const auto value2 = value_t::make_dict_value(obj.get_dict_value()->_value_type, elements2);
					return { vm, value2 };
				}
			}
		}
		else if(obj1.is_struct()){
			if(lookup_key.is_string() == false){
				throw std::runtime_error("You must specify structure member using string.");
			}
			else{
				const auto obj = obj1;

				//### Does simple check for "." -- we should use vector of strings instead.
				const auto nodes = split_on_chars(seq_t(lookup_key.get_string_value()), ".");
				if(nodes.empty()){
					throw std::runtime_error("You must specify structure member using string.");
				}

				const auto s2 = update_struct_member_deep(vm, obj, nodes, new_value);
				return {vm, s2 };
			}
		}
		else {
			throw std::runtime_error("Can only update string, vector, dict or struct.");
		}
	}
}

std::pair<interpreter_t, value_t> host__size(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	if(obj.is_string()){
		const auto size = obj.get_string_value().size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else if(obj.is_json_value()){
		const auto value = obj.get_json_value();
		if(value.is_object()){
			const auto size = value.get_object_size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else if(value.is_array()){
			const auto size = value.get_array_size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else if(value.is_string()){
			const auto size = value.get_string().size();
			return {vm, value_t::make_int(static_cast<int>(size))};
		}
		else{
			throw std::runtime_error("Calling size() on unsupported type of value.");
		}
	}
	else if(obj.is_vector()){
		const auto size = obj.get_vector_value()->_elements.size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else if(obj.is_dict()){
		const auto size = obj.get_dict_value()->_elements.size();
		return {vm, value_t::make_int(static_cast<int>(size))};
	}
	else{
		throw std::runtime_error("Calling size() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__find(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto wanted = args[1];

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto wanted2 = wanted.get_string_value();

		const auto r = str.find(wanted2);
		if(r == std::string::npos){
			return {vm, value_t::make_int(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t::make_int(static_cast<int>(r))};
		}
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		if(wanted.get_type() != vec->_element_type){
			throw std::runtime_error("Type mismatch.");
		}
		const auto size = vec->_elements.size();
		int index = 0;
		while(index < size && vec->_elements[index] != wanted){
			index++;
		}
		if(index == size){
			return {vm, value_t::make_int(static_cast<int>(-1))};
		}
		else{
			return {vm, value_t::make_int(static_cast<int>(index))};
		}
	}
	else{
		throw std::runtime_error("Calling find() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__exists(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto v = obj.get_dict_value();

		const auto found_it = v->_elements.find(key_string);
		const bool exists = found_it != v->_elements.end();
		return {vm, value_t::make_bool(exists)};
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__erase(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto key = args[1];

	if(obj.is_dict()){
		if(key.get_type().is_string() == false){
			throw std::runtime_error("Key must be string.");
		}
		const auto key_string = key.get_string_value();
		const auto v = obj.get_dict_value();

		std::map<string, value_t> elements2 = v->_elements;
		elements2.erase(key_string);
		const auto value2 = value_t::make_dict_value(obj.get_dict_value()->_value_type, elements2);
		return { vm, value2 };
	}
	else{
		throw std::runtime_error("Calling exist() on unsupported type of value.");
	}
}




//	assert(push_back(["one","two"], "three") == ["one","two","three"])
std::pair<interpreter_t, value_t> host__push_back(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("find requires 2 arguments");
	}

	const auto obj = args[0];
	const auto element = args[1];
	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto ch = element.get_string_value();

		auto str2 = str + ch;
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		if(element.get_type() != vec->_element_type){
			throw std::runtime_error("Type mismatch.");
		}
		auto elements2 = vec->_elements;
		elements2.push_back(element);
		const auto v = value_t::make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}

//	assert(subset("abc", 1, 3) == "bc");
std::pair<interpreter_t, value_t> host__subset(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 3){
		throw std::runtime_error("subset() requires 2 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("subset() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("subset() requires start and end to be non-negative.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));

		string str2;
		for(int i = start2 ; i < end2 ; i++){
			str2.push_back(str[i]);
		}
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		vector<value_t> elements2;
		for(int i = start2 ; i < end2 ; i++){
			elements2.push_back(vec->_elements[i]);
		}
		const auto v = value_t::make_vector_value(vec->_element_type, elements2);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling push_back() on unsupported type of value.");
	}
}



//	assert(replace("One ring to rule them all", 4, 7, "rabbit") == "One rabbit to rule them all");
std::pair<interpreter_t, value_t> host__replace(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 4){
		throw std::runtime_error("replace() requires 4 arguments");
	}

	const auto obj = args[0];

	if(args[1].is_int() == false || args[2].is_int() == false){
		throw std::runtime_error("replace() requires start and end to be integers.");
	}

	const auto start = args[1].get_int_value();
	const auto end = args[2].get_int_value();
	if(start < 0 || end < 0){
		throw std::runtime_error("replace() requires start and end to be non-negative.");
	}
	if(args[3].get_type() != args[0].get_type()){
		throw std::runtime_error("replace() requires 4th arg to be same as argument 0.");
	}

	if(obj.is_string()){
		const auto str = obj.get_string_value();
		const auto start2 = std::min(start, static_cast<int>(str.size()));
		const auto end2 = std::min(end, static_cast<int>(str.size()));
		const auto new_bits = args[3].get_string_value();

		string str2 = str.substr(0, start2) + new_bits + str.substr(end2);
		const auto v = value_t::make_string(str2);
		return {vm, v};
	}
	else if(obj.is_vector()){
		const auto vec = obj.get_vector_value();
		const auto start2 = std::min(start, static_cast<int>(vec->_elements.size()));
		const auto end2 = std::min(end, static_cast<int>(vec->_elements.size()));
		const auto new_bits = args[3].get_vector_value();


		const auto string_left = vector<value_t>(vec->_elements.begin(), vec->_elements.begin() + start2);
		const auto string_right = vector<value_t>(vec->_elements.begin() + end2, vec->_elements.end());

		auto result = string_left;
		result.insert(result.end(), new_bits->_elements.begin(), new_bits->_elements.end());
		result.insert(result.end(), string_right.begin(), string_right.end());

		const auto v = value_t::make_vector_value(vec->_element_type, result);
		return {vm, v};
	}
	else{
		throw std::runtime_error("Calling replace() on unsupported type of value.");
	}
}

std::pair<interpreter_t, value_t> host__get_env_path(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 0){
		throw std::runtime_error("get_env_path() requires 0 arguments!");
	}

    const char *homeDir = getenv("HOME");
    const std::string env_path(homeDir);
//	const std::string env_path = "~/Desktop/";

	return {vm, value_t::make_string(env_path) };
}

std::pair<interpreter_t, value_t> host__read_text_file(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("read_text_file() requires 1 arguments!");
	}
	if(args[0].is_string() == false){
		throw std::runtime_error("read_text_file() requires a file path as a string.");
	}

	const string source_path = args[0].get_string_value();
	std::string file_contents;
	{
		std::ifstream f (source_path);
		if (f.is_open() == false){
			throw std::runtime_error("Cannot read source file.");
		}
		std::string line;
		while ( getline(f, line) ) {
			file_contents.append(line + "\n");
		}
		f.close();
	}
	return {vm, value_t::make_string(file_contents) };
}

std::pair<interpreter_t, value_t> host__write_text_file(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("write_text_file() requires 2 arguments!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else if(args[1].is_string() == false){
		throw std::runtime_error("write_text_file() requires a file path as a string.");
	}
	else{
		const string path = args[0].get_string_value();
		const string file_contents = args[1].get_string_value();

		std::ofstream outputFile;
		outputFile.open(path);
		outputFile << file_contents;
		outputFile.close();
		return {vm, value_t() };
	}
}

/*
	Reads json from a text string, returning an unpacked json_value.
*/
std::pair<interpreter_t, value_t> host__decode_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("decode_json_value() requires 1 argument!");
	}
	else if(args[0].is_string() == false){
		throw std::runtime_error("decode_json_value() requires string argument.");
	}
	else{
		const string s = args[0].get_string_value();
		std::pair<json_t, seq_t> result = parse_json(seq_t(s));
		value_t json_value = value_t::make_json_value(result.first);
		return {vm, json_value };
	}
}

std::pair<interpreter_t, value_t> host__encode_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("encode_json() requires 1 argument!");
	}
	else if(args[0].is_json_value()== false){
		throw std::runtime_error("encode_json() requires argument to be json_value.");
	}
	else{
		const auto value0 = args[0].get_json_value();
		const string s = json_to_compact_string(value0);

		return {vm, value_t::make_string(s) };
	}
}



std::pair<interpreter_t, value_t> host__flatten_to_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("flatten_to_json() requires 1 argument!");
	}
	else{
		const auto value = args[0];
		const auto result = flatten_to_json(value);
		return {vm, result };
	}
}

std::pair<interpreter_t, value_t> host__unflatten_from_json(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 2){
		throw std::runtime_error("unflatten_from_json() requires 2 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else if(args[1].is_typeid()== false){
		throw std::runtime_error("unflatten_from_json() requires string argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();
		const auto target_type = args[1].get_typeid_value();
		const auto value = unflatten_json_to_specific_type(json_value, target_type);
		return {vm, value };
	}
}


std::pair<interpreter_t, value_t> host__get_json_type(const interpreter_t& vm, const std::vector<value_t>& args){
	QUARK_ASSERT(vm.check_invariant());

	if(args.size() != 1){
		throw std::runtime_error("get_json_type() requires 1 argument!");
	}
	else if(args[0].is_json_value() == false){
		throw std::runtime_error("get_json_type() requires json_value argument.");
	}
	else{
		const auto json_value = args[0].get_json_value();

		if(json_value.is_object()){
			return { vm, value_t::make_int(1) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_dict(typeid_t::make_json_value())) };
		}
		else if(json_value.is_array()){
			return { vm, value_t::make_int(2) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_vector(typeid_t::make_json_value())) };
		}
		else if(json_value.is_string()){
			return { vm, value_t::make_int(3) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_string()) };
		}
		else if(json_value.is_number()){
			return { vm, value_t::make_int(4) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_float()) };
		}
		else if(json_value.is_true()){
			return { vm, value_t::make_int(5) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_false()){
			return { vm, value_t::make_int(6) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_bool()) };
		}
		else if(json_value.is_null()){
			return { vm, value_t::make_int(7) };
//			return { vm, value_t::make_typeid_value(typeid_t::make_null()) };
		}
		else{
			QUARK_ASSERT(false);
			throw std::exception();
		}
	}
}

const vector<host_function_t> k_host_functions {
	host_function_t{ "print", host__print, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "assert", host__assert, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "to_string", host__to_string, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "to_pretty_string", host__to_pretty_string, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "typeof", host__typeof, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_null()}) },

	host_function_t{ "get_time_of_day", host__get_time_of_day, typeid_t::make_function(typeid_t::make_int(), {}) },
	host_function_t{ "update", host__update, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "size", host__size, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null()}) },
	host_function_t{ "find", host__find, typeid_t::make_function(typeid_t::make_int(), {typeid_t::make_null(), typeid_t::make_null()}) },
	host_function_t{ "exists", host__exists, typeid_t::make_function(typeid_t::make_bool(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "erase", host__erase, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "push_back", host__push_back, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "subset", host__subset, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },
	host_function_t{ "replace", host__replace, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null(),typeid_t::make_null()}) },

	host_function_t{ "get_env_path", host__get_env_path, typeid_t::make_function(typeid_t::make_string(), {}) },
	host_function_t{ "read_text_file", host__read_text_file, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "write_text_file", host__write_text_file, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null(), typeid_t::make_null()}) },

	host_function_t{ "decode_json", host__decode_json, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },
	host_function_t{ "encode_json", host__encode_json, typeid_t::make_function(typeid_t::make_string(), {typeid_t::make_null()}) },

	host_function_t{ "flatten_to_json", host__flatten_to_json, typeid_t::make_function(typeid_t::make_json_value(), {typeid_t::make_null()}) },
	host_function_t{ "unflatten_from_json", host__unflatten_from_json, typeid_t::make_function(typeid_t::make_null(), {typeid_t::make_null(), typeid_t::make_null()}) },

	host_function_t{ "get_json_type", host__get_json_type, typeid_t::make_function(typeid_t::make_typeid(), {typeid_t::make_json_value()}) }
};


}