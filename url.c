#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"

PG_MODULE_MAGIC;

typedef struct pg_url{
  char vl_len_[4];
  int full_len;
  int protocol_len;
  int host_len;
  int port;
  int file_len;
  char data[1];
} pg_url;

static pg_url* parse_url_from_fields(char *protocol, char *host, int port, char *file){
  int32 prot_size = strlen(protocol);
  int32 host_size = strlen(host);
  int32 file_size = strlen(file);

  elog(INFO, "%d %d %d", prot_size, host_size, file_size);

  int32 url_size = sizeof(pg_url);
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
  elog(INFO, "LOG12 %s", u->data);

  return u;
}

static pg_url* parse_url_from_str(char *str){
  char *protocol = "protocol";
  char *host = "host";
  int port = 80;
  char *file = "file";
  pg_url *u = parse_url_from_fields(protocol, host, port, file);
  return u;
}

PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  pg_url *url = parse_url_from_str(str);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_out);
Datum
url_out(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  int32 buf_size = VARSIZE_4B(url_buf);
  int32 full_size = &(url_buf->vl_dat);

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

PG_FUNCTION_INFO_V1(url_test);
Datum
url_test(PG_FUNCTION_ARGS)
{   
  text  *arg1 = cstring_to_text("http://www.google.com/");
  text  *arg2 = PG_GETARG_TEXT_PP(0);
  int32 arg1_size = VARSIZE_ANY_EXHDR(arg1);
  int32 arg2_size = VARSIZE_ANY_EXHDR(arg2);
  int32 new_text_size = arg1_size + arg2_size + VARHDRSZ;
  text *new_text = (text *) palloc(new_text_size);

  elog(INFO, "arg1_size: %d", arg1_size);
  elog(INFO, "arg2_size: %d", arg2_size);
  elog(INFO, "new_text_size: %d", new_text_size);

  SET_VARSIZE(new_text, new_text_size);
  memcpy(VARDATA(new_text), VARDATA_ANY(arg1), arg1_size);
  memcpy(VARDATA(new_text) + arg1_size, VARDATA_ANY(arg2), arg2_size);
  PG_RETURN_TEXT_P(new_text);
}
/*
PG_FUNCTION_INFO_V1(url_constructor);
Datum
url_constructor(PG_FUNCTION_ARGS)
{
  pg_url *url = parse_url_from_str("http://www.google.com/");
  PG_RETURN_POINTER(url);
}

//TODO: This is not working. How to return multiple values?
PG_FUNCTION_INFO_V1(url_constructor_all_fields);
Datum
url_constructor_all_fields(PG_FUNCTION_ARGS)
{
  text *protocol = PG_GETARG_TEXT_P(0);
  text *host = PG_GETARG_TEXT_P(1);
  int32 port = PG_GETARG_INT32(2);
  text *file = PG_GETARG_TEXT_P(3);

  TupleDesc tupdesc;
  HeapTuple tuple;
  Datum result;
  Datum values[4];

  values[0] = PointerGetDatum(protocol);
  values[1] = PointerGetDatum(host);
  values[2] = Int32GetDatum(port);
  values[3] = PointerGetDatum(file);

  tupdesc = CreateTemplateTupleDesc(4);

  TupleDescInitEntry(tupdesc, (AttrNumber) 1, "protocol", TEXTOID, -1, 0);
  TupleDescInitEntry(tupdesc, (AttrNumber) 2, "host", TEXTOID, -1, 0);
  TupleDescInitEntry(tupdesc, (AttrNumber) 3, "port", INT4OID, -1, 0);
  TupleDescInitEntry(tupdesc, (AttrNumber) 4, "file", TEXTOID, -1, 0);

  tuple = heap_form_tuple(tupdesc, values, NULL);

  result = HeapTupleGetDatum(tuple);

  PG_RETURN_DATUM(result);
}
*/