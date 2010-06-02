/*
  Copyright (c) <2007-2009> <Barbara Philippot - Olivier Courtin>

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
#include <time.h>

#include "../ows_define.h"
#include "ows.h"


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

    o->exit = false;
    o->request = NULL;
    o->cgi = NULL;
    o->psql_requests = NULL;
    o->pg = NULL;
    o->pg_dsn = NULL;
    o->output = stdout;
    o->config_file = NULL;
    o->online_resource = NULL;
    o->schema_dir = NULL;
    o->log_file = NULL;
    o->log = NULL;

    o->layers = NULL;

    o->max_width = 0;
    o->max_height = 0;
    o->max_layers = 0;
    o->max_features = 0;
    o->degree_precision = 6;
    o->meter_precision = 0;
    o->max_geobbox = NULL;
    o->wfs_display_bbox = false;

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

    fprintf(output, "exit : %d\n", o->exit?1:0);

    if (o->config_file != NULL) {
        fprintf(output, "config_file: ");
        buffer_flush(o->config_file, output);
        fprintf(output, "\n");
    }

    if (o->schema_dir != NULL) {
        fprintf(output, "schema_dir: ");
        buffer_flush(o->schema_dir, output);
        fprintf(output, "\n");
    }

    if (o->online_resource != NULL) {
        fprintf(output, "online_resource: ");
        buffer_flush(o->online_resource, output);
        fprintf(output, "\n");
    }

    if (o->pg_dsn != NULL) {
        fprintf(output, "pg: ");
        buffer_flush(o->pg_dsn, output);
        fprintf(output, "\n");
    }

    if (o->log_file != NULL) {
        fprintf(output, "log file: ");
        buffer_flush(o->log_file, output);
        fprintf(output, "\n");
    }

    if (o->metadata != NULL) {
        fprintf(output, "metadata: ");
        ows_metadata_flush(o->metadata, output);
        fprintf(output, "\n");
    }

    if (o->contact != NULL) {
        fprintf(output, "contact: ");
        ows_contact_flush(o->contact, output);
        fprintf(output, "\n");
    }

    if (o->cgi != NULL) {
        fprintf(output, "cgi: ");
        array_flush(o->cgi, output);
        fprintf(output, "\n");
    }

    if (o->psql_requests != NULL) {
        fprintf(output, "SQL requests: ");
        list_flush(o->psql_requests, output);
        fprintf(output, "\n");
    }

    if (o->layers != NULL) {
        fprintf(output, "layers: ");
        ows_layer_list_flush(o->layers, output);
        fprintf(output, "\n");
    }

    if (o->request != NULL) {
        fprintf(output, "request: ");
        ows_request_flush(o->request, output);
        fprintf(output, "\n");
    }

    fprintf(output, "max_width: %d\n", o->max_width);
    fprintf(output, "max_height: %d\n", o->max_height);
    fprintf(output, "max_layers: %d\n", o->max_layers);
    fprintf(output, "max_features: %d\n", o->max_features);
    fprintf(output, "degree_precision: %d\n", o->degree_precision);
    fprintf(output, "meter_precision: %d\n", o->meter_precision);

    if (o->max_geobbox != NULL) {
        fprintf(output, "max_geobbox: ");
        ows_geobbox_flush(o->max_geobbox, output);
        fprintf(output, "\n");
    }
    fprintf(output, "wfs_display_bbox: %d\n", o->wfs_display_bbox?1:0);

    if (o->sld_path != NULL) {
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

    if (o->config_file != NULL)
        buffer_free(o->config_file);

    if (o->schema_dir != NULL)
        buffer_free(o->schema_dir);

    if (o->online_resource != NULL)
        buffer_free(o->online_resource);

    if (o->pg != NULL)
        PQfinish(o->pg);

    if (o->log_file != NULL)
        buffer_free(o->log_file);

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
    fprintf(stderr, "TinyOWS version:   %s\n", TINYOWS_VERSION);
#if TINYOWS_FCGI
    fprintf(stderr, "FCGI support:      Yes\n");
#endif
    fprintf(stderr, "Config File Path:  %s\n", o->config_file->buf);
    fprintf(stderr, "PostGIS dsn:       %s\n", o->pg_dsn->buf);
    fprintf(stderr, "Schema dir:        %s\n", o->schema_dir->buf);

    if (o->log_file != NULL)
        fprintf(stderr, "Log file:          %s\n", o->log_file->buf);

    fprintf(stderr, "Available layers:\n");
    ows_layers_storage_flush(o, stderr);
}


static void ows_kvp_or_xml(ows *o, char *query)
{
    /*
     * Request encoding and HTTP method WFS 1.1.0 -> 6.5
     */

    /* GET could only handle KVP */
    if (cgi_method_get()) o->request->method = OWS_METHOD_KVP;

    /* POST could handle KVP or XML encoding */
    else if (cgi_method_post()) {
        /* WFS 1.1.0 mandatory */
        if (     !strcmp(getenv("CONTENT_TYPE"), "application/x-www-form-urlencoded"))
            o->request->method = OWS_METHOD_KVP;

        else if (!strcmp(getenv("CONTENT_TYPE"), "text/xml"))
            o->request->method = OWS_METHOD_XML;

        /* WFS 1.0.0 && CITE Test compliant */
        else if (!strcmp(getenv("CONTENT_TYPE"), "application/xml") ||
                 !strcmp(getenv("CONTENT_TYPE"), "application/xml; charset=UTF-8") ||
                 !strcmp(getenv("CONTENT_TYPE"), "text/plain"))
            o->request->method = OWS_METHOD_XML;

        /* Command line Unit Test cases with XML values (not HTTP) */
    } else if (!cgi_method_post() && !cgi_method_get() && query[0] == '<')
        o->request->method = OWS_METHOD_XML;
    else if (!cgi_method_post() && !cgi_method_get())
        o->request->method = OWS_METHOD_KVP;

    else ows_error(o, OWS_ERROR_REQUEST_HTTP, "Wrong HTTP request Method", "http");

}


int main(int argc, char *argv[])
{
    ows *o;
    char *query=NULL;

    o = ows_init();
    o->config_file = buffer_init();

    /* Config Files */
    if (getenv("TINYOWS_CONFIG_FILE") != NULL)
        buffer_add_str(o->config_file, getenv("TINYOWS_CONFIG_FILE"));
    else
        buffer_add_str(o->config_file, OWS_CONFIG_FILE_PATH);

    /* Parse the configuration file and initialize ows struct */
    if (!o->exit) ows_parse_config(o, o->config_file->buf);

    /* Connect the ows to the database */
    if (!o->exit) ows_pg(o, o->pg_dsn->buf);

    /* Fill layers storage metadata */
    if (!o->exit) ows_layers_storage_fill(o);

    /* Open Log file */
    if (o->log_file != NULL)
        o->log = fopen(o->log_file->buf, "a");


#if TINYOWS_FCGI
   while (FCGI_Accept() >= 0)
   {
#endif

    if (!o->exit) query = cgi_getback_query(o);

    if (query == NULL || strlen(query) == 0) {

    	/* Usage or Version command line options */
        if (argc > 1) {

		if (	!strncmp(argv[1], "--help", 6)
                     || !strncmp(argv[1], "-h", 2)     
                     || !strncmp(argv[1], "--check", 7))
			ows_usage(o);

        	else if (    !strncmp(argv[1], "--version", 9) 
			  || !strncmp(argv[1], "-v", 2))
			printf("%s\n", TINYOWS_VERSION);

        	else ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                             "Service Unknown", "service");
		o->exit = true;

	} else {
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE, 
                             "Service Unknown", "service");
	        o->exit = true;
        }
    } 

    /* Log input query if asked */
   if (o->log && query)
	fprintf(o->log, "[QUERY]\n%s\n", query);

    if (!o->exit) o->request = ows_request_init();
    if (!o->exit) ows_kvp_or_xml(o, query);

    if (!o->exit) {

    	switch (o->request->method) {
            case OWS_METHOD_KVP:
                o->cgi = cgi_parse_kvp(o, query);
                break;
            case OWS_METHOD_XML:
                o->cgi = cgi_parse_xml(o, query);
                break;

            default: ows_error(o, OWS_ERROR_REQUEST_HTTP,
			"Wrong HTTP request Method", "http");
        }
    }

    if (!o->exit) o->psql_requests = list_init();

    /* Fill service's metadata */
    if (!o->exit) ows_metadata_fill(o, o->cgi);

    /* Process service request */
    if (!o->exit) ows_request_check(o, o->request, o->cgi, query);

    /* Run the right OWS service */
    if (!o->exit) {
        switch (o->request->service) {
            case WFS:
                o->request->request.wfs = wfs_request_init();
                wfs_request_check(o, o->request->request.wfs, o->cgi);
                if (!o->exit) wfs(o, o->request->request.wfs);
                break;
            default:
                ows_error(o, OWS_ERROR_INVALID_PARAMETER_VALUE,
                      "Service Unknown", "service");
        }
    }

    if (o->request) {
        ows_request_free(o->request);
        o->request=NULL;
    }

#if TINYOWS_FCGI
    if (o->log) fprintf(o->log, "---\n"); 
    o->exit = false;
    }
    if (o->log) fprintf(o->log, "== FCGI SHUTDOWN == \n");
    OS_LibShutdown();
#endif
    if (o->log) fclose (o->log);
    ows_free(o);

    return EXIT_SUCCESS;
}


/*
 * vim: expandtab sw=4 ts=4
 */
