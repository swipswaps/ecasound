// -*- mode: C++; -*-
#ifndef INCLUDED_KVU_UTILS_H
#define INCLUDED_KVU_UTILS_H

#include <vector>
#include <string>

bool kvu_string_icmp(const std::string& a, const std::string& b);

std::vector<std::string> kvu_string_to_tokens(const std::string& s);
std::vector<std::string> kvu_string_to_tokens_quoted(const std::string& s);
std::vector<std::string> kvu_string_to_vector(const std::string& str, const std::string::value_type separator);
std::vector<int> kvu_string_to_int_vector(const std::string& str, const std::string::value_type separator);

std::string kvu_string_search_and_replace(const std::string& a, 
					  const std::string::value_type from,
					  const std::string::value_type to);
std::string kvu_vector_to_string(const std::vector<std::string>& str, 
				 const std::string& separator);
std::string kvu_remove_trailing_spaces(const std::string& a);
std::string kvu_remove_preceding_spaces(const std::string& a);
std::string kvu_remove_surrounding_spaces(const std::string& a);
std::string kvu_convert_to_uppercase(const std::string& a);
std::string kvu_convert_to_lowercase(const std::string& a);
void kvu_to_uppercase(std::string& a);
void kvu_to_lowercase(std::string& a);

std::string kvu_get_argument_number(int number, const std::string& argu);
std::vector<std::string> kvu_get_arguments(const std::string& argu);
int kvu_get_number_of_arguments(const std::string& argu);
std::string kvu_get_argument_prefix(const std::string& argument);

void kvu_print_time_stamp(void);

int kvu_sleep(long int seconds, long int nanoseconds);

/**
 * Obsolete functions.
 */
/*@{*/

std::vector<std::string> kvu_string_to_words(const std::string& s);

/*@}*/

#endif
