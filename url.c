#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"

PG_MODULE_MAGIC;

typedef struct{
  int32 length;
  char *protocol;
  char *host;
  int   port;
  char *file;
} Url;

static inline Url *parse_url_from_str(const char *str){
  char *protocol = "protocol"; //len 8
  char *host = "host"; // len 4
  int port = 80; // len 4
  char *file = "file"; // len 4
  Url *u = (Url *)palloc(VARHDRSZ + 8 + 4 + 4 + 4);

  SET_VARSIZE(u, VARHDRSZ + 8 + 4 + 4 + 4);

  memcpy(u->protocol, protocol, 8);
  memcpy(u->host, host, 4);
  u->port = port;
  memcpy(u->file, file, 4);
  PG_RETURN_POINTER(u);
}

PG_FUNCTION_INFO_V1(url_in);
Datum
url_in(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  Url *url = parse_url_from_str(str);
  elog(INFO, "url_in: %s", str);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_out);
Datum
url_out(PG_FUNCTION_ARGS)
{
  Url *url = (Url *) PG_GETARG_POINTER(0);
  char *str = palloc(1024);

  sprintf(str, "%s://%s:%d%s", url->protocol, url->host, url->port, url->file);
  PG_RETURN_CSTRING(str);
}

PG_FUNCTION_INFO_V1(url_rcv);
Datum
url_rcv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
  Url *url = palloc(sizeof(Url));
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
  Url *url = (Url *) PG_GETARG_POINTER(0);
  StringInfoData buf;
  pq_begintypsend(&buf);
  pq_sendbytes(&buf, url->protocol, strlen(url->protocol) + 1);
  pq_sendbytes(&buf, url->host, strlen(url->host) + 1);
  pq_sendbytes(&buf, url->port, 4 + 1);
  pq_sendbytes(&buf, url->file, strlen(url->file) + 1);
  PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

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

  SET_VARSIZE(new_text, new_text_size);
  memcpy(VARDATA(new_text), VARDATA_ANY(arg1), arg1_size);
  memcpy(VARDATA(new_text) + arg1_size, VARDATA_ANY(arg2), arg2_size);
  PG_RETURN_TEXT_P(new_text);
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