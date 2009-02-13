/* 
  Copyright (c) <2007> <Barbara Philippot - Olivier Courtin>

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE. 
*/

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

#include "ows.h"
#include "../ows_define.h"

char *ows_strdup(char *s)
{
    char *s1;

    if(!s)
        return(NULL);

    s1 = (char *)malloc(strlen(s) + 1);
    if(!s1)
        return(NULL);

    strcpy(s1,s);
    return(s1);
}
/*
 * Return config file path
 */
char *ows_get_config_path()
{
    const char *config_path = OWS_CONFIG_FILE_PATH;
    const char *config_env_var = getenv("TINYOWS_CONFIG_FILE");

    if(config_env_var != NULL)
    {
        return ows_strdup((char*) config_env_var);
    }
    else
    {
        return ows_strdup((char*) config_path);
    }
}

/*
 * Return schema dir
 */
char *ows_get_schema_path(char *schema_type)
{
    
    char *schema_dir = NULL;
    const char *schema_env_var = getenv("TINYOWS_SCHEMA_DIR");

    if(schema_env_var != NULL)
    {
        schema_dir = (char *) malloc(sizeof(char *) * (strlen(schema_env_var) + 
                                                     strlen(schema_type) + 2 ));
        sprintf(schema_dir, "%s/%s", schema_env_var, schema_type); 
    }
    else
    {
        if( !strcmp(schema_type, WFS_SCHEMA_100_BASIC))
        {
            schema_dir = (char *) malloc( sizeof(char *) * 
                                          (strlen(WFS_SCHEMA_100_BASIC_PATH) ));
            sprintf(schema_dir, "%s", WFS_SCHEMA_100_BASIC_PATH); 
        }
        else if(!strcmp(schema_type, WFS_SCHEMA_100_TRANS))
        {
            schema_dir = (char *) malloc( sizeof(char *) * 
                                          (strlen(WFS_SCHEMA_100_TRANS_PATH) ));
            sprintf(schema_dir, "%s", WFS_SCHEMA_100_TRANS_PATH); 
        }
        else if(!strcmp(schema_type, WFS_SCHEMA_110))
        {
            schema_dir = (char *) malloc( sizeof(char *) * 
                                          (strlen(WFS_SCHEMA_110_PATH) ));
            sprintf(schema_dir, "%s", WFS_SCHEMA_110_PATH); 
        }
    }
    return ows_strdup(schema_dir);
}



/* 
 * Connect the ows to the database specified in configuration file
 */
static void ows_pg(ows * o, char *con_str)
{
	assert(o != NULL);
	assert(con_str != NULL);

	o->pg = PQconnectdb(con_str);

	if (PQstatus(o->pg) != CONNECTION_OK)
		ows_error(o, OWS_ERROR_CONNECTION_FAILED,
		   "connection to database failed", "init_OWS");
}


/*
 * Initialize an ows struct 
 */
static ows *ows_init()
{
	ows *o;

	o = malloc(sizeof(ows));
	assert(o != NULL);

	o->request = NULL;
	o->cgi = NULL;
	o->psql_requests = NULL;
	o->pg = NULL;
	o->pg_dsn = NULL;
	o->output = NULL;

	o->layers = NULL;

	o->max_width = 0;
	o->max_height = 0;
	o->max_layers = 0;
	o->max_features = 0;
	o->max_geobbox = NULL;

	o->metadata = NULL;
	o->contact = NULL;

	o->sld_path = NULL;
	o->sld_writable = 0;

	return o;
}


/* 
 * Flush an ows structure
 * Used for debug purpose
 */
#ifdef OWS_DEBUG
void ows_flush(ows * o, FILE * output)
{
	assert(o != NULL);
	assert(output != NULL);

	if (o->pg_dsn != NULL)
	{
		fprintf(output, "pg: ");
		buffer_flush(o->pg_dsn, output);
		fprintf(output, "\n");
	}
	if (o->metadata != NULL)
	{
		fprintf(output, "metadata: ");
		ows_metadata_flush(o->metadata, output);
		fprintf(output, "\n");
	}
	if (o->contact != NULL)
	{
		fprintf(output, "contact: ");
		ows_contact_flush(o->contact, output);
		fprintf(output, "\n");
	}

	if (o->cgi != NULL)
	{
		fprintf(output, "cgi: ");
		array_flush(o->cgi, output);
		fprintf(output, "\n");
	}
	if (o->psql_requests != NULL)
	{
		fprintf(output, "SQL requests: ");
		list_flush(o->psql_requests, output);
		fprintf(output, "\n");
	}

	if (o->layers != NULL)
	{
		fprintf(output, "layers: ");
		ows_layer_list_flush(o->layers, output);
		fprintf(output, "\n");
	}

	if (o->request != NULL)
	{
		fprintf(output, "request: ");
		ows_request_flush(o->request, output);
		fprintf(output, "\n");
	}

	fprintf(output, "max_width: %d\n", o->max_width);
	fprintf(output, "max_height: %d\n", o->max_height);
	fprintf(output, "max_layers: %d\n", o->max_layers);
	fprintf(output, "max_features: %d\n", o->max_features);
	if (o->max_geobbox != NULL)
	{
		fprintf(output, "max_geobbox: ");
		ows_geobbox_flush(o->max_geobbox, output);
		fprintf(output, "\n");
	}

	if (o->sld_path != NULL)
	{
		fprintf(output, "sld_path: ");
		buffer_flush(o->sld_path, output);
		fprintf(output, "\n");
	}
	fprintf(output, "sld_writable: %d\n", o->sld_writable);
}
#endif


/*
 * Release ows struct 
 */
void ows_free(ows * o)
{
	assert(o != NULL);

	if (o->pg != NULL)
		PQfinish(o->pg);

	if (o->pg_dsn != NULL)
		buffer_free(o->pg_dsn);

	if (o->cgi != NULL)
		array_free(o->cgi);

	if (o->psql_requests != NULL)
		list_free(o->psql_requests);

	if (o->layers != NULL)
		ows_layer_list_free(o->layers);

	if (o->request != NULL)
		ows_request_free(o->request);

	if (o->max_geobbox != NULL)
		ows_geobbox_free(o->max_geobbox);

	if (o->metadata != NULL)
		ows_metadata_free(o->metadata);

	if (o->contact != NULL)
		ows_contact_free(o->contact);

	if (o->sld_path != NULL)
		buffer_free(o->sld_path);

	free(o);
	o = NULL;
}


void ows_usage(ows * o) 
{
    char *config_path;
    char *schema_path;

    printf("TinyOWS should be called by CGI throw a Web Server !\n\n");
    printf("___________\n");
    config_path = ows_get_config_path();
    printf("Config File Path: %s\n", config_path);
    printf("PostGIS dsn: '%s'\n", o->pg_dsn->buf);
    printf("___________\n");
    schema_path = ows_get_schema_path(WFS_SCHEMA_100_BASIC);
    printf("WFS 1.0.0 Basic Schema Path: %s\n", schema_path);
    schema_path = ows_get_schema_path(WFS_SCHEMA_100_TRANS);
    printf("WFS 1.0.0 Transactional Schema Path: %s\n", schema_path);
    schema_path = ows_get_schema_path(WFS_SCHEMA_110);
    printf("WFS 1.1.0 Schema Path: %s\n", schema_path);
    printf("___________\n");

    free(config_path);
    free(schema_path);
   
}


int main(int argc, char *argv[])
{
    char *query, *config_path;
    ows *o;

    o = ows_init();

    /* TODO add an alternative cache system */
    o->output = stdout;

    /* retrieve the query in HTTP request */
    query = cgi_getback_query(o);

    config_path = ows_get_config_path();

    if (query == NULL || strlen(query) == 0) {
        if (argc > 1 && (strncmp(argv[1], "--help", 6) == 0
                         || strncmp(argv[1], "-h", 2) == 0)) {
            ows_parse_config(o, config_path);
            ows_usage(o);
        } else ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                         "Service Unknown", "service");

        return EXIT_SUCCESS;
    }

    /* 
     * Request encoding and HTTP method WFS 1.1.0 -> 6.5
     */

    o->request = ows_request_init();
    /* GET could only handle KVP */
    if (cgi_method_get()) o->request->method = OWS_METHOD_KVP;
    /* POST could handle KVP or XML encoding */
    else if (cgi_method_post()) {
        /* WFS 1.1.0 mandatory */
        if (strcmp(getenv("CONTENT_TYPE"),
                   "application/x-www-form-urlencoded") == 0)
            o->request->method = OWS_METHOD_KVP;
        else if (strcmp(getenv("CONTENT_TYPE"), "text/xml") == 0)
            o->request->method = OWS_METHOD_XML;
        /* WFS 1.0.0 && CITE Test compliant */
        else if (strcmp(getenv("CONTENT_TYPE"), "application/xml") == 0 ||
                 strcmp(getenv("CONTENT_TYPE"), "application/xml; charset=UTF-8") == 0)
            o->request->method = OWS_METHOD_XML;
    	/* Command line Unit Test cases with XML values (not HTTP) */
    } else if (!cgi_method_post() && !cgi_method_get() && query[0] == '<')
        o->request->method = OWS_METHOD_XML;
    else ows_error(o, OWS_ERROR_REQUEST_HTTP,
                   "Wrong HTTP request Method", "http");

    switch(o->request->method) 
    {
        case OWS_METHOD_KVP:
            o->cgi = cgi_parse_kvp(o, query);
            break;
        case OWS_METHOD_XML:
            o->cgi = cgi_parse_xml(o, query);
            break;

        default: ows_error(o, OWS_ERROR_REQUEST_HTTP,
                         "Wrong HTTP request Method", "http");
    }

    o->psql_requests = list_init();

    /* Parse the configuration file and initialize ows struct */
    ows_parse_config(o, config_path);

    /* Connect the ows to the database */
    ows_pg(o, o->pg_dsn->buf);

    /* Fill service's metadata */
    ows_metadata_fill(o, o->cgi);

    /* Process service request */
    ows_request_check(o, o->request, o->cgi, query);

    /* Run the right OWS service */
    switch (o->request->service)
    {
        case WMS:
            o->request->request.wms = wms_request_init();
            wms_request_check(o, o->request->request.wms, o->cgi);
            wms(o, o->request->request.wms);
            break;
        case WFS:
            o->request->request.wfs = wfs_request_init();
            wfs_request_check(o, o->request->request.wfs, o->cgi);
            wfs(o, o->request->request.wfs);
            break;
        default:
            ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                  "Service Unknown", "service");
    }
  
    ows_free(o);

    return EXIT_SUCCESS;
}

    
/*
 * vim: expandtab sw=4 ts=4 
 */
