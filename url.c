#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"

PG_MODULE_MAGIC;

struct url{
  char *protocol;
  char *host;
  int   port;
  char *file;
};

static inline struct url parse_url_from_str(const char *str){

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