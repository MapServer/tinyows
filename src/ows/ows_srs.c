/*
  Copyright (c) <2007-2012> <Barbara Philippot - Olivier Courtin>

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
#include <ctype.h>
#include <libpq-fe.h>
#include <string.h>

#include "ows.h"


/*
 * Initialize proj structure
 */
ows_srs *ows_srs_init()
{
  ows_srs *c;

  c = malloc(sizeof(ows_srs));
  assert(c);

  c->srid = -1;
  c->auth_name = buffer_init();
  c->auth_srid = 0;
  c->is_geographic = true;
  c->honours_authority_axis_order = false;
  c->is_axis_order_gis_friendly = false;
  c->is_long = false;

  return c;
}


ows_srs *ows_srs_copy(ows_srs * d, ows_srs * s)
{
  assert(s);
  assert(d);

  d->srid = s->srid;
  buffer_copy(d->auth_name, s->auth_name);
  d->auth_srid = s->auth_srid;
  d->is_geographic = s->is_geographic;
  d->honours_authority_axis_order = s->honours_authority_axis_order;
  d->is_axis_order_gis_friendly = s->is_axis_order_gis_friendly;
  d->is_long = s->is_long;

  return d;
}


/*
 * Free proj structure
 */
void ows_srs_free(ows_srs * c)
{
  assert(c);

  buffer_free(c->auth_name);
  free(c);
  c = NULL;
}


#ifdef OWS_DEBUG
/*
 * Print ows structure state into a file
 * (mainly for debug purpose)
 */
void ows_srs_flush(ows_srs * c, FILE * output)
{
  assert(c);
  assert(output);

  fprintf(output, "[\n");
  fprintf(output, " srid: %i\n", c->srid);
  fprintf(output, " auth_name: %s\n", c->auth_name->buf);
  fprintf(output, " auth_srid: %i\n", c->auth_srid);

  if (c->is_geographic)
    fprintf(output, " is_geographic: true\n]\n");
  else
    fprintf(output, " is_geographic: false\n]\n");

  if (c->honours_authority_axis_order)
    fprintf(output, " honours_authority_axis_order: true\n]\n");
  else
    fprintf(output, " honours_authority_axis_order: false\n]\n");

  if (c->is_axis_order_gis_friendly)
    fprintf(output, " is_axis_order_gis_friendly: true\n]\n");
  else
    fprintf(output, " is_axis_order_gis_friendly: false\n]\n");

  if (c->is_long)
    fprintf(output, " is_long: true\n]\n");
  else
    fprintf(output, " is_long: false\n]\n");
}
#endif

/*
 * Set s->is_geographic and s->is_axis_order_gis_friendly
 * from srtext.
 */
static void ows_srs_set_is_geographic_and_is_axis_order_gis_friendly_from_def(
        ows_srs* s, const char* proj4text, const char* srtext)
{
    /* Use srtext in priority */
    if (srtext && srtext[0] != '\0')
    {
        char* srtext_horizontal = (char*) malloc(strlen(srtext) + 1);
        char* ptr;
        strcpy(srtext_horizontal, srtext);

        /* Remove the VERT_CS part if we are in a COMPD_CS */
        ptr = strstr(srtext_horizontal, ",VERT_CS[");
        if (ptr)
            *ptr = '\0';

        s->is_axis_order_gis_friendly = true;
        s->is_geographic = (strstr(srtext, "PROJCS[") == NULL &&
                            strstr(srtext, "GEOCCS[") == NULL &&
                            strstr(srtext, "BOUNDCRS[") == NULL) ||
                           strstr(srtext, "BOUNDCRS[SOURCECRS[GEOGCRS") != NULL;

        if( strstr(srtext_horizontal, "AXIS[") == NULL &&
            strstr(srtext_horizontal, "GEOCCS[") == NULL )
        {
            /* If there is no axis definition, then due to how GDAL < 3
            * generated the WKT, this means that the axis order is not
            * GIS friendly */
            s->is_axis_order_gis_friendly = false;
        }
        else if( strstr(srtext_horizontal,
                    "AXIS[\"Latitude\",NORTH],AXIS[\"Longitude\",EAST]") != NULL )
        {
            s->is_axis_order_gis_friendly = false;
        }
        else if( strstr(srtext_horizontal,
                    "AXIS[\"Northing\",NORTH],AXIS[\"Easting\",EAST]") != NULL )
        {
            s->is_axis_order_gis_friendly = false;
        }
        else if( strstr(srtext_horizontal,
                    "AXIS[\"geodetic latitude (Lat)\",north,ORDER[1]") != NULL )
        {
            s->is_axis_order_gis_friendly = false;
        }

        free(srtext_horizontal);
    }
    else if( proj4text && proj4text[0] != '\0' )
    {
        s->is_geographic = strstr(proj4text, "+units=m") == NULL;
        /* This will be wrong for a number of projected CRS ! */
        s->is_axis_order_gis_friendly = !s->is_geographic;
    }
}

/*
 * Set projection value into srs structure
 */
bool ows_srs_set(ows * o, ows_srs * s, const buffer * auth_name, int auth_srid)
{
  PGresult *res;
  buffer *sql;
  const char* proj4text;
  const char* srtext;

  assert(o);
  assert(s);
  assert(o->pg);
  assert(auth_name);

  sql = buffer_init();
  buffer_add_str(sql, "SELECT srid, proj4text, srtext "
                      "FROM spatial_ref_sys WHERE auth_name='");
  buffer_copy(sql, auth_name);
  buffer_add_str(sql, "' AND auth_srid=");
  buffer_add_int(sql, auth_srid);

  res = ows_psql_exec(o, sql->buf);
  buffer_free(sql);

  /* If query dont return exactly 1 result, it means projection is not handled */
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
    PQclear(res);
    return false;
  }

  buffer_empty(s->auth_name);
  buffer_copy(s->auth_name, auth_name);
  s->auth_srid = auth_srid;

  s->srid = atoi(PQgetvalue(res, 0, 0));

  proj4text = PQgetvalue(res, 0, 1);
  srtext = PQgetvalue(res, 0, 2);
  ows_srs_set_is_geographic_and_is_axis_order_gis_friendly_from_def(s, proj4text, srtext);

  PQclear(res);
  return true;
}


/*
 * Aim is to avoid all pg connection,
 * not a big deal as we already know values
 */
bool ows_srs_set_geobbox(ows * o, ows_srs * s)
{
  assert(o);
  assert(s);

  s->srid = 4326;
  buffer_empty(s->auth_name);
  buffer_add_str(s->auth_name, "EPSG");
  s->auth_srid = 4326;
  s->is_geographic = true;
  s->is_axis_order_gis_friendly = false;

  return true;
}


/*
 * Set projection value into srs structure
 */
bool ows_srs_set_from_srid(ows * o, ows_srs * s, int srid)
{
  PGresult *res;
  buffer *sql;
  const char *proj4text;
  const char *srtext;

  assert(o);
  assert(s);

  if (srid == -1 || srid == 0) {
    s->srid = -1;
    buffer_empty(s->auth_name);
    s->auth_srid = 0;
    s->is_geographic = true;
    s->honours_authority_axis_order = false;
    s->is_axis_order_gis_friendly = false;

    return true;
  }

  sql = buffer_init();
  buffer_add_str(sql, "SELECT auth_name, auth_srid, proj4text, srtext "
                      "FROM spatial_ref_sys WHERE srid = '");
  buffer_add_int(sql, srid);
  buffer_add_str(sql, "'");

  res = ows_psql_exec(o, sql->buf);
  buffer_free(sql);

  /* If query dont return exactly 1 result, it mean projection not handled */
  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
    PQclear(res);
    return false;
  }

  buffer_add_str(s->auth_name, PQgetvalue(res, 0, 0));
  s->auth_srid = atoi(PQgetvalue(res, 0, 1));
  s->srid = srid;

  proj4text = PQgetvalue(res, 0, 2);
  srtext = PQgetvalue(res, 0, 3);
  ows_srs_set_is_geographic_and_is_axis_order_gis_friendly_from_def(s, proj4text, srtext);

  PQclear(res);
  return true;
}


/*
 * Set projection value into srs structure
 */
bool ows_srs_set_from_srsname(ows * o, ows_srs * s, const char *srsname)
{
  char sep;
  int srid = -1;
  const char *p = NULL;

  assert(o);
  assert(s);
  assert(srsname);

  /* Severals srsName formats are available...
   *  cf WFS 1.1.0 -> 9.2 (p36)
   *  cf ISO 19142 -> 7.9.2.4.4 (p34)
   *  cf RFC 5165 <http://tools.ietf.org/html/rfc5165>
   *  cf CITE WFS-1.1 (GetFeature-tc17.2)
   */

  /* SRS pattern like:    EPSG:4326
                          urn:EPSG:geographicCRS:4326
                          urn:ogc:def:crs:EPSG:4326
                          urn:ogc:def:crs:EPSG::4326
                          urn:ogc:def:crs:EPSG:6.6:4326
                          urn:x-ogc:def:crs:EPSG:6.6:4326
                          http://www.opengis.net/gml/srs/epsg.xml#4326
                          spatialreferencing.org:900913
  */
  s->is_long = true;

  if    (!strncmp((char *) srsname,        "EPSG:", 5)
         || !strncmp((char *) srsname, "spatialreferencing.org", 22)) {
    sep = ':';
    s->honours_authority_axis_order = false;
    s->is_long = false;

  } else if (!strncmp((char *) srsname, "urn:ogc:def:crs:EPSG:", 21)
             || !strncmp((char *) srsname, "urn:x-ogc:def:crs:EPSG:", 23)
             || !strncmp((char *) srsname, "urn:EPSG:geographicCRS:", 23)) {
    sep = ':';
    s->honours_authority_axis_order = true;

  } else if (!strncmp((char *) srsname, "http://www.opengis.net/gml/srs/epsg.xml#", 40)) {
    sep = '#';
    s->honours_authority_axis_order = false;
  } else return false; /* FIXME must we really not allow other value ? */

  /*  Retrieve from last separator to the end of srsName string */
  for (p = srsname ; *p ; p++);
  for (--p ; *p != sep ; p--)
    if (!isdigit(*p)) return false;
  srid = atoi(++p);

  return ows_srs_set_from_srid(o, s, srid);
}


/*
 * Check if a layer's srs has meter or degree units
 */
bool ows_srs_meter_units(ows * o, buffer * layer_name)
{
  ows_layer_node * ln;

  assert(o);
  assert(layer_name);

  for (ln = o->layers->first ; ln ; ln = ln->next)
    if (ln->layer->name && ln->layer->storage && !strcmp(ln->layer->name->buf, layer_name->buf))
      return !ln->layer->storage->is_geographic;

  assert(0); /* Should not happen */
  return false;
}


/*
 * Retrieve a srs from a layer
 */
int ows_srs_get_srid_from_layer(ows * o, buffer * layer_name)
{
  ows_layer_node * ln;

  assert(o);
  assert(layer_name);

  for (ln = o->layers->first ; ln ; ln = ln->next)
    if (ln->layer->name && ln->layer->storage && !strcmp(ln->layer->name->buf, layer_name->buf))
      return ln->layer->storage->srid;

  return -1;
}


/*
 * Retrieve a list of srs from an srid list
 */
list *ows_srs_get_from_srid(ows * o, list * l)
{
  list_node *ln;
  buffer *b;
  list *srs;

  assert(o);
  assert(l);

  srs = list_init();

  if (l->size == 0) return srs;

  for (ln = l->first; ln ; ln = ln->next) {
    b = ows_srs_get_from_a_srid(o, atoi(ln->value->buf));
    list_add(srs, b);
  }

  return srs;
}


/*
 * Retrieve a srs from a srid
 */
buffer *ows_srs_get_from_a_srid(ows * o, int srid)
{
  buffer *b;
  buffer *sql;
  PGresult *res;

  assert(o);

  sql = buffer_init();
  buffer_add_str(sql, "SELECT auth_name||':'||auth_srid AS srs ");
  buffer_add_str(sql, "FROM spatial_ref_sys ");
  buffer_add_str(sql, "WHERE srid=");
  buffer_add_int(sql, srid);

  res = ows_psql_exec(o, sql->buf);
  buffer_free(sql);

  b = buffer_init();

  if (PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) != 1) {
    PQclear(res);
    return b;
  }

  buffer_add_str(b, PQgetvalue(res, 0, 0));

  PQclear(res);

  return b;
}


/*
 * vim: expandtab sw=4 ts=4
 */
