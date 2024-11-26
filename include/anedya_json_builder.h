#include <stddef.h>

#ifndef _ANEDYA_JSON_BUILDER_H_
#define	_ANEDYA_JSON_BUILDER_H_

#ifdef	__cplusplus
extern "C" {
#endif

/** @defgroup makejoson Make JSON.
  * @{ */

/** Open a JSON object in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_objOpen( char* dest, char const* name, size_t* end_marker );

/** Close a JSON object in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_objClose( char* dest, size_t* end_marker );

/** Used to finish the root JSON object. After call anedya_json_objClose().
  * @param dest Pointer to the end of JSON under construction.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_end( char* dest, size_t* end_marker );

/** Open an array in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_arrOpen( char* dest, char const* name, size_t* end_marker );

/** Close an array in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_arrClose( char* dest, size_t* end_marker );
/** Add a text property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value A valid null-terminated string with the value.
  *              Backslash escapes will be added for special characters.
  * @param len Max length of value. < 0 for unlimit.  
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */  
char* anedya_json_nstr( char* dest, char const* name, char const* value, int len, size_t* end_marker );

/** Add a text property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value A valid null-terminated string with the value.
  *              Backslash escapes will be added for special characters.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
static inline char* anedya_json_str( char* dest, char const* name, char const* value, size_t* end_marker ) {
    return anedya_json_nstr( dest, name, value, -1, end_marker );  
}

/** Add a boolean property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Zero for false. Non zero for true.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_bool( char* dest, char const* name, int value, size_t* end_marker );

/** Add a null property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_null( char* dest, char const* name, size_t* end_marker );

/** Add an integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_int( char* dest, char const* name, int value, size_t* end_marker );

/** Add an unsigned integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_uint( char* dest, char const* name, unsigned int value, size_t* end_marker );

/** Add a long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_long( char* dest, char const* name, long int value, size_t* end_marker );

/** Add an unsigned long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_ulong( char* dest, char const* name, unsigned long int value, size_t* end_marker );

/** Add a long long integer property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_verylong( char* dest, char const* name, long long int value, size_t* end_marker );

/** Add a double precision number property in a JSON string.
  * @param dest Pointer to the end of JSON under construction.
  * @param name Pointer to null-terminated string or null for unnamed.
  * @param value Value of the property.
  * @param end_marker Pointer to remaining length of dest
  * @return Pointer to the new end of JSON under construction. */
char* anedya_json_double( char* dest, char const* name, double value, size_t* end_marker );

/** @ } */

#ifdef	__cplusplus
}
#endif

#endif	/* _ANEDYA_JSON_BUILDER_H_ */
