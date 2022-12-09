// Import libraries needed
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"
#include <regex.h>

PG_MODULE_MAGIC; // “magic block”  to ensure that a dynamically loaded object file is not loaded into an incompatible server, required by PostgreSQL

// Defining the elements that composes the pg_url object
typedef struct pg_url
{
  char vl_len_[4];
  int protocol_len;
  int user_len;
  int host_len;
  int port;
  int file_len;
  char data[1];
} pg_url;

/***
 * Internal functions
 ***/

/**
 * @brief Returns the default port for a protocol
 *
 * @param protocol
 * @return int
 *
 * @note If the protocol is not supported, returns -1
 */
static int default_port(char *protocol)
{
  // Compares the protocol to asign its defult port number
  if (strcasecmp(protocol, "http") == 0)
    return 80;
  if (strcasecmp(protocol, "https") == 0)
    return 443;
  if (strcasecmp(protocol, "ftp") == 0)
    return 21;
  if (strcasecmp(protocol, "sftp") == 0)
    return 22;
  if (strcasecmp(protocol, "ssh") == 0)
    return 22;
  if (strcasecmp(protocol, "git") == 0)
    return 9418;
  if (strcasecmp(protocol, "check") == 0)
    return 1;
  return -1;
}

/**
 * @brief Create a url from fields object
 *
 * @param protocol
 * @param user
 * @param host
 * @param port
 * @param file
 * @return pg_url*
 */
static pg_url *create_url_from_fields(char *protocol, char *user, char *host, int port, char *file)
{ // Gather the url elements and return a pointer to the full url

  // Get size of components
  int32 prot_size = strlen(protocol);
  int32 user_len = strlen(user);
  int32 host_size = strlen(host);
  int32 file_size = strlen(file);

  // Declare pointer u and allocate it on memory
  pg_url *u = (pg_url *)palloc(VARHDRSZ + 5 * 5 + prot_size + user_len + host_size + file_size + 4);
  // All variable-length types must begin with an opaque length field of exactly 4 bytes. "VARHDRSZ is the same as sizeof(int4)
  // Set values on memory
  SET_VARSIZE(u, VARHDRSZ + 5 * 5 + prot_size + user_len + host_size + file_size + 4);

  // set the protocol
  int offset;
  offset = 0;                                 // offset will tell us where in the memory are we storing each value. Starting with position 0.
  memset(u->data + offset, 0, prot_size + 1); // Empty the space that will be overwritten with our protocol
  u->protocol_len = prot_size + 1;            // Get protocol size
  memcpy(u->data, protocol, prot_size + 1);   // Write at the beginig of our url u the protocol
  offset += prot_size + 1;                    // Set variable to point to the net position after our protocol [u = protocol]

  // set the host
  memset(u->data + offset, 0, host_size + 1);    // Empty the space next to where we had finished writing our protocol previously
  u->host_len = host_size + 1;                   // Get host size
  memcpy(u->data + offset, host, host_size + 1); // Write the host after the protocol [u = protocol + host]
  offset += host_size + 1;                       // Update the position to point at the next space after host

  // set the port
  u->port = port;

  // set the file
  memset(u->data + offset, 0, file_size + 1);    // Empty the next space
  u->file_len = file_size + 1;                   // Get size
  memcpy(u->data + offset, file, file_size + 1); // Write the host after the protocol [u->data = protocol + host + file]
  offset += file_size + 1;                       // Update the position to point at the next space after file

  // set the user
  memset(u->data + offset, 0, user_len + 1);    // Empty the next space
  u->user_len = user_len + 1;                   // Get size
  memcpy(u->data + offset, user, user_len + 1); // Write the user at the end [u->data = protocol + host + file + user]

  return u;
}

/**
 * @brief Parse a url into its components using a regular expression
 *
 * @param url the url to parse
 * @return pg_url*
 */
static pg_url *parse_url(char *url)
{ // Receive the url and split into its components
  regex_t reegex;
  // Creation of regEx

  //                               protocol     :             user          : password         @         host         :  port     /  file
  int value = regcomp(&reegex, "(([a-zA-Z0-9]+):\\/\\/)*((([a-zA-Z0-9\\._-]+(:[a-zA-Z0-9\\._-]+)?)@)?([a-zA-Z0-9\\._+-]+))(:([0-9]+))?(/(.+))?", REG_EXTENDED); // If the regcomp() function is successful, it returns 0
  regmatch_t pmatch[12];
  /*
Expected matches from url:
  pmatch[0] = full thing
  pmatch[1] = protocol://
  pmatch[2] = protocol
  pmatch[3] = userinfo@host
  pmatch[4] = userinfo@
  pmatch[5] = userinfo
  pmatch[6] = :password
  pmatch[7] = host
  pmatch[8] = :port
  pmatch[9] = port
  pmatch[10] = /file
  pmatch[11] = file
*/

  value = regexec(&reegex, url, 12, pmatch, 0);

  // Extract the protocol
  char *protocol;
  if (pmatch[2].rm_so == -1) // If there is no match for protocol, we assume its 'http'
  {
    protocol = palloc(5);
    memcpy(protocol, "http", 5);
  }
  else
  { // Else, we add it to the url
    char *protocol_start = url + pmatch[2].rm_so;
    size_t protocol_length = pmatch[2].rm_eo - pmatch[2].rm_so;
    protocol = palloc(protocol_length + 1);

    memset(protocol, 0, protocol_length + 1);          // Empty the memory in which our protocol will be located
    memcpy(protocol, protocol_start, protocol_length); // copies into 'protocol' the protocol received according to its size
  }

  // Do the same with the userinfo
  char *user;
  if (pmatch[5].rm_so == -1)
  {
    user = palloc(1);
    memcpy(user, "", 1);
  }
  else
  {
    char *user_start = url + pmatch[5].rm_so;
    size_t user_length = pmatch[5].rm_eo - pmatch[5].rm_so;
    user = palloc(user_length + 1);
    memset(user, 0, user_length + 1);
    memcpy(user, user_start, user_length);
  }

  // Do the same with the host
  char *host;
  if (pmatch[7].rm_so == -1)
  {
    host = palloc(1);
    memcpy(host, "", 1);
  }
  else
  {
    char *host_start = url + pmatch[7].rm_so;
    size_t host_length = pmatch[7].rm_eo - pmatch[7].rm_so;
    host = palloc(host_length + 1);
    memset(host, 0, host_length + 1);
    memcpy(host, host_start, host_length);
  }

  // Do the same with protocol (but this is integer)
  int port;
  if (pmatch[9].rm_so == -1)
  {
    port = default_port(protocol);
  }
  else
  {
    char *port_start = url + pmatch[9].rm_so;
    size_t port_length = pmatch[9].rm_eo - pmatch[9].rm_so;
    char *port_str = palloc(port_length + 1);
    memset(port_str, 0, port_length + 1);
    memcpy(port_str, port_start, port_length);
    port = atoi(port_str);
  }

  // Do the same with the file
  char *file;
  if (pmatch[11].rm_so == -1)
  {
    file = palloc(1);
    memcpy(file, "", 1);
  }
  else
  {
    char *file_start = url + pmatch[11].rm_so;
    size_t file_length = pmatch[11].rm_eo - pmatch[11].rm_so;
    file = palloc(file_length + 1);
    memset(file, 0, file_length + 1);
    memcpy(file, file_start, file_length);
  }

  // Create the url
  pg_url *u = create_url_from_fields(protocol, user, host, port, file);

  // Free the memory allocated to the pattern
  regfree(&reegex);

  return u;
}

/**
 * @brief Computes the string form of a given url
 *
 * @param u url to be converted to string
 * @return char*
 */
static char *internal_to_string(pg_url *u)
{
  int prot_size = u->protocol_len;
  int user_size = u->user_len;
  int host_size = u->host_len;
  int file_size = u->file_len;
  char *str;
  if (user_size == 1)
  { // If there is no user, don't show it
    str = palloc(prot_size + user_size + host_size + file_size + 10);
    sprintf(str, "%s://%s:%d/%s", u->data, u->data + u->protocol_len, u->port, u->data + u->protocol_len + u->host_len); // Print URL's elements according to their position
  }
  else
  {
    str = palloc(prot_size + user_size + host_size + file_size + 11);
    sprintf(str, "%s://%s@%s:%d/%s", u->data, u->data + u->protocol_len + u->host_len + u->file_len, u->data + u->protocol_len, u->port, u->data + u->protocol_len + u->host_len); // Print URL's elements according to their position
  }
  return str;
}

/**
 * @brief Returns a string copy of the host of a given url
 *
 * @param u
 * @return char*
 */
static char *internal_get_host(pg_url *u)
{
  // Get the sizes
  int prot_size = u->protocol_len;
  int host_size = u->host_len;

  // Allocate enough memory for the string
  char *str = palloc(host_size);

  // Get the host
  char *host = u->data + prot_size;
  str = psprintf("%s", host);

  return str;
}

/**
 * @brief Returns a string copy of the file of a given url
 *
 * @param u url for which we want to get the file
 * @return char*
 */
static char *internal_get_file(pg_url *u)
{
  // Get the sizes
  int prot_size = u->protocol_len;
  int host_size = u->host_len;
  int file_size = u->file_len;

  // Allocate enough memory for the string
  char *str = palloc(file_size);

  // Get the file
  char *file = u->data + prot_size + host_size;
  str = psprintf("%s", file);

  return str;
}

/***
 * Postgres functions
 */

/**
 * @brief Construct a new pg_url from a string
 *
 */
PG_FUNCTION_INFO_V1(url_in);   // Declare Postgres function
Datum url_in(PG_FUNCTION_ARGS) // Elaborate function url_in : recives string from Postgres and translates it into our pg_url object
{
  char *str = PG_GETARG_CSTRING(0); // Get argument as string
  pg_url *url = parse_url(str);     // Convert string into our pg_url object
  PG_RETURN_POINTER(url);
}

/**
 * @brief Converts a pg_url into a string
 *
 */
PG_FUNCTION_INFO_V1(url_out);   // Declare Postgres function
Datum url_out(PG_FUNCTION_ARGS) // Elaborate function url_out :
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0); // varlena: used as header for variable-length datatypes   .

  pg_url *url = (pg_url *)(&(url_buf->vl_dat)); // Get data content from url_buf [varlena -> vl_dat] received from postgres and store it in our pg_url object
  url = (pg_url *)pg_detoast_datum(url_buf);    // Unpack postgres value keeping alignment order

  char *str = internal_to_string(url); // Convert our pg_url object into string

  PG_RETURN_CSTRING(str);
}

/***
 * Constructors
 */

/**
 * @brief Construct a new pg_url from fields
 *
 * @param protocol
 * @param host
 * @param port
 * @param file
 *
 * @return pg_url
 */
PG_FUNCTION_INFO_V1(url_constructor_from_fields);
Datum url_constructor_from_fields(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  int port = PG_GETARG_INT32(2);
  char *file = PG_GETARG_CSTRING(3);
  pg_url *url = create_url_from_fields(protocol, "", host, port, file);
  PG_RETURN_POINTER(url);
}

/**
 * @brief Construct a new pg_url from fields with default port
 *
 * @param protocol
 * @param host
 * @param file
 *
 * @return pg_url
 *
 */
PG_FUNCTION_INFO_V1(url_constructor_from_fields_default_port);
Datum url_constructor_from_fields_default_port(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  char *file = PG_GETARG_CSTRING(2);
  pg_url *url = create_url_from_fields(protocol, "", host, default_port(protocol), file);
  PG_RETURN_POINTER(url);
}

/**
 * @brief Construct a new pg_url from a string, parsing it
 *
 * @param str
 *
 * @return pg_url
 */
PG_FUNCTION_INFO_V1(url_constructor_from_string);
Datum url_constructor_from_string(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  pg_url *url = parse_url(str);
  PG_RETURN_POINTER(url);
}

/**
 * @brief Construct a new pg_url from a text, parsing it
 *
 * @param txt
 *
 * @return pg_url
 *
 */
PG_FUNCTION_INFO_V1(url_constructor_from_text);
Datum url_constructor_from_text(PG_FUNCTION_ARGS)
{
  text *txt = PG_GETARG_TEXT_P(0);
  char *str = text_to_cstring(txt);
  pg_url *url = parse_url(str);
  PG_RETURN_POINTER(url);
}

/**
 * @brief Construct a new pg_url from a context and a specification
 *
 * @param context
 * @param spec
 *
 * @return pg_url
 *
 * @note Appends spec to context and then reconstruct url from the result
 */
PG_FUNCTION_INFO_V1(url_constructor_context_spec);
Datum url_constructor_context_spec(PG_FUNCTION_ARGS)
{
  // Get URL context
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get URL spec
  char *spec_str = PG_GETARG_CSTRING(1);

  char *context_str = internal_to_string(url);
  int context_len = strlen(context_str);
  int spec_len = strlen(spec_str);
  int full_len = context_len + spec_len + 1;
  char *full_str = palloc(full_len);
  memset(full_str, 0, full_len);
  memcpy(full_str, context_str, context_len);
  memcpy(full_str + context_len, spec_str, spec_len);
  PG_RETURN_POINTER(parse_url(full_str));
}

/***
 * Getters
 */

/**
 * @brief Get default port associated to the protocol of given url
 *
 * @param url
 *
 * @return int
 *
 * @note If the given protocol is not supported, returns -1
 */
PG_FUNCTION_INFO_V1(get_default_port);
Datum get_default_port(PG_FUNCTION_ARGS)
{
  pg_url *url = (pg_url *)PG_GETARG_POINTER(0);
  // Get protocol and default port associated to it
  char *protocol = url->data;
  int port = default_port(protocol);
  PG_RETURN_INT32(port);
}

/**
 * @brief Get the file of a given url
 *
 * @param url
 *
 * @return char*
 */
PG_FUNCTION_INFO_V1(get_file);
Datum get_file(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);
  char *str = internal_get_file(url);
  PG_RETURN_CSTRING(str);
}

/**
 * @brief Get userinfo of a given url
 *
 * @param url
 *
 * @return char*
 */
PG_FUNCTION_INFO_V1(get_userinfo);
Datum get_userinfo(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get the sizes
  int prot_size = url->protocol_len;
  int user_len = url->user_len;
  int host_size = url->host_len;
  int file_size = url->file_len;

  // Allocate enough memory for the string
  char *str = palloc(user_len);

  // Get the userinfo
  char *user = url->data + prot_size + host_size + file_size;

  str = psprintf("%s", user);

  PG_RETURN_CSTRING(str);
}

/**
 * @brief Get the host of a given url
 *
 * @param url
 *
 * @return char*
 */
PG_FUNCTION_INFO_V1(get_host);
Datum get_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get the position of the host
  int prot_size = url->protocol_len;

  // Get the host length
  int host_size = url->host_len;

  // Allocate enough memory for the string
  char *str = palloc(host_size);

  // Get the host
  char *host = url->data + prot_size;
  str = psprintf("%s", host);
  PG_RETURN_CSTRING(str);
}

/**
 * @brief Get the authority of a given url
 *
 * @param url
 *
 * @return char
 *
 * @note The authority is defined as userinfo@host
 */
PG_FUNCTION_INFO_V1(get_authority);
Datum get_authority(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get the sizes
  int prot_size = url->protocol_len;
  int host_size = url->host_len;
  int file_size = url->file_len;

  // Get host and userinfo
  char *host = url->data + prot_size;
  char *user = url->data + prot_size + host_size + file_size;

  // Concatenate them as userinfo@host
  char *auth = psprintf("%s@%s", user, host);

  PG_RETURN_CSTRING(auth);
}

/**
 * @brief Get the port of a given url
 *
 * @param url
 *
 * @return int
 */
PG_FUNCTION_INFO_V1(get_port);
Datum get_port(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get the sizes
  int port = url->port;
  PG_RETURN_INT32(port);
}

/**
 * @brief Get the path of a given url
 *
 * @param url
 *
 * @return char*
 *
 * @note The path is defined as the file without the query part and the fragment
 */
PG_FUNCTION_INFO_V1(get_path);
Datum get_path(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  char *file = internal_get_file(url);
  char *token = strtok(file, "?#");
  if (token == NULL)
  { // If there is no query part nor fragment, return the file
    PG_RETURN_CSTRING(file);
  }
  else
  { // If there is a query part or a fragment, return the path
    PG_RETURN_CSTRING(token);
  }
}

/**
 * @brief Get the query part of a given url
 *
 * @param url
 *
 * @return char
 *
 * @note The query part is defined as the part after ? and before #
 */
PG_FUNCTION_INFO_V1(get_query);
Datum get_query(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  char *file = internal_get_file(url);

  // Extract the query: part after ? and before #
  char *token = strtok(file, "?");
  token = strtok(NULL, "#");

  if (token == NULL)
    PG_RETURN_NULL();
  else
    PG_RETURN_CSTRING(token);
}

/**
 * @brief Get the reference part of a given url
 *
 * @param url
 *
 * @return char*
 *
 * @note The reference part is defined as the part after #
 */
PG_FUNCTION_INFO_V1(get_ref);
Datum get_ref(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  char *file = internal_get_file(url);

  // Extract the ref: part after #
  char *token = strtok(file, "#");
  token = strtok(NULL, "#");

  if (token == NULL)
    PG_RETURN_NULL();
  else
    PG_RETURN_CSTRING(token);
}

/**
 * @brief Get the protocol of the given url
 *
 * @param url
 *
 * @return char*
 */
PG_FUNCTION_INFO_V1(get_protocol);
Datum get_protocol(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  // Get the protocol length
  int prot_size = url->protocol_len;

  // Allocate enough memory for the string
  char *str = palloc(prot_size);

  // Get the protocol
  char *protocol = url->data;
  str = psprintf("%s", protocol);
  PG_RETURN_CSTRING(str);
}

/***
 * Show functions
 */

/**
 * @brief Get the string representation of the given url
 *
 * @param url
 *
 * @return char*
 */
PG_FUNCTION_INFO_V1(toString);
Datum toString(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)pg_detoast_datum(url_buf);

  char *str = internal_to_string(url);

  PG_RETURN_CSTRING(str);
}

/***
 * Comparison functions
 */

/***
 * Functions comparing full urls
 */

/**
 * @brief Compares two given urls
 *
 * @param url1
 * @param url2
 *
 * @return int: 0 if equal, 1 if url1 > url2, -1 if url1 < url2
 */
PG_FUNCTION_INFO_V1(pg_url_cmp);
Datum pg_url_cmp(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Get the strings
  char *str1 = internal_to_string(url1);
  char *str2 = internal_to_string(url2);

  // Compare the strings
  int result = strcmp(str1, str2);
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */

  PG_RETURN_INT32(result);
}

/**
 * @brief Compare equality of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 *
 * @note Two urls are considered equal if their string representation is equal
 * @note This function is index supported, see the sql file definition
 */
PG_FUNCTION_INFO_V1(equals);
Datum equals(PG_FUNCTION_ARGS)
{
  // Get value for url1
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  // Get value for url2
  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compare components
  bool result = (url1->port == url2->port)            // Check the port
                && (url1->file_len == url2->file_len) // Check the file length
                && (url1->host_len == url2->host_len) // Check the host length
                && (url1->protocol_len == url2->protocol_len);

  // If the components match, compare string values
  if (result)
  {
    // Get the strings
    char *str1 = internal_to_string(url1);
    char *str2 = internal_to_string(url2);

    // Compare the strings
    result = strcmp(str1, str2) == 0; // if strings are equal
    PG_RETURN_BOOL(result);
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare equality of the files of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 *
 * @note Two files are considered equal if they are in the same host and have the same file name, including query and fragment
 * @note equals implies same_file
 * @note This function is index supported, see the sql file definition
 */
PG_FUNCTION_INFO_V1(same_file);
Datum same_file(PG_FUNCTION_ARGS)
{
  // Get value for url1
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  // Get value for url2
  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compare components
  bool result = (url1->file_len == url2->file_len);

  // If the components match, compare their file part
  if (result)
  {
    // Get the hosts
    char *host1 = internal_get_host(url1);
    char *host2 = internal_get_host(url2);

    // If hosts are different, false
    if (strcmp(host1, host2) != 0)
    {
      PG_RETURN_BOOL(false);
    }

    // Else, we get the files
    char *file1 = internal_get_file(url1);
    char *file2 = internal_get_file(url2);

    // And compare them
    result = strcmp(file1, file2) == 0; // if strings are equal
    PG_RETURN_BOOL(result);
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}
/**
 * @brief Compare equality of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 *
 * @note Two hosts are considered equal if their string representation is equal
 * @note same_file implies same_host
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(same_host_internal);
Datum same_host_internal(PG_FUNCTION_ARGS)
{
  // Get value for url1
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  // Get value for url2
  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);
  // Compare components
  bool result = (url1->host_len == url2->host_len);

  // If the components match, compare their host part
  if (result)
  {
    char *host1 = internal_get_host(url1);
    char *host2 = internal_get_host(url2);
    result = strcmp(host1, host2) == 0; // if strings are equal
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare inequality of hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(different_host);
Datum different_host(PG_FUNCTION_ARGS)
{
  // Get value for url1
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  // Get value for url2
  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compare components
  bool result = (url1->host_len == url2->host_len);

  // If the components match, compare their host part
  if (result)
  {
    char *host1 = url1->data + url1->protocol_len;
    char *host2 = url2->data + url2->protocol_len;
    result = strcmp(host1, host2) != 0; // if strings are different
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare inequality of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(not_equals);
Datum not_equals(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  bool result = (url1->port != url2->port)            // Check the port
                || (url1->file_len != url2->file_len) // Check the file length
                || (url1->host_len != url2->host_len) // Check the host length
                || (url1->protocol_len != url2->protocol_len);

  if (!result)
  {
    // Get the strings
    char *str1 = internal_to_string(url1);
    char *str2 = internal_to_string(url2);

    // Compare the strings
    result = strcmp(str1, str2) != 0;
    PG_RETURN_BOOL(result);
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare < of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(less_than);
Datum less_than(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Get the strings
  char *str1 = internal_to_string(url1);
  char *str2 = internal_to_string(url2);

  // Compare the strings
  bool result = strcmp(str1, str2) < 0;
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */

  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare <= of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(less_than_or_equal);
Datum less_than_or_equal(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Get the strings
  char *str1 = internal_to_string(url1);
  char *str2 = internal_to_string(url2);

  // Compare the strings
  bool result = strcmp(str1, str2) <= 0;
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare > of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(greater_than);
Datum greater_than(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Get the strings
  char *str1 = internal_to_string(url1);
  char *str2 = internal_to_string(url2);

  // Compare the strings
  bool result = strcmp(str1, str2) > 0;
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare >= of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 */
PG_FUNCTION_INFO_V1(greater_than_or_equal);
Datum greater_than_or_equal(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Get the strings
  char *str1 = internal_to_string(url1);
  char *str2 = internal_to_string(url2);

  // Compare the strings
  bool result = strcmp(str1, str2) >= 0;
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */
  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare < of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(less_than_host);
Datum less_than_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 < host2?
  bool result = strcmp(host1, host2) < 0;

  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare <= of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(less_than_or_equal_host);
Datum less_than_or_equal_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 <= host2?
  bool result = strcmp(host1, host2) <= 0;

  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare > of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(greater_than_host);
Datum greater_than_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 > host2?
  bool result = strcmp(host1, host2) > 0;

  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare >= of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(greater_than_or_equal_host);
Datum greater_than_or_equal_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 >= host2?
  bool result = strcmp(host1, host2) >= 0;

  PG_RETURN_BOOL(result);
}

/**
 * @brief Compare = of the hosts of two given urls
 *
 * @param url1
 * @param url2
 *
 * @return bool
 * @note This function is a base function for BTree construction
 */
PG_FUNCTION_INFO_V1(pg_url_cmp_host);
Datum pg_url_cmp_host(PG_FUNCTION_ARGS)
{
  struct varlena *url_buf1 = (struct varlena *)PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)pg_detoast_datum(url_buf1);

  struct varlena *url_buf2 = (struct varlena *)PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (pg_url *)pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // Compare hosts
  int result = strcmp(host1, host2);
  /*
  strcmp Returns:	Scenario:
  0			if strings are equal
  >0			if the first non-matching character in str1 is greater (in ASCII) than that of str2.
  <0			if the first non-matching character in str1 is lower (in ASCII) than that of str2.
  */
  PG_RETURN_INT32(result);
}
