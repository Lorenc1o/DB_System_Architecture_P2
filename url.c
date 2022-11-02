#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>

PG_MODULE_MAGIC;

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

//TODO: This is not working. How to return multiple values? How to do a proper constructor?
PG_FUNCTION_INFO_V1(url_constructor_all_fields);
Datum
url_constructor_all_fields(PG_FUNCTION_ARGS)
{
  text *protocol = PG_GETARG_TEXT_P(0);
  text *host = PG_GETARG_TEXT_P(1);
  int32 port = PG_GETARG_INT32(2);
  text *file = PG_GETARG_TEXT_P(3);
  PG_RETURN_TEXT_P(protocol);
}