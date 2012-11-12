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
#include <float.h>
#include <math.h>
#include <string.h>

#include "../ows/ows.h"



/*
 * Return the boundaries of the features returned by the request
 */
static void wfs_gml_bounded_by(ows * o, wfs_request * wr, double xmin, double ymin, double xmax, double ymax, ows_srs * srs)
{
  int precision;

  assert(o);
  assert(wr);
  assert(srs);

  if (srs->is_degree) precision = o->degree_precision;
  else precision = o->meter_precision;

  if (!srs->srid || srs->srid == -1 || (xmin == ymin && xmax == ymax && xmin == xmax)) {
    if (ows_version_get(o->request->version) == 100)
      fprintf(o->output, "<gml:boundedBy><gml:null>missing</gml:null></gml:boundedBy>\n");
    else return; /* No Null boundedBy in WFS 1.1.0 SF-0 */

  } else {
    fprintf(o->output, "<gml:boundedBy>\n");

    if (wr->format == WFS_GML212) {
      fprintf(o->output, "  <gml:Box srsName=\"");
      if (strcmp(srs->auth_name->buf, "EPSG")) fprintf(o->output, "%s:", srs->auth_name->buf);
      else if (srs->is_long) fprintf(o->output, "urn:ogc:def:crs:EPSG::");
      else fprintf(o->output, "EPSG:");
      fprintf(o->output, "%d\">", srs->srid);

      if (fabs(xmin) > OWS_MAX_DOUBLE || fabs(ymin) > OWS_MAX_DOUBLE ||
          fabs(xmax) > OWS_MAX_DOUBLE || fabs(ymax) > OWS_MAX_DOUBLE)
        fprintf(o->output, "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">%g,%g %g,%g</gml:coordinates>",
                xmin, ymin, xmax, ymax);
      else fprintf(o->output, "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">%.*f,%.*f %.*f,%.*f</gml:coordinates>",
                     precision, xmin, precision, ymin, precision, xmax, precision, ymax);
      fprintf(o->output, "</gml:Box>\n");

    } else if (wr->format == WFS_GML311) {
      fprintf(o->output, "  <gml:Envelope srsName=\"");
      if (strcmp(srs->auth_name->buf, "EPSG")) fprintf(o->output, "%s:", srs->auth_name->buf);
      else if (srs->is_long) fprintf(o->output, "urn:ogc:def:crs:EPSG::");
      else fprintf(o->output, "EPSG:");
      fprintf(o->output, "%d\">", srs->srid);

      if (srs->is_reverse_axis && !srs->is_eastern_axis && srs->is_long) {
        if (fabs(xmin) > OWS_MAX_DOUBLE || fabs(ymin) > OWS_MAX_DOUBLE ||
            fabs(xmax) > OWS_MAX_DOUBLE || fabs(ymax) > OWS_MAX_DOUBLE) {
          fprintf(o->output, "<gml:lowerCorner>%g %g</gml:lowerCorner>", ymin, xmin);
          fprintf(o->output, "<gml:upperCorner>%g %g</gml:upperCorner>", ymax, xmax);
        } else {
          fprintf(o->output, "<gml:lowerCorner>%.*f %.*f</gml:lowerCorner>", precision, ymin, precision, xmin);
          fprintf(o->output, "<gml:upperCorner>%.*f %.*f</gml:upperCorner>", precision, ymax, precision, xmax);
        }
      } else {
        if (fabs(xmin) > OWS_MAX_DOUBLE || fabs(ymin) > OWS_MAX_DOUBLE ||
            fabs(xmax) > OWS_MAX_DOUBLE || fabs(ymax) > OWS_MAX_DOUBLE) {
          fprintf(o->output, "<gml:lowerCorner>%g %g</gml:lowerCorner>", xmin, ymin);
          fprintf(o->output, "<gml:upperCorner>%g %g</gml:upperCorner>", xmax, ymax);
        } else {
          fprintf(o->output, "<gml:lowerCorner>%.*f %.*f</gml:lowerCorner>", precision, xmin, precision, ymin);
          fprintf(o->output, "<gml:upperCorner>%.*f %.*f</gml:upperCorner>", precision, xmax, precision, ymax);
        }
      }
      fprintf(o->output, "</gml:Envelope>\n");
    }

    fprintf(o->output, "</gml:boundedBy>\n");
  }
}


/*
 * Display the properties of one feature
 */
void wfs_gml_display_feature(ows * o, wfs_request * wr,
                             buffer * layer_name, buffer * prefix,
                             char * prop_name, buffer * prop_type, char * value)
{
  buffer *time, *pkey, *value_encoded, *prop_name_buffer;
  bool gml_ns = false;
  assert(layer_name && prop_name);
  assert(prop_type);
  assert(prefix);
  assert(value);
  assert(layer_name && prop_name && prop_type && prefix && value && wr && o);

  pkey = ows_psql_id_column(o, layer_name); /* CAUTION: pkey could be NULL ! */

  /* No Pkey display in GML (default behaviour) */
  if (pkey && pkey->buf && !strcmp(prop_name, pkey->buf) && !o->expose_pk) return;

  /* Avoid to expose elements from gml_exclude_items */
  prop_name_buffer = buffer_from_str(prop_name);

  if ((ows_layer_get(o->layers, layer_name))->exclude_items
       && in_list(ows_layer_get(o->layers, layer_name)->exclude_items, prop_name_buffer)) return;

  buffer_free(prop_name_buffer);

  if (strlen(value) == 0) return; /* Don't display empty property */

  /* We have to check if we use gml ns or not */
  if ((ows_layer_get(o->layers, layer_name))->gml_ns
      && (in_list_str((ows_layer_get(o->layers, layer_name))->gml_ns, prop_name))) gml_ns = true;

  if (gml_ns) fprintf(o->output, "   <gml:%s>", prop_name);
  else        fprintf(o->output, "   <%s:%s>", prefix->buf, prop_name);


  /* PSQL date must be transformed into GML format */
  if (    buffer_cmp(prop_type, "timestamptz")
       || buffer_cmp(prop_type, "timestamp")
       || buffer_cmp(prop_type, "datetime")
       || buffer_cmp(prop_type, "date")) {
    time = ows_psql_timestamp_to_xml_time(value);
    fprintf(o->output, "%s", time->buf);
    buffer_free(time);

    /* PSQL boolean must be transformed into GML format */
  } else if (buffer_cmp(prop_type, "bool")) {
    if (!strcmp(value, "t")) fprintf(o->output, "true");
    if (!strcmp(value, "f")) fprintf(o->output, "false");

  } else if (    buffer_cmp(prop_type, "text")
              || buffer_cmp(prop_type, "hstore")
              || buffer_ncmp(prop_type, "char", 4)
              || buffer_ncmp(prop_type, "varchar", 7)) {

    value_encoded = buffer_encode_xml_entities_str(value);
    fprintf(o->output, "%s", value_encoded->buf);
    buffer_free(value_encoded);

  } else fprintf(o->output, "%s", value);

  if (gml_ns) fprintf(o->output, "</gml:%s>\n", prop_name);
  else        fprintf(o->output, "</%s:%s>\n", prefix->buf, prop_name);
}


/*
 * Display in GML all feature members returned by the request
 */
void wfs_gml_feature_member(ows * o, wfs_request * wr, buffer * layer_name, list * properties, PGresult * res)
{
  int i, j, number, end, nb_fields;
  buffer *id_name, *ns_prefix, *prop_type, *layer;
  array * describe;
  assert(o && wr && res && layer_name);

  /* CAUTION: Properties could be NULL ! */

  number = -1;
  id_name = ows_psql_id_column(o, layer_name);

  /* CAUTION: We could imagine layer without PK ! */
  if (id_name && id_name->use) number = PQfnumber(res, id_name->buf);

  ns_prefix = ows_layer_ns_prefix(o->layers, ows_layer_uri_to_prefix(o->layers, layer_name));
  describe = ows_psql_describe_table(o, layer_name);

  /* display the results in gml */
  for (i = 0, end = PQntuples(res); i < end; i++) {
    fprintf(o->output, "  <gml:featureMember>\n");

     layer = ows_layer_no_uri(o->layers, layer_name);

    /* print layer's name and id according to GML version */
    if (id_name && id_name->use) {
      if (wr->format == WFS_GML311)
        fprintf(o->output,  "   <%s gml:id=\"%s.%s\">\n",
                ows_layer_uri_to_prefix(o->layers, layer_name)->buf,
                layer->buf, PQgetvalue(res, i, number));
      else fprintf(o->output, "   <%s fid=\"%s.%s\">\n",
                      ows_layer_uri_to_prefix(o->layers, layer_name)->buf,
                      layer->buf, PQgetvalue(res, i, number));
    } else fprintf(o->output,   "   <%s>\n", ows_layer_uri_to_prefix(o->layers, layer_name)->buf);

    /* print properties */
    for (j = 0, nb_fields = PQnfields(res) ; j < nb_fields ; j++) {
      if (    !properties
           || in_list_str(properties, PQfname(res, j))
           || buffer_cmp(properties->first->value, "*")
           || (ows_psql_not_null_properties(o, layer_name)
              && in_list_str(ows_psql_not_null_properties(o, layer_name), PQfname(res, j)))
           || ((ows_layer_get(o->layers, layer_name))->gml_ns
              && in_list_str((ows_layer_get(o->layers, layer_name))->gml_ns, PQfname(res, j)))) {
        prop_type = array_get(describe, PQfname(res, j));
        wfs_gml_display_feature(o, wr, layer_name, ns_prefix, PQfname(res,j), prop_type, PQgetvalue(res, i ,j));
      }
    }

    fprintf(o->output, "   </%s>\n", ows_layer_uri_to_prefix(o->layers, layer_name)->buf);
    fprintf(o->output, "  </gml:featureMember>\n");
  }
}


/*
 * Display in GML the fisrt node and the required namespaces
 */
static void wfs_gml_display_namespaces(ows * o, wfs_request * wr)
{
  array *namespaces;
  array_node *an;
  list_node *ln;
  buffer * ns_prefix;

  assert(o);
  assert(wr);

  namespaces = ows_layer_list_namespaces(o->layers);
  assert(namespaces);

  if (wr->format == WFS_GML212)
    fprintf(o->output, "Content-Type: text/xml; subtype=gml/2.1.2\n\n");
  else if (wr->format == WFS_GML311)
    fprintf(o->output, "Content-Type: text/xml; subtype=gml/3.1.1\n\n");

  fprintf(o->output, "<?xml version='1.0' encoding='%s'?>\n", o->encoding->buf);
  fprintf(o->output, "<wfs:FeatureCollection\n");

  for (an = namespaces->first; an != NULL; an = an->next)
    fprintf(o->output, " xmlns:%s='%s'\n", an->key->buf, an->value->buf);

  fprintf(o->output, " xmlns:wfs='http://www.opengis.net/wfs'\n");
  fprintf(o->output, " xmlns:xsi='http://www.w3.org/2001/XMLSchema-instance'\n");
  fprintf(o->output, " xmlns:gml='http://www.opengis.net/gml'\n");
  fprintf(o->output, " xmlns:xsd='http://www.w3.org/2001/XMLSchema'\n");
  fprintf(o->output, " xmlns:ogc='http://www.opengis.net/ogc'\n");
  fprintf(o->output, " xmlns:xlink='http://www.w3.org/1999/xlink'\n");
  fprintf(o->output, " xmlns:ows='http://www.opengis.net/ows'\n");

  fprintf(o->output, " xsi:schemaLocation='");
  if (ows_version_get(o->request->version) == 100)
    fprintf(o->output, "%s\n    %s?service=WFS&amp;version=1.0.0&amp;request=DescribeFeatureType",
            namespaces->first->value->buf, o->online_resource->buf);
  else
    fprintf(o->output, "%s\n    %s?service=WFS&amp;version=1.1.0&amp;request=DescribeFeatureType",
            namespaces->first->value->buf, o->online_resource->buf);

  /* FeatureId request could be without Typename parameter */
  if (wr->typename) {
    fprintf(o->output, "&amp;Typename=");
    for (ln = wr->typename->first ; ln ; ln = ln->next) {
      ns_prefix = ows_layer_ns_prefix(o->layers, ln->value);
      fprintf(o->output, "%s:%s", ns_prefix->buf, ln->value->buf);
      if (ln->next) fprintf(o->output, ",");
    }
  }

  if (ows_version_get(o->request->version) == 100) {
    fprintf(o->output, "   http://www.opengis.net/wfs\n");
    fprintf(o->output, "   http://schemas.opengis.net/wfs/1.0.0/WFS-basic.xsd\n");
  } else {
    fprintf(o->output, "   http://www.opengis.net/wfs\n");
    fprintf(o->output, "   http://schemas.opengis.net/wfs/1.1.0/wfs.xsd\n");
  }

  if (wr->format == WFS_GML212) {
    fprintf(o->output, "   http://www.opengis.net/gml\n");
    fprintf(o->output, "   http://schemas.opengis.net/gml/2.1.2/feature.xsd'\n");
  } else {
    fprintf(o->output, "   http://www.opengis.net/gml\n");
    fprintf(o->output, "   http://schemas.opengis.net/gml/3.1.1/base/gml.xsd'\n");
  }

  array_free(namespaces);
}


/*
 * Diplay in GML result of a GetFeature hits request
 */
static void wfs_gml_display_hits(ows * o, wfs_request * wr, mlist * request_list)
{
  list_node * ln;
  PGresult *res;
  buffer * date;
  int hits = 0;

  assert(o);
  assert(wr);
  assert(request_list);

  wfs_gml_display_namespaces(o, wr);

  /* Just count the number of features */
  for (ln = request_list->first->value->first ; ln ; ln = ln->next) {
    buffer_add_head_str(ln->value, "SELECT count(*) FROM (");
    buffer_add_str(ln->value, ") AS foo");

    res = ows_psql_exec(o, ln->value->buf);
    if (PQresultStatus(res) == PGRES_TUPLES_OK)
      hits = hits + atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
  }

  /* Render GML hits output */
  res = ows_psql_exec(o, "SELECT localtimestamp");
  date = ows_psql_timestamp_to_xml_time(PQgetvalue(res, 0, 0));
  fprintf(o->output, " timeStamp='%s' numberOfFeatures='%d' />\n", date->buf, hits);
  buffer_free(date);
  PQclear(res);
}

/*
 * Diplay in GML result of a GetFeature request
 */
static void wfs_gml_display_results(ows * o, wfs_request * wr, mlist * request_list)
{
  mlist_node *mln_property, *mln_fid;
  list_node *ln, *ln_typename;
  buffer *layer_name, *layer_uri;
  list *fe;
  PGresult *res;
  ows_bbox *outer_b;

  assert(o && wr && request_list);

  ln = ln_typename = NULL;
  mln_property = mln_fid = NULL;

  /* Display the first node and namespaces */
  wfs_gml_display_namespaces(o, wr);
  fprintf(o->output, ">\n");

  /* Display only if we really asked the bbox of the features retrieved. Overhead could be signifiant ! */
  if (o->display_bbox) {
    outer_b = ows_bbox_boundaries(o, request_list->first->next->value, request_list->last->value, wr->srs);
    wfs_gml_bounded_by(o, wr, outer_b->xmin, outer_b->ymin, outer_b->xmax, outer_b->ymax, outer_b->srs);
    ows_bbox_free(outer_b);
  }

  /* Initialize the nodes to run through requests */
  if (wr->typename)     ln_typename = wr->typename->first;
  if (wr->featureid)    mln_fid = wr->featureid->first;
  if (wr->propertyname) mln_property = wr->propertyname->first;

  for (ln = request_list->first->value->first ; ln ; ln = ln->next) {

    res = ows_psql_exec(o, ln->value->buf);

    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      PQclear(res);

      /* Increments the nodes */
      if (wr->typename)     ln_typename = ln_typename->next;
      if (wr->featureid)    mln_fid = mln_fid->next;
      if (wr->propertyname) mln_property = mln_property->next;

      break;
    }

    /* Define a layer_name which match typename or featureid */

    if (wr->typename) {
      layer_name = buffer_init();
      buffer_copy(layer_name, ln_typename->value);
      layer_uri = ows_layer_prefix_to_uri(o->layers, layer_name);
      buffer_free(layer_name);
    } else {
      fe = list_explode('.', mln_fid->value->first->value);
      layer_uri = ows_layer_no_uri_to_uri(o->layers, fe->first->value); 
      list_free(fe);
    }

    /* Display each feature member */
    if (wr->propertyname) wfs_gml_feature_member(o, wr, layer_uri, mln_property->value, res);
    else                  wfs_gml_feature_member(o, wr, layer_uri, NULL, res);   /* PropertyNames not mandatory */

    PQclear(res);

    /* Increments the nodes */
    if (wr->featureid)    mln_fid = mln_fid->next;
    if (wr->propertyname) mln_property = mln_property->next;
    if (wr->typename)     ln_typename = ln_typename->next;
  }

  fprintf(o->output, "</wfs:FeatureCollection>\n");
}


/*
 * Transform part of GetFeature request into SELECT statement of a SQL request
 */
static buffer *wfs_retrieve_sql_request_select(ows * o, wfs_request * wr, buffer * layer_name)
{
  int gml_opt;
  buffer *select;
  array *prop_table;
  array_node *an;
  bool gml_boundedby;

  assert(o && wr);

  select = buffer_init();
  buffer_add_str(select, "SELECT ");

  prop_table = ows_psql_describe_table(o, layer_name);

  for (an = prop_table->first ; an ; an = an->next) {

    if (!strcmp(an->key->buf, "boundedBy")
            && (ows_layer_get(o->layers, layer_name))->gml_ns
            && (in_list_str((ows_layer_get(o->layers, layer_name))->gml_ns, an->key->buf))) gml_boundedby = true;
    else gml_boundedby = false;

    /* geometry columns must be returned in GML */
    if (ows_psql_is_geometry_column(o, layer_name, an->key)) {

      if (wr->format == WFS_GML212) {
        buffer_add_str(select, "ST_AsGML(");

        /* Geometry Reprojection on the fly step if asked */
        if (wr->srs) {
          buffer_add_str(select, "ST_Transform(");
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\"::geometry,");
          buffer_add_int(select, wr->srs->srid);
          buffer_add_str(select, "),");
        } else {
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\",");
        }

        if ((wr->srs && !wr->srs->is_degree) ||
            (!wr->srs && ows_srs_meter_units(o, layer_name)))
          buffer_add_int(select, o->meter_precision);
        else
          buffer_add_int(select, o->degree_precision);

                                                 gml_opt  = 0; /* Short SRS */
        if (wr->srs && wr->srs->is_long)         gml_opt += 1; /* FIXME really ? */
        if (wr->srs && wr->srs->is_eastern_axis) gml_opt += 16;
        if (gml_boundedby)                       gml_opt += 32;

        buffer_add_str(select, ",");
        buffer_add_int(select, gml_opt);
        buffer_add_str(select, ") AS \"");
        buffer_copy(select, an->key);
        buffer_add_str(select, "\" ");
      }
      /* GML3 */
      else if (wr->format == WFS_GML311) {
        buffer_add_str(select, "ST_AsGML(3, ");

        /* Geometry Reprojection on the fly step if asked */
        if (wr->srs) {
          buffer_add_str(select, "ST_Transform(");
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\"::geometry,");
          buffer_add_int(select, wr->srs->srid);
          buffer_add_str(select, "),");
        } else {
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\",");
        }

        gml_opt = 6; /* no srsDimension (CITE Compliant) and use LineString rather than curve */
        if (wr->srs && wr->srs->is_long) gml_opt += 1; /* Long SRS */
        if (gml_boundedby) gml_opt += 32;

        if ((wr->srs && !wr->srs->is_degree) || (!wr->srs && ows_srs_meter_units(o, layer_name))) {
          buffer_add_int(select, o->meter_precision);
          if (wr->srs && !wr->srs->is_eastern_axis && wr->srs->is_long) gml_opt += 16;
        } else {
          buffer_add_int(select, o->degree_precision);
          if (wr->srs && !wr->srs->is_eastern_axis && wr->srs->is_long) gml_opt += 16;
        }

        buffer_add_str(select, ", ");
        buffer_add_int(select, gml_opt);
        buffer_add_str(select, ") AS \"");
        buffer_copy(select, an->key);
        buffer_add_str(select, "\" ");
      } else if (wr->format == WFS_GEOJSON) {
        buffer_add_str(select, "ST_AsGeoJSON(");

        /* Geometry Reprojection on the fly step if asked */
        if (wr->srs) {
          buffer_add_str(select, "ST_Transform(");
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\"::geometry,");
          buffer_add_int(select, wr->srs->srid);
          buffer_add_str(select, "),");
        } else {
          buffer_add(select, '"');
          buffer_copy(select, an->key);
          buffer_add_str(select, "\",");
        }

        if ((wr->srs && !wr->srs->is_degree) ||
            (!wr->srs && ows_srs_meter_units(o, layer_name)))
          buffer_add_int(select, o->meter_precision);
        else
          buffer_add_int(select, o->degree_precision);

        buffer_add_str(select, ", 1) AS \""); /* Bbox */

        buffer_copy(select, an->key);
        buffer_add_str(select, "\" ");
      }

    }
    /* Columns are written in quotation marks */
    else {
      buffer_add_str(select, "\"");
      buffer_copy(select, an->key);
      buffer_add_str(select, "\"");
    }

    if (an->next) buffer_add_str(select, ",");
  }

  return select;
}


/*
 * Retrieve a list of SQL requests from the GetFeature parameters
 */
static mlist *wfs_retrieve_sql_request_list(ows * o, wfs_request * wr)
{
  mlist *requests;
  mlist_node *mln_fid;
  list *fid, *sql_req, *from_list, *where_list;
  list_node *ln_typename, *ln_filter;
  buffer *geom, *sql, *where, *layer_name, *layer_uri, *sql_count;
  int srid, size, cpt, features, max_features;
  filter_encoding *fe;
  ows_bbox *bbox;
  char *escaped;
  PGresult * res;

  assert(o && wr);

  mln_fid = NULL;
  ln_typename = ln_filter = NULL;
  where = geom = NULL;
  size = features = 0;

  /* Initialize the nodes to run through typename and fid */
  if (wr->typename) {
    size = wr->typename->size;
    ln_typename = wr->typename->first;
  }

  if (wr->filter) ln_filter = wr->filter->first;

  if (wr->featureid) {
    size = wr->featureid->size;
    mln_fid = wr->featureid->first;
  }

  /* Will contain list of SQL requests */
  sql_req = list_init();
  /* The 'from_list' and 'where_list' will contain layer names
     and SQL WHERE statement used later to compute the boundaries */
  from_list = list_init();
  where_list = list_init();

  /* Fill a SQL request for each typename */
  for (cpt = 0; cpt < size; cpt++) {
    layer_name = buffer_init();

    /* Defines a layer_name which match typename or featureid */
    if (wr->typename) {
      buffer_copy(layer_name, ln_typename->value);
      layer_uri = ows_layer_prefix_to_uri(o->layers, layer_name); 
    } else {
      fid = list_explode('.', mln_fid->value->first->value);
      layer_uri = ows_layer_no_uri_to_uri(o->layers, fid->first->value);
      buffer_copy(layer_name, ows_layer_uri_to_prefix(o->layers, layer_uri));
      list_free(fid);
    }


    /* SELECT */
    sql = wfs_retrieve_sql_request_select(o, wr, layer_uri);

    /* FROM : match layer_name (typename or featureid) */
    buffer_add_str(sql, " FROM \"");
    buffer_copy(sql, ows_psql_schema_name(o, layer_uri));
    buffer_add_str(sql, "\".\"");
    buffer_copy(sql, ows_psql_table_name(o, layer_uri));
    buffer_add_str(sql, "\"");

    /* WHERE : match featureid, bbox or filter */

    /* FeatureId */
    if (wr->featureid) {
      where = fe_kvp_featureid(o, wr, layer_uri, mln_fid->value);

      if (where->use == 0) {
        buffer_free(where);
        buffer_free(sql);
        buffer_free(layer_name);
        list_free(sql_req);
        list_free(from_list);
        list_free(where_list);
        wfs_error(o, wr, WFS_ERROR_NO_MATCHING, "error : an id_column is required to use featureid", "GetFeature");
        return NULL;
      }
    }

    /* BBOX */
    else if (wr->bbox) where = fe_kvp_bbox(o, wr, layer_uri, wr->bbox);

    /* Filter */
    else if (wr->filter) {
      if (ln_filter->value->use != 0) {
        where = buffer_init();
        buffer_add_str(where, " WHERE ");

        fe = filter_encoding_init();
        fe = fe_filter(o, fe, layer_name, ln_filter->value);

        if (fe->error_code != FE_NO_ERROR) {
          buffer_free(where);
          buffer_free(sql);
          buffer_free(layer_name);
          list_free(sql_req);
          list_free(from_list);
          list_free(where_list);
          fe_error(o, fe);
          return NULL;
        }

        buffer_copy(where, fe->sql);
        filter_encoding_free(fe);
      }
    } else where = buffer_init();

    buffer_free(layer_name);

    if (o->max_geobbox && where->use != 0) buffer_add_str(where, " AND ");
    else if (o->max_geobbox && where->use == 0) buffer_add_str(where, " WHERE ");

    /* geobbox's limits of ows */
    if (o->max_geobbox) {

      bbox = ows_bbox_init();
      ows_bbox_set_from_geobbox(o, bbox, o->max_geobbox);

      buffer_add_str(where, "NOT (ST_Disjoint(");
      buffer_copy(where, geom);
      buffer_add_str(where, ",ST_Transform(");
      ows_bbox_to_query(o, bbox, where);
      buffer_add_str(where, ",");
      srid = ows_srs_get_srid_from_layer(o, layer_uri);
      buffer_add_int(where, srid);
      buffer_add_str(where, ")))");

      ows_bbox_free(bbox);
    }

    /* sortby parameter */
    if (wr->sortby) {
      buffer_add_str(where, " ORDER BY ");
      escaped = ows_psql_escape_string(o, wr->sortby->buf);
      if (escaped) {
        buffer_add_str(where, escaped);
        free(escaped);
      }
    }

    /* maxfeatures parameter, or max_features ows limits, limits the number of results */
    max_features = -1;
    if (wr->maxfeatures > 0 && o->max_features > 0 && wr->maxfeatures > o->max_features)
      max_features = o->max_features;
    else if (wr->maxfeatures > 0)
      max_features = wr->maxfeatures;
    else if (o->max_features > 0)
      max_features = o->max_features;

    if (max_features > 0 && wr->typename->size == 1) {
      buffer_add_str(where, " LIMIT ");
      buffer_add_int(where, max_features);
    } else if (max_features > 0 && wr->typename->size > 1) {
      /* We have to compute LIMIT for each layer in this case ! */
      sql_count = buffer_init();
      buffer_add_str(sql_count, "SELECT count(*) FROM (");
      buffer_copy(sql_count, sql);
      buffer_copy(sql_count, where);
      buffer_add_str(sql_count, " LIMIT ");
      buffer_add_int(sql_count, max_features - features );
      buffer_add_str(sql_count, ") AS c");
      res = ows_psql_exec(o, sql_count->buf);
      if (PQresultStatus(res) == PGRES_TUPLES_OK) {
        if (features + atoi(PQgetvalue(res, 0, 0)) <= max_features) {
          buffer_add_str(where, " LIMIT ");
          if ((max_features - features) > 0)
            buffer_add_int(where, max_features - features);
          else buffer_add_int(where, 0);
          features += atoi(PQgetvalue(res, 0, 0));
        }
      }
      PQclear(res);
      buffer_free(sql_count);
    }

    buffer_copy(sql, where);

    list_add(sql_req, sql);
    list_add(where_list, where);
    list_add_by_copy(from_list, layer_uri);

    /* incrementation of the nodes */
    if (wr->featureid) mln_fid = mln_fid->next;
    if (wr->typename)  ln_typename = ln_typename->next;
    if (wr->filter)    ln_filter = ln_filter->next;
  }

  /* requests multiple list contains three lists : sql requests, from list and where list */
  requests = mlist_init();
  mlist_add(requests, sql_req);
  mlist_add(requests, from_list);
  mlist_add(requests, where_list);

  return requests;
}


/*
 * Diplay in GeoJSON result of a GetFeature request
 */
static void wfs_geojson_display_results(ows * o, wfs_request * wr, mlist * request_list)
{
  PGresult *res;
  list_node *ln, *ll;
  array *prop_table;
  array_node *an;
  buffer *prop, *value_enc, *geom, *id_name;
  bool first_row, first_col;
  int i,j;
  int geoms;
  int number;

  assert(o);
  assert(wr);
  assert(request_list);

  ln = ll= NULL;

  ll = request_list->first->next->value->first;
  geom = buffer_init();
  prop = buffer_init();
  id_name = buffer_init();

  fprintf(o->output, "Content-Type: application/json\n\n");
  fprintf(o->output, "{\"type\": \"FeatureCollection\", \"crs\":{\"type\":\"name\",\"properties\":{\"name\":\"");
  if (ows_version_get(o->request->version) == 100)
    fprintf(o->output, "EPSG:%i", wr->srs->srid);
  else
    fprintf(o->output, "urn:ogc:def:crs:EPSG::%i", wr->srs->srid);

  fprintf(o->output, "\"}}, \"features\": [");

  for (ln = request_list->first->value->first ; ln ; ln = ln->next) {

    res = ows_psql_exec(o, ln->value->buf);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
      PQclear(res);
      ll = ll->next;
      break;
    }

    prop_table = ows_psql_describe_table(o, ll->value);
    first_row = true;
    id_name = ows_psql_id_column(o, ll->value);
    number = -1;
    if (id_name && id_name->use)
         number = PQfnumber(res, id_name->buf);
    buffer_empty(id_name);

    for (i=0 ; i < PQntuples(res) ; i++) {

      first_col = true;
      geoms = 0;

      if (first_row) first_row = false;
      else fprintf(o->output, ",");

      if ( number >= 0 ) {
        buffer_add_str(id_name, "\"id\": \"");
        buffer_copy(id_name, ll->value);
        buffer_add_str(id_name, ".");
        buffer_add_str(id_name, PQgetvalue(res, i, number));
        buffer_add_str(id_name, "\", ");
      }
      for (an = prop_table->first, j=0 ; an ; an = an->next, j++) {
        
        if (ows_psql_is_geometry_column(o, ll->value, an->key)) {
          buffer_add_str(geom, PQgetvalue(res, i, j));
          geoms++;
        } else {

          if (first_col)  first_col = false;
          else buffer_add_str(prop, ", \"");

          buffer_copy(prop, an->key);
          buffer_add_str(prop, "\": \"");
          value_enc = buffer_encode_json_str(PQgetvalue(res, i, j));
          buffer_copy(prop, value_enc);
          buffer_free(value_enc);
          buffer_add(prop, '"');
        }
      }

      if (geoms == 0) {
        fprintf(o->output,
                "{\"type\":\"Feature\", %s\"properties\":{\"%s}}\n",
                id_name->buf, prop->buf);
      } else if (geoms == 1) {
        fprintf(o->output,
                "{\"type\":\"Feature\", %s\"properties\":{\"%s}, \"geometry\":%s}\n",
                id_name->buf, prop->buf, geom->buf);
      } else if (geoms > 1) {
        fprintf(o->output,
                "{\"type\":\"Feature\", %s\"properties\":{\"%s}, \"geometry\":%s%s]}}\n",
                id_name->buf,
                prop->buf, "{ \"type\": \"GeometryCollection\", \"geometries\": [",
                geom->buf);
      }
      buffer_empty(prop);
      buffer_empty(geom);
      buffer_empty(id_name);
    }

    PQclear(res);
    ll = ll->next;
  }

  fprintf(o->output, "]}");

  buffer_free(geom);
  buffer_free(prop);
}


/*
 * Execute the GetFeature request
 */
void wfs_get_feature(ows * o, wfs_request * wr)
{
  mlist *request_list;
  assert(o && wr);

  /* Retrieve a list of SQL requests from the GetFeature parameters */
  request_list = wfs_retrieve_sql_request_list(o, wr);
  if (!request_list) return;

  if (wr->format == WFS_GML212 || wr->format == WFS_GML311) {
    /* Display result of the GetFeature request in GML */
    if (buffer_cmp(wr->resulttype, "hits"))
      wfs_gml_display_hits(o, wr, request_list);
    else
      wfs_gml_display_results(o, wr, request_list);

  } else if (wr->format == WFS_GEOJSON)
    wfs_geojson_display_results(o, wr, request_list);

  /* Add here other functions to display GetFeature response in other formats */

  mlist_free(request_list);
}
