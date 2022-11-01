#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(url_test);
Datum
url_test(PG_FUNCTION_ARGS)
{
    char* arg = palloc(strlen(PG_GETARG_CSTRING(0)));
    strcpy(arg, PG_GETARG_CSTRING(0));
    char *url = "http://www.google.com";


    elog(NOTICE, "arg len [%d]'%s'\n", strlen(arg), arg);
    elog(NOTICE, "url len [%d]'%s'\n", strlen(url), url);

    //sprintf(number, "%d", arg);
    //strcat(url, "/");
    //strcat(url, arg);

    PG_RETURN_TEXT_P(cstring_to_text(url));
}