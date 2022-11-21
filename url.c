#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"
#include <regex.h>

PG_MODULE_MAGIC;

typedef struct pg_url{
  char vl_len_[4];
  int protocol_len;
  int host_len;
  int port;
  int file_len;
  char data[1];
} pg_url;

static int default_port(char *protocol){
  if (strcasecmp(protocol, "http") == 0) return 80;
  if (strcasecmp(protocol, "https") == 0) return 443;
  if (strcasecmp(protocol, "ftp") == 0) return 21;
  if (strcasecmp(protocol, "sftp") == 0) return 22;
  if (strcasecmp(protocol, "ssh") == 0) return 22;
  if (strcasecmp(protocol, "git") == 0) return 9418;
  return -1;
}

static pg_url* create_url_from_fields(char *protocol, char *host, int port, char *file){
  int32 prot_size = strlen(protocol);
  int32 host_size = strlen(host);
  int32 file_size = strlen(file);

  pg_url *u = (pg_url *)palloc(VARHDRSZ + 5*4 + prot_size + host_size + file_size + 3);
  SET_VARSIZE(u, VARHDRSZ + 5*4 + prot_size + host_size + file_size + 3);

  // set the protocol
  int offset = 0;
  memset(u->data + offset, 0, prot_size +1);
  u->protocol_len = prot_size + 1;
  memcpy(u->data, protocol, prot_size+1);
  offset += prot_size + 1;

  // set the host
  memset(u->data + offset, 0, host_size + 1);
  u->host_len = host_size + 1;
  memcpy(u->data + offset, host, host_size+1);
  offset += host_size + 1;

  // set the port
  u->port = port;

  // set the file
  memset(u->data + offset, 0, file_size + 1);
  u->file_len = file_size + 1;
  memcpy(u->data + offset, file, file_size+1);

  return u;
}
/*
  * parse a url into its components
  * returns a pg_url
  * 
  * @param url the url to parse
  * @return a pg_url
  *
  * TODO treat special cases
*/
static pg_url* parse_url(char *url){
  regex_t reegex;
  // Creation of regEx
	int value = regcomp( &reegex, "(([a-zA-Z0-9]+):\\/\\/)*((www.)?[a-zA-Z0-9]+\\.[a-zA-Z0-9]*)(:([0-9]+))?(/([a-zA-Z0-9\\?_&=]+))?", REG_EXTENDED);

  regmatch_t pmatch[9];
  value = regexec( &reegex, url, 9, pmatch, 0);

  // Extract the protocol
  char *protocol;
  if (pmatch[2].rm_so == -1)
  {
    protocol = malloc(5);
    memcpy(protocol, "http", 5);
  } else{
    char *protocol_start = url + pmatch[2].rm_so;
    size_t protocol_length = pmatch[2].rm_eo - pmatch[2].rm_so;
    protocol = malloc(protocol_length + 1);
    memset(protocol, 0, protocol_length + 1);
    memcpy(protocol, protocol_start, protocol_length);
  }

  // Do the same with the host
  char *host;
  if (pmatch[3].rm_so == -1)
  {
    host = malloc(1);
    memcpy(host, "", 1);
  } else{
    char *host_start = url + pmatch[3].rm_so;
    size_t host_length = pmatch[3].rm_eo - pmatch[3].rm_so;
    host = malloc(host_length + 1);
    memset(host, 0, host_length + 1);
    memcpy(host, host_start, host_length);
  }

  // Do the same with protocol (but this is integer)
  int port;
  if (pmatch[6].rm_so == -1)
  {
    port = default_port(protocol);
  } else{
    char *port_start = url + pmatch[6].rm_so;
    size_t port_length = pmatch[6].rm_eo - pmatch[6].rm_so;
    char *port_str = malloc(port_length + 1);
    memset(port_str, 0, port_length + 1);
    memcpy(port_str, port_start, port_length);
    port = atoi(port_str);
  }

  // Do the same with the file
  char *file;
  if (pmatch[8].rm_so == -1)
  {
    file = malloc(1);
    memcpy(file, "", 1);
  } else{
    char *file_start = url + pmatch[8].rm_so;
    size_t file_length = pmatch[8].rm_eo - pmatch[8].rm_so;
    file = malloc(file_length + 1);
    memset(file, 0, file_length + 1);
    memcpy(file, file_start, file_length);
  }

  // Create the url
  pg_url *u = create_url_from_fields(protocol, host, port, file);

  // Free the memory allocated to the pattern
  regfree(&reegex);

  return u;
}

static pg_url* create_url_from_str(char *str){
  char *protocol = "http";
  char *host = palloc(strlen(str));
  strcpy(host, str);
  int port = 80;
  char *file = "index";
  pg_url *u = create_url_from_fields(protocol, host, port, file);
  return u;
}

static char* internal_to_string(pg_url *u){
  int prot_size = u->protocol_len;
  int host_size = u->host_len;
  int file_size = u->file_len;
  char *str = palloc(prot_size + host_size + file_size + 10);
  sprintf(str, "%s://%s:%d/%s", u->data, u->data + u->protocol_len, u->port, u->data + u->protocol_len + u->host_len);
  return str;
}

PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  pg_url *url = parse_url(str);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_out);
Datum
url_out(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);

  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the sizes
  int prot_size = url->protocol_len;
  int host_size = url->host_len;
  int port = url->port;
  int file_size = url->file_len;

  // Allocate enough memory for the string
  char *str = palloc(prot_size + host_size + file_size + 10);

  // Get the different parts of the url
  char *protocol = url->data;
  char *host = url->data + prot_size;
  char *file = url->data + prot_size + host_size;
  str = psprintf("%s://%s:%d/%s", protocol, host, port, file);
  PG_RETURN_CSTRING(str);
}
/*
PG_FUNCTION_INFO_V1(url_rcv);
Datum
url_rcv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
  pg_url *url = palloc(sizeof(pg_url));
  url->protocol = pstrdup(buf->data);
  url->host = pstrdup(buf->data + strlen(url->protocol) + 1);
  url->port = atoi(buf->data + strlen(url->protocol) + strlen(url->host) + 2);
  url->file = pstrdup(buf->data + strlen(url->protocol) + strlen(url->host) + strlen(url->file) + 4);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_send);
Datum
url_send(PG_FUNCTION_ARGS)
{
  pg_url *url = (pg_url *) PG_GETARG_POINTER(0);
  StringInfoData buf;
  pq_begintypsend(&buf);
  pq_sendbytes(&buf, url->protocol, strlen(url->protocol) + 1);
  pq_sendbytes(&buf, url->host, strlen(url->host) + 1);
  pq_sendbytes(&buf, url->port, 4 + 1);
  pq_sendbytes(&buf, url->file, strlen(url->file) + 1);
  PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}*/

PG_FUNCTION_INFO_V1(url_constructor_from_fields);
Datum
url_constructor_from_fields(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  int port = PG_GETARG_INT32(2);
  char *file = PG_GETARG_CSTRING(3);
  pg_url *url = create_url_from_fields(protocol, host, port, file);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_constructor_from_fields_default_port);
Datum
url_constructor_from_fields_default_port(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  char *file = PG_GETARG_CSTRING(2);
  pg_url *url = create_url_from_fields(protocol, host, default_port(protocol), file);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_constructor_from_string);
Datum
url_constructor_from_string(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  pg_url *url = create_url_from_str(str);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(get_host);
Datum
get_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the offset of the host
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

PG_FUNCTION_INFO_V1(get_port);
Datum
get_port(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the sizes
  int port = url->port;
  PG_RETURN_INT32(port);
}

PG_FUNCTION_INFO_V1(get_protocol);
Datum
get_protocol(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the protocol length
  int prot_size = url->protocol_len;

  // Allocate enough memory for the string
  char *str = palloc(prot_size);

  // Get the protocol
  char *protocol = url->data;
  str = psprintf("%s", protocol);
  PG_RETURN_CSTRING(str);
}

PG_FUNCTION_INFO_V1(get_file);
Datum
get_file(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the sizes
  int prot_size = url->protocol_len;
  int host_size = url->host_len;
  int file_size = url->file_len;

  // Allocate enough memory for the string
  char *str = palloc(file_size);

  // Get the file
  char *file = url->data + prot_size + host_size;
  str = psprintf("%s", file);
  PG_RETURN_CSTRING(str);
}

PG_FUNCTION_INFO_V1(toString);
Datum
toString(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

  // Get the sizes
  int prot_size = url->protocol_len;
  int host_size = url->host_len;
  int port = url->port;
  int file_size = url->file_len;

  // Allocate enough memory for the string
  char *str = palloc(prot_size + host_size + file_size + 10);

  // Get the different parts of the url
  char *protocol = url->data;
  char *host = url->data + prot_size;
  char *file = url->data + prot_size + host_size;
  str = psprintf("%s://%s:%d/%s", protocol, host, port, file);
  PG_RETURN_CSTRING(str);
}

PG_FUNCTION_INFO_V1(equals);
Datum
equals(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  bool result = (url1->port == url2->port) // Check the port
    && (url1->file_len == url2->file_len) // Check the file length
    && (url1->host_len == url2->host_len) // Check the host length
    && (url1->protocol_len == url2->protocol_len);
  
  if(result){
    // Get the strings
    char *str1 = internal_to_string(url1);
    char *str2 = internal_to_string(url2);

    // Compare the strings
    result = strcmp(str1, str2) == 0;
    PG_RETURN_BOOL(result);
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(not_equals);
Datum
not_equals(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  bool result = (url1->port != url2->port) // Check the port
    || (url1->file_len != url2->file_len) // Check the file length
    || (url1->host_len != url2->host_len) // Check the host length
    || (url1->protocol_len != url2->protocol_len);
  
  if(!result){
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