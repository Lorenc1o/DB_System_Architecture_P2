// Import libraries needed
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include <strings.h>
#include "funcapi.h"
#include <regex.h>
#include<stdio.h>
 
PG_MODULE_MAGIC; //“magic block”  to ensure that a dynamically loaded object file is not loaded into an incompatible server, required by PostgreSQL 

//Defining the elements that composes the pg_url object
typedef struct pg_url{
  char vl_len_[4];
  int protocol_len;
  int user_len;
  int host_len;
  int port;
  int file_len;
  char data[1];
} pg_url;

static int default_port(char *protocol){
  //Compares the protocol to asign its defult port number
  if (strcasecmp(protocol, "http") == 0) return 80;
  if (strcasecmp(protocol, "https") == 0) return 443;
  if (strcasecmp(protocol, "ftp") == 0) return 21;
  if (strcasecmp(protocol, "sftp") == 0) return 22;
  if (strcasecmp(protocol, "ssh") == 0) return 22;
  if (strcasecmp(protocol, "git") == 0) return 9418;
  if (strcasecmp(protocol, "check") == 0) return 1;
  return -1;
}


static pg_url* create_url_from_fields(char *protocol, char *user, char *host, int port, char *file){ //Gather the url elements and return a pointer to the full url
	
  //Get size of components
  int32 prot_size = strlen(protocol);
  int32 user_len = strlen(user);
  int32 host_size = strlen(host);
  int32 file_size = strlen(file);
  
	
//Declare pointer u and allocate it on memory 
  pg_url *u = (pg_url *)palloc(VARHDRSZ + 5*5 + prot_size + user_len + host_size + file_size + 4);
//All variable-length types must begin with an opaque length field of exactly 4 bytes. "VARHDRSZ is the same as sizeof(int4)
//Set values on memory
  SET_VARSIZE(u, VARHDRSZ + 5*5 + prot_size + user_len + host_size + file_size + 4);

  // set the protocol
  int offset;
  offset = 0; //offset will tell us where in the memory are we storing each value. Starting with position 0.
  memset(u->data + offset, 0, prot_size +1); // Empty the space that will be overwritten with our protocol
  u->protocol_len = prot_size + 1; // Get protocol size
  memcpy(u->data, protocol, prot_size+1); // Write at the beginig of our url u the protocol
  offset += prot_size + 1; //Set variable to point to the net position after our protocol [u = protocol]

  // set the host
  memset(u->data + offset, 0, host_size + 1); //Empty the space next to where we had finished writing our protocol previously
  u->host_len = host_size + 1; // Get host size
  memcpy(u->data + offset, host, host_size+1);  // Write the host after the protocol [u = protocol + host]
  offset += host_size + 1; //Update the position to point at the next space after host

  // set the port
  u->port = port;

  // set the file
  memset(u->data + offset, 0, file_size + 1); //Empty the next space 
  u->file_len = file_size + 1; // Get size
  memcpy(u->data + offset, file, file_size+1); // Write the host after the protocol [u->data = protocol + host + file]
  offset += file_size + 1; //Update the position to point at the next space after file

  // set the user
  memset(u->data + offset, 0, user_len + 1); //Empty the next space
  u->user_len = user_len + 1; // Get size
  memcpy(u->data + offset, user, user_len+1); // Write the user at the end [u->data = protocol + host + file + user]

  return u;
}
/*
  * parse a url into its components
  * returns a pg_url
  * 
  * @param url the url to parse
  * @return a pg_url
  *
*/


static pg_url* parse_url(char *url){ //Receive the url and split into its components
  regex_t reegex;
  // Creation of regEx
  //                               protocol     :             user          : password         @         host         :  port     /  file
	int value = regcomp( &reegex, "(([a-zA-Z0-9]+):\\/\\/)*((([a-zA-Z0-9\\._-]+(:[a-zA-Z0-9\\._-]+)?)@)?([a-zA-Z0-9\\._+-]+))(:([0-9]+))?(/([a-zA-Z0-9\\?_&#=]+))?", REG_EXTENDED);  //If the regcomp() function is successful, it returns 0
  regmatch_t pmatch[12];
    /*
  Expected matches from url:
	  pmatch[0] = full thing
	  pmatch[1] = protocol://
	int value = regcomp( &reegex, 
	//"(([a-zA-Z0-9]+):\\/\\/)*((www.)?[a-zA-Z0-9]+\\.[a-zA-Z0-9]*)(:([0-9]+))?(/([a-zA-Z0-9\\?_&=\\-]+))?"
	"(([a-zA-Z0-9]+):\\/\\/)?((a-zA-Z0-9.+-):?(a-zZ-Z0-9.+-)?@)?((www.)?[a-zA-Z0-9]+(\\.[a-zA-Z0-9]+)?)(:([0-9]+))?(/([a-zA-Z0-9\\?_&#=]+))?"
	, REG_EXTENDED);  //If the regcomp() function is successful, it returns 0
	
	

  /*
    
  12 expected matches from url:
	  pmatch[0] =
	  pmatch[1] =  
	  pmatch[2] = protocol
	  pmatch[3] = user host
	  pmatch[4] =user
	  pmatch[5] =password
	  pmatch[6] = host
	  pmatch[7] = www.x...
	  pmatch[8] = www.
	  pmatch[10] = port
	  pmatch[11] = file
  
  
  
  */
  
  regmatch_t pmatch[12];
  value = regexec( &reegex, url, 12, pmatch, 0);
    
  /*regmatch_t pmatch[9];
  9 expected matches from url:
	  pmatch[0] =
	  pmatch[1] =  
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
  
  value = regexec( &reegex, url, 12, pmatch, 0);
  
  
  
  // Extract the protocol
  char *protocol;
  if (pmatch[2].rm_so == -1) //If there is no match for protocol, we assume its 'http'
  {
    protocol = palloc(5);
    memcpy(protocol, "http", 5);
  } else{ // Else, we add it to the url
    char *protocol_start = url + pmatch[2].rm_so;
    size_t protocol_length = pmatch[2].rm_eo - pmatch[2].rm_so;
    protocol = palloc(protocol_length + 1);
	  
    memset(protocol, 0, protocol_length + 1); //Empty the memory in which our protocol will be located 
    memcpy(protocol, protocol_start, protocol_length); // copies into 'protocol' the protocol received according to its size
  }

  // Do the same with the userinfo
  char *user;
  if (pmatch[5].rm_so == -1)
  {
    user = palloc(1);
    memcpy(user, "", 1);
  } else{
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
  } else{
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
  } else{
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
  } else{
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

// For a given URL *u, return its string equivalent
static char* internal_to_string(pg_url *u){
  int prot_size = u->protocol_len;
  int user_size = u-> user_len;
  int host_size = u->host_len;
  int file_size = u->file_len;
  char *str = palloc(prot_size + user_size + host_size + file_size + 11);
  sprintf(str, "%s://%s@%s:%d/%s", u->data, u->data + u->protocol_len, u->data + u->protocol_len + u->host_len + u->file_len, u->port, u->data + u->protocol_len + u->host_len); //Print URL's elements according to their position
  return str;
}

PG_FUNCTION_INFO_V1(url_in); //Declare Postgres function
Datum url_in(PG_FUNCTION_ARGS) //Elaborate function url_in : recives string from Postgres and translates it into our pg_url object
{
  char *str = PG_GETARG_CSTRING(0); //Get argument as string 
  pg_url *url = parse_url(str); //Convert string into our pg_url object
  PG_RETURN_POINTER(url); 
}

PG_FUNCTION_INFO_V1(url_out); //Declare Postgres function
Datum url_out(PG_FUNCTION_ARGS) //Elaborate function url_out :
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0); // varlena: used as header for variable-length datatypes   .

  pg_url *url = (pg_url *)(&(url_buf->vl_dat)); //Get data content from url_buf [varlena -> vl_dat] received from postgres and store it in our pg_url object
  url = (pg_url *) pg_detoast_datum(url_buf); //Unpack postgres value keeping alignment order

  // Get the sizes
  int prot_size = url->protocol_len;
  int user_size = url->user_len;
  int host_size = url->host_len;
  int port = url->port;
  int file_size = url->file_len;

  // Allocate enough memory for the string
  char *str = palloc(prot_size + user_size +host_size + file_size + 11);

  // Get the different parts of the url
  char *protocol = url->data;
  char *host = url->data + prot_size;
  char *file = url->data + prot_size + host_size;
  char *user = url->data + prot_size + host_size + file_size;
  str = psprintf("%s://%s@%s:%d/%s", protocol, user, host, port, file);
  PG_RETURN_CSTRING(str);
}
/*
PG_FUNCTION_INFO_V1(url_recv);
Datum
url_rcv(PG_FUNCTION_ARGS)
{
  StringInfo buf = (StringInfo) PG_GETARG_POINTER(0);
  pg_url *url = (pg_url *) palloc(VARHDRSZ + buf->len);
  SET_VARSIZE(url, VARHDRSZ + buf->len);
  memcpy(url->data, buf->data, buf->len);
  PG_RETURN_POINTER(url);
}

PG_FUNCTION_INFO_V1(url_send);
Datum
url_send(PG_FUNCTION_ARGS)
{
  pg_url *url = (pg_url *) PG_GETARG_POINTER(0);
  StringInfoData buf;
  pq_begintypsend(&buf);
  pq_sendint(&buf, url->protocol_len, 4);
  pq_sendint(&buf, url->host_len, 4);
  pq_sendint(&buf, url->port, 4);
  pq_sendint(&buf, url->file_len, 4);
  pq_sendbytes(&buf, url->data, url->protocol_len + url->host_len + url->file_len);
  PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}*/


// ---- Constructors

//Constructor URL(varchar protocol, varchar host, int port, varchar file)
PG_FUNCTION_INFO_V1(url_constructor_from_fields);
Datum
url_constructor_from_fields(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  int port = PG_GETARG_INT32(2);
  char *file = PG_GETARG_CSTRING(3);
  pg_url *url = create_url_from_fields(protocol, "", host, port, file);
  PG_RETURN_POINTER(url);
}

//Constructor URL(varchar protocol, varchar host, varchar file)
PG_FUNCTION_INFO_V1(url_constructor_from_fields_default_port);
Datum
url_constructor_from_fields_default_port(PG_FUNCTION_ARGS)
{
  char *protocol = PG_GETARG_CSTRING(0);
  char *host = PG_GETARG_CSTRING(1);
  char *file = PG_GETARG_CSTRING(2);
  pg_url *url = create_url_from_fields(protocol, "", host, default_port(protocol), file);
  PG_RETURN_POINTER(url);
}

//Constructor URL(varchar spec)
PG_FUNCTION_INFO_V1(url_constructor_from_string);
Datum
url_constructor_from_string(PG_FUNCTION_ARGS)
{
  char *str = PG_GETARG_CSTRING(0);
  pg_url *url = parse_url(str);
  PG_RETURN_POINTER(url);
}

// Constructor URL(varchar spec) but receives text instead of cstring
PG_FUNCTION_INFO_V1(url_constructor_from_text);
Datum
url_constructor_from_text(PG_FUNCTION_ARGS)
{
  text *str = PG_GETARG_TEXT_P(0);
  char *cstr = text_to_cstring(str);
  pg_url *url = parse_url(cstr);
  PG_RETURN_POINTER(url);
}

// TODO:  Constructor URL(URL context, varchar spec)
PG_FUNCTION_INFO_V1(url_constructor_context_spec);
Datum
url_constructor_context_spec(PG_FUNCTION_ARGS)
{
  //Get URL context
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);
  
   pg_url *resulting_url;
  //memcpy(resulting_url, "", 1);
  resulting_url = "";
  //Get URL spec
  char *spec_strng = PG_GETARG_CSTRING(1);
  //Cast spec into pg_url
  pg_url *spec_url = parse_url(spec_strng);
   

   //Spec data
   int spec_prot_size = spec_url->protocol_len;
   int spec_host_size = spec_url->host_len;
   int spec_file_size = spec_url->file_len;

   char *spec_path;
   
   
   //Context data
   int prot_size = url->protocol_len;
   int host_size = url->host_len;
   int file_size = url->file_len;
  
   char *context_scheme = url->data;
   char *spec_scheme = spec_url -> data;

   //result variables
       char *str_result;
       char *result_scheme= "";
		   char *result_authority= "";
		   char *result_host= "h";
		   int result_port;
		   char *result_path= "p";
		   char *result_query= "q";
		   char *result_file= "";
		   char *result_fragment= "f";

       //Regex to split spec
    regex_t spec_reegex;
    
    //compare spec_url to regex
    int value = regcomp( &spec_reegex,
    //   <scheme>       ://   <authority>   /<path>    ? <query>          #<fragment>
    "(([a-zA-Z0-9^/?#]+)://)?(([^/?#]*))?(\\/[^?#]*)?(\\?([a-zA-Z0-9^#]*))?(#(.*))?" 
    ,REG_EXTENDED); 
    regmatch_t spec_pmatch[10];

      //URL Generic syntax considered: 
      /*  expected matches from url:
            scheme    = $2
            authority = $4
            path      = $5
            query     = $7
            fragment  = $9
      */
      
    value = regexec( &spec_reegex, spec_url, 10, spec_pmatch, 0);
   
	if (spec_pmatch[2].rm_so == -1)//If spec has no scheme, the scheme component is inherited from the context URL.
  {
		   //--Get Context Protocol (aka Schema)
		   result_scheme = context_scheme;
		   //str_result = strcat(str_result, context_scheme);
		   
	    	if (spec_pmatch[4].rm_so == -1) //If spec has no authority, the authority of the new URL will be inherited from the context.
		  {
		  
			 // Get the protocol, host and the port
			  result_host = url->data + prot_size;
			  result_port = url->port;
			  
			  //Build the Authority
			  result_authority = psprintf("%s:%d", result_host, result_port);
			  
			  
			  if (spec_pmatch[5].rm_so == -1) //If spec doesnt have path
			  {
				    spec_path = malloc(1);
				    memcpy(spec_path, "", 1);
				    spec_path = "No path";
			  } else{// If spec has a path
			  ///---TODO: If the spec's path component begins with a slash character "/" then the path is treated as absolute and the spec path replaces the context path. 
			  //Otherwise, the path is treated as a relative path and is appended to the context path.
			  	//Get spec path
			  	//TODO: Fix - Not finding spec in result
				    char *spec_path_start = spec_url + spec_pmatch[5].rm_so;
				    size_t spec_path_length = spec_pmatch[5].rm_eo - spec_pmatch[5].rm_so;
				    spec_path = malloc(spec_path_length + 1);
				    memset(spec_path, 0, spec_path_length + 1);
				    memcpy(spec_path, spec_path_start, spec_path_length);
				    
				 // char *spec_file = spec_url->data + spec_prot_size + spec_host_size; // Get the file
				 //char *spec_path = "specpath"; //strtok(spec_file, "?");
				    
				//Get context path
				  char *context_file = url->data + prot_size + host_size; // Get the file
				  char *context_path = strtok(context_file, "?"); //Separate query from file
				   
				  result_path = strcat(context_path, spec_path);

			  }
			  
			  if (spec_pmatch[7].rm_so == -1)//If query is empty
			  {
			    if(spec_path == "") //If path is empty and scheme, authority, and query are undefined, then the new URL is a reference to the current document. Otherwise, the fragment and query parts present in the spec are used in the new URL.???
			    {
			    	//TODO
			    } 
			    //TODO: Fix - Allways entering here, even when spec has a query
			    result_query = "entraifnoQ";
			    
			  } else{
			  
			    char *spec_query_start = spec_url + spec_pmatch[7].rm_so;
			    size_t spec_query_length = spec_pmatch[7].rm_eo - spec_pmatch[7].rm_so;
			    result_query = malloc(spec_query_length + 1);
			    memset(result_query, 0, spec_query_length + 1);
			    memcpy(result_query, spec_query_start, spec_query_length);
			    result_query = "entraQ";
			  }
			  
			  if (spec_pmatch[7].rm_so == -1)
			  {
			    result_fragment = malloc(1);
			    memcpy(result_fragment, "", 1);
			    result_fragment = "insideifNofragment";
			  } else{
			    char *result_fragment_start = spec_url + spec_pmatch[9].rm_so;
			    size_t result_fragment_length = spec_pmatch[9].rm_eo - spec_pmatch[9].rm_so;
			    result_fragment = malloc(result_fragment_length + 1);
			    memset(result_fragment, 0, result_fragment_length + 1);
			    memcpy(result_fragment, result_fragment_start, result_fragment_length);
			    result_fragment = "insideifhasfragment";

			  }
	   
		    	result_file= psprintf("%s?%s#%s", result_path, result_query, result_fragment);
		  } else{ //If spec has authority, then the spec authority and path will replace the context authority and path. 

		  //TODO: Fix -- Never entering this condition
		    char *result_authority_start = spec_url + spec_pmatch[4].rm_so;
		    size_t spec_authority_length = spec_pmatch[4].rm_eo - spec_pmatch[4].rm_so;
		    result_authority = malloc(spec_authority_length + 1);
		    memset(result_authority, 0, spec_authority_length + 1);
		    memcpy(result_authority, result_authority_start, spec_authority_length);
		    
		    // Get the protocol, host and the port
			  result_host = spec_url->data + spec_prot_size;
			  result_port = spec_url->port;
		  }
	    
	    resulting_url = create_url_from_fields(result_scheme, result_host, result_port, result_file);

	  
	  } else{ //If spec has scheme ---> then the new URL is created as an absolute URL based on the spec alone.

      //TODO: Fix - This condition is never happening
		   resulting_url=spec_url;
	    }
  
  regfree(&spec_reegex);
  PG_RETURN_POINTER(resulting_url); //(resulting_url);
}

// ---- Methods

// varchar getAuthority() : Gets the authority part of this URL.
PG_FUNCTION_INFO_V1(getAuthority);
Datum
getAuthority(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);
  
  // Get the  of the host
  int prot_size = url->protocol_len;

  // Get the host length
  int host_size = url->host_len;

  // Allocate enough memory for the string
  char *str = palloc(host_size+4+1);

  
  // Get the host and the port
  char *host = url->data + prot_size;
  int port = url->port;
  
  //Build the Authority
  str = psprintf("%s:%d", host, port);
  
  
  PG_RETURN_CSTRING(str);
}


// int getDefaultPort(): Gets the default port number of the protocol associated with this URL.
PG_FUNCTION_INFO_V1(get_default_port);
Datum
get_default_port(PG_FUNCTION_ARGS)
{
  pg_url *url = (pg_url *) PG_GETARG_POINTER(0);
  // Get protocol and default port associated to it
  char *protocol = url->data;
  int port = default_port(protocol);
  PG_RETURN_INT32(port);
}

//Gets the file name of this URL.
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

//Gets the userinfo
PG_FUNCTION_INFO_V1(get_userinfo);
Datum
get_userinfo(PG_FUNCTION_ARGS)
{
 struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

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


//Gets the host name of this URL, if applicable.
PG_FUNCTION_INFO_V1(get_host);
Datum
get_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url = (pg_url *)(&(url_buf->vl_dat));
  url = (pg_url *) pg_detoast_datum(url_buf);

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

//varchar getPath() Gets the path part of this URL.

PG_FUNCTION_INFO_V1(getPath);
Datum
getPath(PG_FUNCTION_ARGS)
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
  char *ptr = strtok(file, "?");
  
  //str = psprintf("%s", ptr);
  PG_RETURN_CSTRING(file);
}



//Gets the port number of this URL.
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

//Gets the protocol name of this URL.
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

// varchar getQuery() Gets the query part of this URL.
PG_FUNCTION_INFO_V1(getQuery);
Datum
getQuery(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  char *url = (char * ) (&(url_buf->vl_dat));
  url = (char *) pg_detoast_datum(url_buf);

  regex_t reegex;
  
  int value = regcomp( &reegex,
  //"([a-zA-Z0-9\\\^:./?#\\-]*)*(\\?([^#]*))?(#(.*))?"
  "(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?" //URL estandar
  //"(([a-zA-Z0-9]+):\\/\\/)*((www.)?[a-zA-Z0-9]+\\.[a-zA-Z0-9]*)(:([0-9]+))?(/([a-zA-Z0-9\\?_&=\\-]+))?" //Jose
   ,REG_EXTENDED); //"([a-zA-Z0-9\\\^:/?#\\-]*)*(\\?([^#]*))?(#(.*))?", REG_EXTENDED);  //If the regcomp() function is successful, it returns 0
  regmatch_t pmatch[10];
	    /*
	  5 expected matches from url:
		      scheme    = $2
		      authority = $4
		      path      = $5
		      query     = $7
		      fragment  = $9

	  */
	  
   value = regexec( &reegex, url, 10, pmatch, 0);
	  
	  // Extract the query
   char *query;
   char *str;

  if (pmatch[7].rm_so == -1)
  {
    query = malloc(1);
    memcpy(query, "", 1);
    query = "No query found. Verify if the URL is written properly";
  } else{
    char *query_start = url + pmatch[7].rm_so;
    size_t query_length = pmatch[7].rm_eo - pmatch[7].rm_so;
    query = malloc(query_length + 1);
    memset(query, 0, query_length + 1);
    memcpy(query, query_start, query_length);

  }

  regfree(&reegex);
  
  str = psprintf("%s", query);
  
  PG_RETURN_CSTRING(str);
}


//Constructs a string representation of this URL.
PG_FUNCTION_INFO_V1(toString);
Datum
getRef(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  char *url = (char * ) (&(url_buf->vl_dat));
  url = (char *) pg_detoast_datum(url_buf);

  regex_t reegex;
  
  int value = regcomp( &reegex,
  //"([a-zA-Z0-9\\\^:./?#\\-]*)*(\\?([^#]*))?(#(.*))?"
  "(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?" //URL estandar
  //"(([a-zA-Z0-9]+):\\/\\/)*((www.)?[a-zA-Z0-9]+\\.[a-zA-Z0-9]*)(:([0-9]+))?(/([a-zA-Z0-9\\?_&=\\-]+))?" //Jose
   ,REG_EXTENDED); //"([a-zA-Z0-9\\\^:/?#\\-]*)*(\\?([^#]*))?(#(.*))?", REG_EXTENDED);  //If the regcomp() function is successful, it returns 0
  regmatch_t pmatch[10];
	    /*
	  5 expected matches from url:
		      scheme    = $2
		      authority = $4
		      path      = $5
		      query     = $7
		      fragment  = $9

	  */
	  
   value = regexec( &reegex, url, 10, pmatch, 0);
	  
	  // Extract the query
   char *query;
   char *str;

  if (pmatch[9].rm_so == -1)
  {
    query = malloc(1);
    memcpy(query, "", 1);
    query = "No query found. Verify if the URL is written properly";
  } else{
    char *query_start = url + pmatch[9].rm_so;
    size_t query_length = pmatch[9].rm_eo - pmatch[9].rm_so;
    query = malloc(query_length + 1);
    memset(query, 0, query_length + 1);
    memcpy(query, query_start, query_length);

  }

  regfree(&reegex);
  
  str = psprintf("%s", query);
  
  PG_RETURN_CSTRING(str);
}

// TODO: String getRef() Gets the anchor (also known as the "reference") of this URL.
// TODO: String getUserInfo() Gets the userInfo part of this URL.

  regex_t reegex;
  
  //compare input to regex
  int value = regcomp( &reegex,
  "(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?" //regex of URL estandar
   ,REG_EXTENDED); 
  regmatch_t pmatch[10];
	    /*
	   expected matches from url:
		      scheme    = $2
		      authority = $4
		      path      = $5
		      query     = $7
		      fragment  = $9

	  */
	  
   value = regexec( &reegex, url, 10, pmatch, 0);
	  
	  // Extract the query
   char *query;
   char *str;

	//Assign match to variable 'query'
  if (pmatch[7].rm_so == -1)
  {
    query = malloc(1);
    memcpy(query, "", 1);
    query = "No query found. Verify if the URL is written properly";
  } else{
    char *query_start = url + pmatch[7].rm_so;
    size_t query_length = pmatch[7].rm_eo - pmatch[7].rm_so;
    query = malloc(query_length + 1);
    memset(query, 0, query_length + 1);
    memcpy(query, query_start, query_length);

  }

  regfree(&reegex);
  
  str = psprintf("%s", query);
  
  PG_RETURN_CSTRING(str);
}

//String getRef() Gets the anchor (also known as the "reference") of this URL.
PG_FUNCTION_INFO_V1(getRef);
Datum
getRef(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf = (struct varlena*) PG_GETARG_VARLENA_P(0);
  char *url = (char * ) (&(url_buf->vl_dat));
  url = (char *) pg_detoast_datum(url_buf);

  regex_t reegex;
  
  int value = regcomp( &reegex,
  "(([^:/?#]+):)?(//([^/?#]*))?([^?#]*)(\\?([^#]*))?(#(.*))?" //regex of URL estandar
   ,REG_EXTENDED); 
  regmatch_t pmatch[10];
	    /*
	   expected matches from url:
		      scheme    = $2
		      authority = $4
		      path      = $5
		      query     = $7
		      fragment  = $9

	  */
	  
   value = regexec( &reegex, url, 10, pmatch, 0);
	  
	  // Extract the reference
   char *ref;
   char *str;

//Assign match to variable 'ref'
  if (pmatch[9].rm_so == -1)
  {
    ref = malloc(1);
    memcpy(ref, "", 1);
    ref = "No reference found. Verify if the URL is written properly";
  } else{
    char *ref_start = url + pmatch[9].rm_so;
    size_t ref_length = pmatch[9].rm_eo - pmatch[9].rm_so;
    ref = malloc(ref_length + 1);
    memset(ref, 0, ref_length + 1);
    memcpy(ref, ref_start, ref_length);

  }

  regfree(&reegex);
  
  str = psprintf("%s", ref);
  
  PG_RETURN_CSTRING(str);
}

// TODO: String getUserInfo() Gets the userInfo part of this URL.

//Constructs a string representation of this URL.
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

// ---- URLs functions supported by INDEXING

/* pg_url_cmp: compares two url
* returns -1 if url1 < url2
* returns 0 if url1 == url2
* returns 1 if url1 > url2
*/
PG_FUNCTION_INFO_V1(pg_url_cmp);
Datum
pg_url_cmp(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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

//boolean equals(URL url1, URL url2): Compares two URLs for equality. This operation must be index-supported.
PG_FUNCTION_INFO_V1(equals);
Datum
equals(PG_FUNCTION_ARGS)
{
  //Get value for url1
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  //Get value for url2
  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  //Compare components
  bool result = (url1->port == url2->port) // Check the port
    && (url1->file_len == url2->file_len) // Check the file length
    && (url1->host_len == url2->host_len) // Check the host length
    && (url1->protocol_len == url2->protocol_len);
  
  // If the components match, compare string values
  if(result){
    // Get the strings
    char *str1 = internal_to_string(url1);
    char *str2 = internal_to_string(url2);

    // Compare the strings
    result = strcmp(str1, str2) == 0; //if strings are equal
    PG_RETURN_BOOL(result);
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

// Check if the hosts of two urls are equal
PG_FUNCTION_INFO_V1(same_host);
Datum
same_host(PG_FUNCTION_ARGS)
{
  //Get value for url1
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  //Get value for url2
  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);
  //Compare components
  bool result = (url1->host_len == url2->host_len);
  
  // If the components match, compare their host part
  if(result){
    char *host1 = url1->data + url1->protocol_len;
    char *host2 = url2->data + url2->protocol_len;
    result = strcmp(host1, host2) == 0; //if strings are equal
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

// Check if the files of two urls are equal
PG_FUNCTION_INFO_V1(same_file);
Datum
same_file(PG_FUNCTION_ARGS)
{
  //Get value for url1
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  //Get value for url2
  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  //Compare components
  bool result = (url1->file_len == url2->file_len);
  
  // If the components match, compare their file part
  if(result){
    char *file1 = url1->data + url1->protocol_len + url1->host_len;
    char *file2 = url2->data + url2->protocol_len + url2->host_len;
    result = strcmp(file1, file2) == 0; //if strings are equal
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}

// Check if the hosts of two urls are different
PG_FUNCTION_INFO_V1(different_host);
Datum
different_host(PG_FUNCTION_ARGS)
{
  //Get value for url1
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  //Get value for url2
  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  //Compare components
  bool result = (url1->host_len == url2->host_len);
  
  // If the components match, compare their host part
  if(result){
    char *host1 = url1->data + url1->protocol_len;
    char *host2 = url2->data + url2->protocol_len;
    result = strcmp(host1, host2) != 0; //if strings are different
  }
  // If the lengths are not the same, the urls are not equal
  PG_RETURN_BOOL(result);
}


// Not Equals function for Indexing
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

// Equals function for Indexing
PG_FUNCTION_INFO_V1(less_than);
Datum
less_than(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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


PG_FUNCTION_INFO_V1(less_than_or_equal);
Datum
less_than_or_equal(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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

PG_FUNCTION_INFO_V1(greater_than);
Datum
greater_than(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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

PG_FUNCTION_INFO_V1(greater_than_or_equal);
Datum
greater_than_or_equal(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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

PG_FUNCTION_INFO_V1(less_than_host);
Datum
less_than_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 < host2?
  bool result = strcmp(host1, host2) < 0;

  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(less_than_or_equal_host);
Datum
less_than_or_equal_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 <= host2?
  bool result = strcmp(host1, host2) <= 0;

  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(greater_than_host);
Datum
greater_than_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 > host2?
  bool result = strcmp(host1, host2) > 0;

  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(greater_than_or_equal_host);
Datum
greater_than_or_equal_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

  // Compute hosts
  char *host1 = url1->data + url1->protocol_len;
  char *host2 = url2->data + url2->protocol_len;
  // is host1 >= host2?
  bool result = strcmp(host1, host2) >= 0;

  PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(pg_url_cmp_host);
Datum
pg_url_cmp_host(PG_FUNCTION_ARGS)
{
  struct varlena* url_buf1 = (struct varlena*) PG_GETARG_VARLENA_P(0);
  pg_url *url1 = (pg_url *)(&(url_buf1->vl_dat));
  url1 = (pg_url *) pg_detoast_datum(url_buf1);

  struct varlena* url_buf2 = (struct varlena*) PG_GETARG_VARLENA_P(1);
  pg_url *url2 = (struct pg_url*)(&(url_buf2->vl_dat));
  url2 = (pg_url *) pg_detoast_datum(url_buf2);

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
