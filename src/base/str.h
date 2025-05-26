/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef BASE_STR_H
#define BASE_STR_H

#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <ctime>

#include <base/detect.h>
#include <base/types.h>

/**
 * String related functions.
 *
 * @defgroup Strings
 */

/**
 * Appends a string to another.
 *
 * @ingroup Strings
 *
 * @param dst Pointer to a buffer that contains a string.
 * @param src String to append.
 * @param dst_size Size of the buffer of the dst string.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
void str_append(char *dst, const char *src, int dst_size);

/**
 * Appends a string to a fixed-size array of chars.
 *
 * @ingroup Strings
 *
 * @param dst Array that shall receive the string.
 * @param src String to append.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
template<int N>
void str_append(char (&dst)[N], const char *src)
{
	str_append(dst, src, N);
}

/**
 * Copies a string to another.
 *
 * @ingroup Strings
 *
 * @param dst Pointer to a buffer that shall receive the string.
 * @param src String to be copied.
 * @param dst_size Size of the buffer dst.
 *
 * @return Length of written string, even if it has been truncated
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
int str_copy(char *dst, const char *src, int dst_size);

/**
 * Copies a string to a fixed-size array of chars.
 *
 * @ingroup Strings
 *
 * @param dst Array that shall receive the string.
 * @param src String to be copied.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
template<int N>
void str_copy(char (&dst)[N], const char *src)
{
	str_copy(dst, src, N);
}

/**
 * Truncates a UTF-8 encoded string to a given length.
 *
 * @ingroup Strings
 *
 * @param dst Pointer to a buffer that shall receive the string.
 * @param dst_size Size of the buffer dst.
 * @param str String to be truncated.
 * @param truncation_len Maximum codepoints in the returned string.
 *
 * @remark The strings are treated as utf8-encoded null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
void str_utf8_truncate(char *dst, int dst_size, const char *src, int truncation_len);

/**
 * Truncates a string to a given length.
 *
 * @ingroup Strings
 *
 * @param dst Pointer to a buffer that shall receive the string.
 * @param dst_size Size of the buffer dst.
 * @param src String to be truncated.
 * @param truncation_len Maximum length of the returned string (not
 *                       counting the null-termination).
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that dst string will contain null-termination.
 */
void str_truncate(char *dst, int dst_size, const char *src, int truncation_len);

/**
 * Returns the length of a null-terminated string.
 *
 * @ingroup Strings
 *
 * @param str Pointer to the string.
 *
 * @return Length of string in bytes excluding the null-termination.
 */
int str_length(const char *str);

/**
 * Performs printf formatting into a buffer.
 *
 * @ingroup Strings
 *
 * @param buffer Pointer to the buffer to receive the formatted string.
 * @param buffer_size Size of the buffer.
 * @param format printf formatting string.
 * @param args The variable argument list.
 *
 * @return Length of written string, even if it has been truncated.
 *
 * @remark See the C manual for syntax for the printf formatting string.
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that buffer string will contain null-termination.
 */
int str_format_v(char *buffer, int buffer_size, const char *format, va_list args)
	GNUC_ATTRIBUTE((format(printf, 3, 0)));

/**
 * Performs printf formatting into a buffer.
 *
 * @ingroup Strings
 *
 * @param buffer Pointer to the buffer to receive the formatted string.
 * @param buffer_size Size of the buffer.
 * @param format printf formatting string.
 * @param ... Parameters for the formatting.
 *
 * @return Length of written string, even if it has been truncated.
 *
 * @remark See the C manual for syntax for the printf formatting string.
 * @remark The strings are treated as null-terminated strings.
 * @remark Guarantees that buffer string will contain null-termination.
 */
int str_format(char *buffer, int buffer_size, const char *format, ...)
	GNUC_ATTRIBUTE((format(printf, 3, 4)));

#if !defined(CONF_DEBUG)
int str_format_int(char *buffer, size_t buffer_size, int value);

template<typename... Args>
int str_format_opt(char *buffer, int buffer_size, const char *format, Args... args)
{
	static_assert(sizeof...(args) > 0, "Use str_copy instead of str_format without format arguments");
	return str_format(buffer, buffer_size, format, args...);
}

template<>
inline int str_format_opt(char *buffer, int buffer_size, const char *format, int val)
{
	if(strcmp(format, "%d") == 0)
	{
		return str_format_int(buffer, buffer_size, val);
	}
	else
	{
		return str_format(buffer, buffer_size, format, val);
	}
}

#define str_format str_format_opt
#endif

/**
 * Trims specific number of words at the start of a string.
 *
 * @ingroup Strings
 *
 * @param str String to trim the words from.
 * @param words Count of words to trim.
 *
 * @return Trimmed string
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Leading whitespace is always trimmed.
 */
const char *str_trim_words(const char *str, int words);

/**
 * Check whether string has ASCII control characters.
 *
 * @ingroup Strings
 *
 * @param str String to check.
 *
 * @return Whether the string has ASCII control characters.
 *
 * @remark The strings are treated as null-terminated strings.
 */
bool str_has_cc(const char *str);

/**
 * Replaces all characters below 32 with whitespace.
 *
 * @ingroup Strings
 *
 * @param str String to sanitize.
 *
 * @remark The strings are treated as null-terminated strings.
 */
void str_sanitize_cc(char *str);

/**
 * Replaces all characters below 32 with whitespace with
 * exception to \t, \n and \r.
 *
 * @ingroup Strings
 *
 * @param str String to sanitize.
 *
 * @remark The strings are treated as null-terminated strings.
 */
void str_sanitize(char *str);

/**
 * Replaces all invalid filename characters with whitespace.
 *
 * @param str String to sanitize.
 * @remark The strings are treated as null-terminated strings.
 */
void str_sanitize_filename(char *str);

/**
 * Checks if a string is a valid filename on all supported platforms.
 *
 * @param str Filename to check.
 *
 * @return `true` if the string is a valid filename, `false` otherwise.
 *
 * @remark The strings are treated as null-terminated strings.
 */
bool str_valid_filename(const char *str);

/**
 * Removes leading and trailing spaces and limits the use of multiple spaces.
 *
 * @ingroup Strings
 *
 * @param str String to clean up.
 *
 * @remark The strings are treated as null-terminated strings.
 */
void str_clean_whitespaces(char *str);

/**
 * Skips leading non-whitespace characters.
 *
 * @ingroup Strings
 *
 * @param str Pointer to the string.
 *
 * @return Pointer to the first whitespace character found
 *		   within the string.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Whitespace is defined according to str_isspace.
 */
char *str_skip_to_whitespace(char *str);

/**
 * @ingroup Strings
 *
 * @see str_skip_to_whitespace
 */
const char *str_skip_to_whitespace_const(const char *str);

/**
 * Skips leading whitespace characters.
 *
 * @ingroup Strings
 *
 * @param str Pointer to the string.
 *
 * @return Pointer to the first non-whitespace character found
 *         within the string.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Whitespace is defined according to str_isspace.
 */
char *str_skip_whitespaces(char *str);

/**
 * @ingroup Strings
 *
 * @see str_skip_whitespaces
 */
const char *str_skip_whitespaces_const(const char *str);

/**
 * Compares to strings case insensitively.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 *
 * @remark Only guaranteed to work with a-z/A-Z.
 * @remark The strings are treated as null-terminated strings.
 */
int str_comp_nocase(const char *a, const char *b);

/**
 * Compares up to `num` characters of two strings case insensitively.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 * @param num Maximum characters to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 *
 * @remark Only guaranteed to work with a-z/A-Z.
 * @remark Use `str_utf8_comp_nocase_num` for unicode support.
 * @remark The strings are treated as null-terminated strings.
 */
int str_comp_nocase_num(const char *a, const char *b, int num);

/**
 * Compares two strings case sensitive.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_comp(const char *a, const char *b);

/**
 * Compares up to `num` characters of two strings case sensitive.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 * @param num Maximum characters to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_comp_num(const char *a, const char *b, int num);

/**
 * Compares two strings case insensitive, digit chars will be compared as numbers.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 *
 * @return `< 0` - String a is less than string b
 * @return `0` - String a is equal to string b
 * @return `> 0` - String a is greater than string b
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_comp_filenames(const char *a, const char *b);

/**
 * Checks case insensitive whether the string begins with a certain prefix.
 *
 * @ingroup Strings
 *
 * @param str String to check.
 * @param prefix Prefix to look for.
 *
 * @return A pointer to the string `str` after the string prefix, or `nullptr` if
 *         the string prefix isn't a prefix of the string `str`.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_startswith_nocase(const char *str, const char *prefix);

/**
 * Checks case sensitive whether the string begins with a certain prefix.
 *
 * @ingroup Strings
 *
 * @param str String to check.
 * @param prefix Prefix to look for.
 *
 * @return A pointer to the string `str` after the string prefix, or `nullptr` if
 *         the string prefix isn't a prefix of the string `str`.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_startswith(const char *str, const char *prefix);

/**
 * Checks case insensitive whether the string ends with a certain suffix.
 *
 * @ingroup Strings
 *
 * @param str String to check.
 * @param suffix Suffix to look.
 *
 * @return A pointer to the beginning of the suffix in the string `str`.
 * @return `nullptr` if the string suffix isn't a suffix of the string `str`.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_endswith_nocase(const char *str, const char *suffix);

/**
 * Checks case sensitive whether the string ends with a certain suffix.
 *
 * @param str String to check.
 * @param suffix Suffix to look for.
 *
 * @return A pointer to the beginning of the suffix in the string `str`.
 * @return `nullptr` if the string suffix isn't a suffix of the string `str`.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_endswith(const char *str, const char *suffix);

/**
 * Computes the edit distance between two strings.
 *
 * @param a First string for the edit distance.
 * @param b Second string for the edit distance.
 *
 * @return The edit distance between the both strings.
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_utf8_dist(const char *a, const char *b);

/**
 * Computes the edit distance between two strings, allows buffers
 * to be passed in.
 *
 * @ingroup Strings
 *
 * @param a First string for the edit distance.
 * @param b Second string for the edit distance.
 * @param buf Buffer for the function.
 * @param buf_len Length of the buffer, must be at least as long as
 *                twice the length of both strings combined plus two.
 *
 * @return The edit distance between the both strings.
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_utf8_dist_buffer(const char *a, const char *b, int *buf, int buf_len);

/**
 * Computes the edit distance between two strings, allows buffers
 * to be passed in.
 *
 * @ingroup Strings
 *
 * @param a First string for the edit distance.
 * @param a_len Length of the first string.
 * @param b Second string for the edit distance.
 * @param b_len Length of the second string.
 * @param buf Buffer for the function.
 * @param buf_len Length of the buffer, must be at least as long as
 *                the length of both strings combined plus two.
 *
 * @return The edit distance between the both strings.
 *
 * @remark The strings are treated as null-terminated strings.
 */
int str_utf32_dist_buffer(const int *a, int a_len, const int *b, int b_len, int *buf, int buf_len);

/**
 * Finds a string inside another string case insensitively.
 *
 * @ingroup Strings
 *
 * @param haystack String to search in.
 * @param needle String to search for.
 *
 * @return A pointer into `haystack` where the needle was found.
 * @return Returns `nullptr` if `needle` could not be found.
 *
 * @remark Only guaranteed to work with a-z/A-Z.
 * @remark Use str_utf8_find_nocase for unicode support.
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_find_nocase(const char *haystack, const char *needle);

/**
 * Finds a string inside another string case sensitive.
 *
 * @ingroup Strings
 *
 * @param haystack String to search in.
 * @param needle String to search for.
 *
 * @return A pointer into `haystack` where the needle was found.
 * @return Returns `nullptr` if `needle` could not be found.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_find(const char *haystack, const char *needle);

/**
 * @ingroup Strings
 *
 * @param haystack String to search in.
 * @param delim String to search for.
 * @param offset Number of characters into `haystack`.
 * @param start Will be set to the first delimiter on the left side of the offset (or `haystack` start).
 * @param end Will be set to the first delimiter on the right side of the offset (or `haystack` end).
 *
 * @return `true` if both delimiters were found.
 * @return `false` if a delimiter is missing (it uses `haystack` start and end as fallback).
 *
 * @remark The strings are treated as null-terminated strings.
 */
bool str_delimiters_around_offset(const char *haystay, const char *delim, int offset, int *start, int *end);

/**
 * Finds the last occurrence of a character
 *
 * @ingroup Strings
 *
 * @param haystack String to search in.
 * @param needle Character to search for.

 * @return A pointer into haystack where the needle was found.
 * @return Returns `nullptr` if needle could not be found.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark The zero-terminator character can also be found with this function.
 */
const char *str_rchr(const char *haystack, char needle);

/**
 * Counts the number of occurrences of a character in a string.
 *
 * @ingroup Strings
 *
 * @param haystack String to count in.
 * @param needle Character to count.

 * @return The number of characters in the haystack string matching
 *         the needle character.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark The number of zero-terminator characters cannot be counted.
 */
int str_countchr(const char *haystack, char needle);

/**
 * Takes a datablock and generates a hex string of it, with spaces between bytes.
 *
 * @ingroup Strings
 *
 * @param dst Buffer to fill with hex data.
 * @param dst_size Size of the buffer (at least 3 * data_size + 1 to contain all data).
 * @param data Data to turn into hex.
 * @param data_size Size of the data.
 *
 * @remark The destination buffer will be null-terminated.
 */
void str_hex(char *dst, int dst_size, const void *data, int data_size);

/**
 * Takes a datablock and generates a hex string of it, in the C style array format,
 * i.e. with bytes formatted in 0x00-0xFF notation and commas with spaces between the bytes.
 * The output can be split over multiple lines by specifying the maximum number of bytes
 * that should be printed per line.
 *
 * @ingroup Strings
 *
 * @param dst Buffer to fill with hex data.
 * @param dst_size Size of the buffer (at least `6 * data_size + 1` to contain all data).
 * @param data Data to turn into hex.
 * @param data_size Size of the data.
 * @param bytes_per_line After this many printed bytes a newline will be printed.
 *
 * @remark The destination buffer will be null-terminated.
 */
void str_hex_cstyle(char *dst, int dst_size, const void *data, int data_size, int bytes_per_line = 12);

/**
 * Takes a hex string *without spaces between bytes* and returns a byte array.
 *
 * @ingroup Strings
 *
 * @param dst Buffer for the byte array.
 * @param dst_size size of the buffer.
 * @param data String to decode.
 *
 * @return `2` if string doesn't exactly fit the buffer.
 * @return `1` if invalid character in string.
 * @return `0` if success.
 *
 * @remark The contents of the buffer is only valid on success.
 */
int str_hex_decode(void *dst, int dst_size, const char *src);

/**
 * Takes a datablock and generates the base64 encoding of it.
 *
 * @ingroup Strings
 *
 * @param dst Buffer to fill with base64 data.
 * @param dst_size Size of the buffer.
 * @param data Data to turn into base64.
 * @param data Size of the data.
 *
 * @remark The destination buffer will be null-terminated
 */
void str_base64(char *dst, int dst_size, const void *data, int data_size);

/**
 * Takes a base64 string without any whitespace and correct
 * padding and returns a byte array.
 *
 * @ingroup Strings
 *
 * @param dst Buffer for the byte array.
 * @param dst_size Size of the buffer.
 * @param data String to decode.
 *
 * @return `< 0` - Error.
 * @return `<= 0` - Success, length of the resulting byte buffer.
 *
 * @remark The contents of the buffer is only valid on success.
 */
int str_base64_decode(void *dst, int dst_size, const char *data);

/**
 * Escapes \ and " characters in a string.
 *
 * @param dst Destination array pointer, gets increased, will point to the terminating null.
 * @param src Source array.
 * @param end End of destination array.
 */
void str_escape(char **dst, const char *src, const char *end);

int str_toint(const char *str);
bool str_toint(const char *str, int *out);
int str_toint_base(const char *str, int base);
unsigned long str_toulong_base(const char *str, int base);
int64_t str_toint64_base(const char *str, int base = 10);
float str_tofloat(const char *str);
bool str_tofloat(const char *str, float *out);

/**
 * Determines whether a character is whitespace.
 *
 * @ingroup Strings
 *
 * @param c the character to check.
 *
 * @return `1` if the character is whitespace, `0` otherwise.
 *
 * @remark The following characters are considered whitespace: ' ', '\n', '\r', '\t'.
 */
int str_isspace(char c);

char str_uppercase(char c);

bool str_isnum(char c);

int str_isallnum(const char *str);

int str_isallnum_hex(const char *str);

unsigned str_quickhash(const char *str);

int str_utf8_to_skeleton(const char *str, int *buf, int buf_len);

/**
 * Checks if two strings only differ by confusable characters.
 *
 * @ingroup Strings
 *
 * @param str1 String to compare.
 * @param str2 String to compare.
 *
 * @return `0` if the strings are confusables.
 */
int str_utf8_comp_confusable(const char *str1, const char *str2);

/**
 * Converts the given Unicode codepoint to lowercase (locale insensitive).
 *
 * @ingroup Strings
 *
 * @param code Unicode codepoint to convert.
 *
 * @return Lowercase codepoint, or the original codepoint if there is no lowercase version.
 */
int str_utf8_tolower_codepoint(int code);

/**
 * Converts the given UTF-8 string to lowercase (locale insensitive).
 *
 * @ingroup Strings
 *
 * @param str String to convert to lowercase.
 * @param output Buffer that will receive the lowercase string.
 * @param size Size of the output buffer.
 *
 * @remark The strings are treated as zero-terminated strings.
 * @remark This function does not work in-place as converting a UTF-8 string to
 *         lowercase may increase its length.
 */
void str_utf8_tolower(const char *input, char *output, size_t size);

/**
 * Compares two UTF-8 strings case insensitively.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 */
int str_utf8_comp_nocase(const char *a, const char *b);

/**
 * Compares up to `num` bytes of two UTF-8 strings case insensitively.
 *
 * @ingroup Strings
 *
 * @param a String to compare.
 * @param b String to compare.
 * @param num Maximum bytes to compare.
 *
 * @return `< 0` if string a is less than string b.
 * @return `0` if string a is equal to string b.
 * @return `> 0` if string a is greater than string b.
 */
int str_utf8_comp_nocase_num(const char *a, const char *b, int num);

/**
 * Finds a UTF-8 string inside another UTF-8 string case insensitively.
 *
 * @ingroup Strings
 *
 * @param haystack String to search in.
 * @param needle String to search for.
 * @param end A pointer that will be set to a pointer into haystack directly behind the
 *            last character where the needle was found. Will be set to `nullptr `if needle
 *            could not be found. Optional parameter.
 *
 * @return A pointer into haystack where the needle was found.
 * @return Returns `nullptr` if needle could not be found.
 *
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_utf8_find_nocase(const char *haystack, const char *needle, const char **end = nullptr);

/**
 * Checks whether the given Unicode codepoint renders as space.
 *
 * @ingroup Strings
 *
 * @param code Unicode codepoint to check.
 *
 * @return Whether the codepoint is a space.
 */
int str_utf8_isspace(int code);

/**
 * Checks whether a given byte is the start of a UTF-8 character.
 *
 * @ingroup Strings
 *
 * @param c Byte to check.
 *
 * @return Whether the char starts a UTF-8 character.
 */
int str_utf8_isstart(char c);

/**
 * Skips leading characters that render as spaces.
 *
 * @ingroup Strings
 *
 * @param str Input string.
 *
 * @return Pointer to the first non-whitespace character found within the string.
 * @remark The strings are treated as null-terminated strings.
 */
const char *str_utf8_skip_whitespaces(const char *str);

/**
 * Removes trailing characters that render as spaces by modifying the string in-place.
 *
 * @ingroup Strings
 *
 * @param param Input string.
 *
 * @remark The string is modified in-place.
 * @remark The strings are treated as null-terminated.
 */
void str_utf8_trim_right(char *param);

/**
 * Moves a cursor backwards in an UTF-8 string,
 *
 * @ingroup Strings
 *
 * @param str UTF-8 string.
 * @param cursor Position in the string.
 *
 * @return New cursor position.
 *
 * @remark Won't move the cursor less then 0.
 * @remark The strings are treated as null-terminated.
 */
int str_utf8_rewind(const char *str, int cursor);

/**
 * Fixes truncation of a Unicode character at the end of a UTF-8 string.
 *
 * @ingroup Strings
 *
 * @param str UTF-8 string.
 *
 * @return The new string length.
 *
 * @remark The strings are treated as null-terminated.
 */
int str_utf8_fix_truncation(char *str);

/**
 * Moves a cursor forwards in an UTF-8 string.
 *
 * @ingroup Strings
 *
 * @param str UTF-8 string.
 * @param cursor Position in the string.
 *
 * @return New cursor position.
 *
 * @remark Won't move the cursor beyond the null-termination marker.
 * @remark The strings are treated as null-terminated.
 */
int str_utf8_forward(const char *str, int cursor);

/**
 * Decodes a UTF-8 codepoint.
 *
 * @ingroup Strings
 *
 * @param ptr Pointer to a UTF-8 string. This pointer will be moved forward.
 *
 * @return The Unicode codepoint. `-1` for invalid input and 0 for end of string.
 *
 * @remark This function will also move the pointer forward.
 * @remark You may call this function again after an error occurred.
 * @remark The strings are treated as null-terminated.
 */
int str_utf8_decode(const char **ptr);

/**
 * Encode a UTF-8 character.
 *
 * @ingroup Strings
 *
 * @param ptr Pointer to a buffer that should receive the data. Should be able to hold at least 4 bytes.
 *
 * @return Number of bytes put into the buffer.
 *
 * @remark Does not do null-termination of the string.
 */
int str_utf8_encode(char *ptr, int chr);

/**
 * Checks if a strings contains just valid UTF-8 characters.
 *
 * @ingroup Strings
 *
 * @param str Pointer to a possible UTF-8 string.
 *
 * @return `0` if invalid characters were found, `1` if only valid characters were found.
 *
 * @remark The string is treated as null-terminated UTF-8 string.
 */
int str_utf8_check(const char *str);

/**
 * Copies a number of UTF-8 characters from one string to another.
 *
 * @ingroup Strings
 *
 * @param dst Pointer to a buffer that shall receive the string.
 * @param src String to be copied.
 * @param dst_size Size of the buffer dst.
 * @param num Maximum number of UTF-8 characters to be copied.
 *
 * @remark The strings are treated as null-terminated strings.
 * @remark Garantees that dst string will contain null-termination.
 */
void str_utf8_copy_num(char *dst, const char *src, int dst_size, int num);

/**
 * Determines the byte size and UTF-8 character count of a UTF-8 string.
 *
 * @ingroup Strings
 *
 * @param str Pointer to the string.
 * @param max_size Maximum number of bytes to count.
 * @param max_count Maximum number of UTF-8 characters to count.
 * @param size Pointer to store size (number of non. Zero bytes) of the string.
 * @param count Pointer to store count of UTF-8 characters of the string.
 *
 * @remark The string is treated as null-terminated UTF-8 string.
 * @remark It's the user's responsibility to make sure the bounds are aligned.
 */
void str_utf8_stats(const char *str, size_t max_size, size_t max_count, size_t *size, size_t *count);

/**
 * Converts a byte offset of a UTF-8 string to the UTF-8 character offset.
 *
 * @ingroup Strings
 *
 * @param text Pointer to the string.
 * @param byte_offset Offset in bytes.
 *
 * @return Offset in UTF-8 characters. Clamped to the maximum length of the string in UTF-8 characters.
 *
 * @remark The string is treated as a null-terminated UTF-8 string.
 * @remark It's the user's responsibility to make sure the bounds are aligned.
 */
size_t str_utf8_offset_bytes_to_chars(const char *str, size_t byte_offset);

/**
 * Converts a UTF-8 character offset of a UTF-8 string to the byte offset.
 *
 * @ingroup Strings
 *
 * @param text Pointer to the string.
 * @param char_offset Offset in UTF-8 characters.
 *
 * @return Offset in bytes. Clamped to the maximum length of the string in bytes.
 *
 * @remark The string is treated as a null-terminated UTF-8 string.
 * @remark It's the user's responsibility to make sure the bounds are aligned.
 */
size_t str_utf8_offset_chars_to_bytes(const char *str, size_t char_offset);

/**
 * Writes the next token after str into buf, returns the rest of the string.
 *
 * @ingroup Strings
 *
 * @param str Pointer to string.
 * @param delim Delimiter for tokenization.
 * @param buffer Buffer to store token in.
 * @param buffer_size Size of the buffer.
 *
 * @return Pointer to rest of the string.
 *
 * @remark The token is always null-terminated.
 */
const char *str_next_token(const char *str, const char *delim, char *buffer, int buffer_size);

/**
 * Checks if needle is in list delimited by delim.
 *
 * @param list List.
 * @param delim List delimiter.
 * @param needle Item that is being looked for.
 *
 * @return `1` - Item is in list.
 * @return `0` - Item isn't in list.
 *
* @remark The strings are treated as null-terminated strings.
 */
int str_in_list(const char *list, const char *delim, const char *needle);

#endif
