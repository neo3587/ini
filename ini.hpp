
#pragma once

#ifndef __NEO_INI_HPP__
#define __NEO_INI_HPP__


/*
	Header name: ini.hpp
	Author: neo3587

	Read more about ini files: https://en.wikipedia.org/wiki/INI_file

	Notes:
		- Requires C++11 or higher

*/

#include "oi_map.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cctype>
#include <iomanip>
#include <functional>



namespace neo {

	namespace __ini_details {

		std::string _comm_add_sharp(const std::string& comm) {
			if(comm.empty())
				return "";
			std::stringstream ss(comm);
			std::string line, str;
			while(!ss.eof()) {
				std::getline(ss, line);
				str += "#" + line + "\n";
			}
			return std::move(str);
		}
		
		struct lcase_pred : public std::binary_function<std::string, std::string, bool> {
			bool operator()(const std::string& left, const std::string& right) const {
				return std::lexicographical_compare(left.begin(), left.end(), right.begin(), right.end(), [](char a, char b) { return std::tolower(a) < std::tolower(b); });
			}
		};

		class value : public std::string {

			public:

				std::string comment;

				using std::string::string;
				value() {}
				value(const value&) = default;
				value(value&&) = default;
				value(const std::string& str) : std::string(str) {}
				value(std::string&& str) : std::string(std::forward<std::string>(str)) {}

				using std::string::operator=;
				value& operator=(const value&) = default;
				value& operator=(value&&) = default;

				template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
				operator T() const {
					std::istringstream ss(*this);
					T tmp;
					ss >> std::setprecision(std::numeric_limits<T>::max_digits10) >> tmp;
					return tmp;;
				}

				template<class T, typename = typename std::enable_if<std::is_arithmetic<T>::value>::type>
				value& operator=(const T& val) {
					std::ostringstream ss;
					ss << std::setprecision(std::numeric_limits<T>::max_digits10) << val;
					*this = std::move(ss.str());
					return *this;
				}

				template<class T>
				T read(std::function<T(const value&)> fn = [](const value& val) -> T { return val; }) const {
					return fn(*this);
				}
				template<class T>
				value& write(const T& val, std::function<value(const T&)> fn = [&](const T& t) -> value { return value().operator=(t); }) {
					*this = fn(val);
					return *this;
				}


		};
		class keys : public oi_map<std::string, value, lcase_pred> {

			public:

				std::string comment;

				using value = value;

				keys() {}
				keys(const keys&) = default;
				keys(keys&&) = default;
				keys& operator=(const keys&) = default;
				keys& operator=(keys&&) = default;

				/* Renames the selected key, any iterator to the key before the rename becomes unusable
				*/
				iterator rename(const_iterator pos, const std::string& new_name) {
					if(pos == this->end())
						return this->end();
					value v = std::move(pos->second);
					pos = this->erase(pos);
					iterator it = this->emplace_hint(this->end(), new_name, std::move(v));
					this->splice(pos, it);
					return it;
				}
				/* Renames the selected key, any iterator to the key before the rename becomes unusable
				*/
				iterator rename(const std::string& key, const std::string& new_name) {
					return rename(find(key), new_name);
				}

				std::string to_string(bool comments = true) const noexcept {
					std::string str;
					for(const_iterator it = this->cbegin(); it != this->cend(); ++it) 
						str += (comments ? _comm_add_sharp(it->second.comment) : "") + it->first + " = " + it->second + "\n";
					//if(str.back() == '\n') str.pop_back();
					return std::move(str);
				}

		};
		class sections : public oi_map<std::string, keys, lcase_pred> {

			public:

				using iterator		 = typename oi_map<std::string, keys, lcase_pred>::iterator;
				using const_iterator = typename oi_map<std::string, keys, lcase_pred>::const_iterator;

				using oi_map<std::string, keys, lcase_pred>::oi_map;

				/* Renames the selected section, any iterator to the section before the rename becomes unusable					
				*/
				iterator rename(const_iterator pos, const std::string& new_name) {
					if(pos == this->end() || this->find(new_name) != this->end())
						return this->end();
					keys k = std::move(pos->second);
					pos = erase(pos);
					iterator it = emplace(new_name, std::move(k)).first;
					this->splice(pos, it);
					return it;
				}
				/* Renames the selected section, any iterator to the section before the rename becomes unusable
				*/ 
				iterator rename(const std::string& section, const std::string& new_name) {
					return rename(this->find(section), new_name);
				}

				std::string to_string(bool comments = true) const noexcept {
					std::string str;
					for(const_iterator it = this->cbegin(); it != this->cend(); ++it) 
						str += (comments ? _comm_add_sharp(it->second.comment) : "") + "[" + it->first + "]\n" + it->second.to_string(comments) + "\n";
					//if(str.back() == '\n') str.pop_back(); 
					return std::move(str);
				}

		};

	}

	template<bool Sections = true>
	class ini : public std::conditional<Sections, __ini_details::sections, __ini_details::keys>::type {

		public:

			using keys	= __ini_details::keys;
			using value = __ini_details::value;

			// Constructors:

			ini() {}
			ini(const ini&) = default;
			ini(ini&&) = default;

			ini& operator=(const ini&) = default;
			ini& operator=(ini&&) = default;

			ini(const std::string& filename) {
				parse_file(filename);
			}
			template<class Istream, typename = typename std::enable_if<std::is_constructible<std::istream&, Istream>::value || std::is_constructible<std::istream&&, Istream>::value>::type>
			ini(Istream&& is) {
				parse_file(std::forward<Istream>(is));
			}

			// IO Operations:

			template<class Istream, typename = typename std::enable_if<std::is_constructible<std::istream&, Istream>::value || std::is_constructible<std::istream&&, Istream>::value>::type>
			Istream&& parse_file(Istream&& is) {
				this->clear();
				return std::forward<Istream>(_parse_file(std::forward<Istream>(is)));
			}
			std::ifstream&& parse_file(const std::string& filename) {
				return std::move(parse_file(std::ifstream(filename)));
			}
			void parse_str(const std::string& str) {
				parse_file(std::istringstream(str));
			}

			template<class Ostream, typename = typename std::enable_if<std::is_constructible<std::ostream&, Ostream>::value || std::is_constructible<std::ostream&&, Ostream>::value>::type>
			Ostream&& to_file(Ostream&& os) const {
				os << this->to_string();
				return std::forward<Ostream>(os);
			}
			std::ofstream&& to_file(const std::string& filename) const {
				std::ofstream ofs(filename);
				ofs << this->to_string();
				return std::move(ofs);
			}

		private:

			std::string _str_trim(const std::string& str) {
				size_t fp = str.find_first_not_of(" \t\n\r\f\v"), lp = str.find_last_not_of(" \t\n\r\f\v");
				fp = fp == std::string::npos ? 0 : fp;
				lp = lp == std::string::npos ? 0 : lp;
				return str.substr(fp, lp - fp + 1);
			}
			std::pair<std::string, std::string> _split_comment(const std::string& str) {
				size_t pos = str.find_first_of(";#");
				return std::pair<std::string, std::string>(str.substr(0, pos), pos == std::string::npos ? "" : str.substr(pos + 1));
			}
			void _keyval_insert(std::string& line, std::istream& is, keys& k, std::string& acc_comm) {
				std::size_t equal_pos = line.find_first_of('=');
				if(equal_pos != std::string::npos) {
					std::string key = _str_trim(line.substr(0, equal_pos));
					std::string val = _str_trim(line.substr(equal_pos + 1));
					std::string comm = std::move(acc_comm);
					while(val.back() == '\\' && is.good() && !is.eof()) { // append multi-line values
						val.pop_back();
						std::getline(is, line);
						std::pair<std::string, std::string> pr = _split_comment(line);
						val += _str_trim(pr.first);
						comm += pr.second + "\n";
					}
					if(!comm.empty())
						comm.pop_back();
					k.emplace_hint(k.end(), std::move(key), std::move(val))->second.comment = std::move(comm);
				};
			}

			template<class Istream>
			typename std::enable_if<std::integral_constant<bool, !Sections>::value, Istream&&>::type _parse_file(Istream&& is) {
				
				std::string line, comm;

				while(is.good() && !is.eof()) {
					std::getline(is, line);
					std::pair<std::string, std::string> pr = _split_comment(line);
					line = _str_trim(pr.first);
					comm += (pr.second.empty() ? "" : pr.second + "\n");
					
					if(line.empty())
						continue;
					
					_keyval_insert(pr.first, is, *this, comm);
				}

				return std::forward<Istream>(is);
			}
			template<class Istream>
			typename std::enable_if<std::integral_constant<bool, Sections>::value, Istream&&>::type _parse_file(Istream&& is) {

				typename ini::iterator it = this->end();
				std::string line, comm;

				while(is.good() && !is.eof()) {

					std::getline(is, line);
					std::pair<std::string, std::string> pr = _split_comment(line);
					line = _str_trim(pr.first);
					comm += (pr.second.empty() ? "" : pr.second + "\n");

					if(line.empty())
						continue;

					// if it's a section
					else if(line.front() == '[' && line.back() == ']') {
						it = this->emplace(line.substr(1, line.size() - 2), keys()).first;
						if(!comm.empty())
							comm.pop_back();
						it->second.comment = std::move(comm);
					}

					// insert key only if a section was found
					else if(it != this->end())
						_keyval_insert(line, is, it->second, comm);
				}

				return std::forward<Istream>(is);
			}

	};	

}



#endif
