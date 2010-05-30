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
#include <float.h>
#include <math.h>
#include <string.h>

#include "../ows/ows.h"



/*
 * Return the boundaries of the features returned by the request
 */
void wfs_gml_bounded_by(ows * o, wfs_request * wr, float xmin, float ymin,
                        float xmax, float ymax, int srid)
{
    assert(o != NULL);
    assert(wr != NULL);

    fprintf(o->output, "<gml:boundedBy>\n");

    if (fabs(xmin) + DBL_MIN <= 1 + DBL_EPSILON &&
	fabs(ymin) + DBL_MIN <= 1 + DBL_EPSILON) {
        if (ows_version_get(o->request->version) == 100)
            fprintf(o->output, "<gml:null>missing</gml:null>\n");
        else
            fprintf(o->output, "<gml:Null>missing</gml:Null>\n");
    } else {

        if (wr->format == WFS_GML2) {
            fprintf(o->output, "<gml:Box srsName=\"EPSG:%d\">", srid);
            fprintf(o->output, "<gml:coordinates decimal=\".\" cs=\",\" ts=\" \">");
            fprintf(o->output, "%f,", xmin);
            fprintf(o->output, "%f ", ymin);
            fprintf(o->output, "%f,", xmax);
            fprintf(o->output, "%f</gml:coordinates>", ymax);
            fprintf(o->output, "</gml:Box>\n");

        } else if (wr->format == WFS_GML3) {
            fprintf(o->output, "<gml:Envelope srsName=\"urn:ogc:def:crs:EPSG::%d\">", srid);
            fprintf(o->output, "<gml:lowerCorner>");
            fprintf(o->output, "%f ", xmin);
            fprintf(o->output, "%f", ymin);
            fprintf(o->output, "</gml:lowerCorner>");
            fprintf(o->output, "<gml:upperCorner>");
            fprintf(o->output, "%f ", xmax);
            fprintf(o->output, "%f</gml:upperCorner>", ymax);
            fprintf(o->output, "</gml:Envelope>\n");
        }
    }

    fprintf(o->output, "</gml:boundedBy>\n");
}


/*
 * Display the properties of one feature
 */
void wfs_gml_display_feature(ows * o, wfs_request * wr,
                             buffer * layer_name, buffer * prefix, buffer * prop_name,
                             buffer * prop_type, buffer * value)
{
    buffer *time, *pkey, *value_encoded;
    bool gml_ns = false;

    assert(o != NULL);
    assert(wr != NULL);
    assert(layer_name != NULL);
    assert(prefix != NULL);
    assert(prop_name != NULL);
    assert(prop_type != NULL);
    assert(value != NULL);

    pkey = ows_psql_id_column(o, layer_name); /* pkey could be NULL !!! */

    /* No Pkey display in GML */
    if (buffer_cmp(value, "") || (pkey != NULL && buffer_cmp(prop_name, pkey->buf))) 
        return;

    /* Don't handle boundedBy column (CITE 1.0 Unit test)) */
    if (buffer_cmp(prop_name, "boundedBy")) return;

    /* name and description fields if exists belong to GML namespace */
    if (buffer_cmp(prop_name, "name")
            || buffer_cmp(prop_name, "description"))
        gml_ns = true;

#if 0
    /* Used in CITE 1.0 Unit test as geometry column name 
       TODO: what about a more generic way to handle that ? */
    if (wr->format == WFS_GML2
            && (buffer_cmp(prop_name, "pointProperty")
            || buffer_cmp(prop_name, "multiPointProperty")
            || buffer_cmp(prop_name, "lineStringProperty")
            || buffer_cmp(prop_name, "multiLineStringProperty")
            || buffer_cmp(prop_name, "polygonProperty")
            || buffer_cmp(prop_name, "multiPolygonProperty")))
        gml_ns = true;
#endif

    if (gml_ns == true)
        fprintf(o->output, "   <gml:%s>", prop_name->buf);
    else 
        fprintf(o->output, "   <%s:%s>", prefix->buf, prop_name->buf);

    if (buffer_cmp(prop_type, "timestamptz")
            || buffer_cmp(prop_type, "timestamp")
            || buffer_cmp(prop_type, "datetime")
            || buffer_cmp(prop_type, "date")) {
        /* PSQL date must be transformed into GML format */
        time = ows_psql_timestamp_to_xml_time(value->buf);
        fprintf(o->output, "%s", time->buf);
        buffer_free(time);
    } else if (buffer_cmp(prop_type, "bool")) {
        /* PSQL boolean must be transformed into GML format */
        if (buffer_cmp(value, "t"))
            fprintf(o->output, "true");

        if (buffer_cmp(value, "f"))
            fprintf(o->output, "false");

    } else if (buffer_cmp(prop_type, "text")
            || buffer_ncmp(prop_type, "char", 4)
            || buffer_ncmp(prop_type, "varchar", 7)) {
	    value_encoded = buffer_encode_xml_entities(value);
        fprintf(o->output, "%s", value_encoded->buf);
        buffer_free(value_encoded);

    } else fprintf(o->output, "%s", value->buf);
    
    if (gml_ns)
        fprintf(o->output, "</gml:%s>\n", prop_name->buf);
    else
        fprintf(o->output, "</%s:%s>\n", prefix->buf, prop_name->buf);
}


/*
 * Display in GML all feature members returned by the request
 */
void wfs_gml_feature_member(ows * o, wfs_request * wr, buffer * layer_name,
                            list * properties, PGresult * res)
{
    int i, j, number, end, nb_fields;
    buffer *id_name, *prefix, *prop_name, *prop_type, *value;
    array * describe;
    list *mandatory_prop;

    assert(o != NULL);
    assert(wr != NULL);
    assert(layer_name != NULL);
    assert(res != NULL);
    /* NOTA: properties could be NULL ! */

    number = -1;
    id_name = ows_psql_id_column(o, layer_name);

    /* We could imagine layer without PK ! */
    if (id_name != NULL && id_name->use > 0)
        number = ows_psql_column_number_id_column(o, layer_name);

    prefix = ows_layer_prefix(o->layers, layer_name);
    mandatory_prop = ows_psql_not_null_properties(o, layer_name);
    describe = ows_psql_describe_table(o, layer_name);

    /* display the results in gml */
    for (i = 0, end = PQntuples(res); i < end; i++) {
        fprintf(o->output, "  <gml:featureMember>\n");

        /* print layer's name and id according to GML version */
        if (id_name != NULL && id_name->use != 0) {
            if (wr->format == WFS_GML3)
                fprintf(o->output, "   <%s:%s gml:id=\"%s.%s\">\n",
                        prefix->buf, layer_name->buf, layer_name->buf,
                        PQgetvalue(res, i, number));
            else
                fprintf(o->output, "   <%s:%s fid=\"%s.%s\">\n",
                        prefix->buf, layer_name->buf, layer_name->buf,
                        PQgetvalue(res, i, number));
        } else
            fprintf(o->output, "   <%s:%s>\n", prefix->buf,
                    layer_name->buf);

        /* print properties */
        for (j = 0, nb_fields = PQnfields(res); j < nb_fields; j++) {
            prop_name = buffer_init();
            value = buffer_init();
            buffer_add_str(prop_name, PQfname(res, j));
            buffer_add_str(value, PQgetvalue(res, i, j));
            prop_type = array_get(describe, prop_name->buf);

            if (properties == NULL || properties->first == NULL)
                wfs_gml_display_feature(o, wr, layer_name, prefix,
                                        prop_name, prop_type, value);
            else if (in_list(properties, prop_name)
                     || in_list(mandatory_prop, prop_name)
                     || buffer_cmp(properties->first->value, "*"))
                wfs_gml_display_feature(o, wr, layer_name, prefix,
                                        prop_name, prop_type, value);

            buffer_free(value);
            buffer_free(prop_name);
        }

        fprintf(o->output, "   </%s:%s>\n", prefix->buf, layer_name->buf);
        fprintf(o->output, "  </gml:featureMember>\n");
    }

    buffer_free(layer_name);
    buffer_free(prefix);
}


/*
 * Display in GML the fisrt node and the required namespaces
 */
static void wfs_gml_display_namespaces(ows * o, wfs_request * wr)
{
    array *namespaces;
    array_node *an;
    list_node *ln;
    buffer * prefix;

    assert(o != NULL);
    assert(wr != NULL);

    namespaces = ows_layer_list_namespaces(o->layers);

    assert(namespaces != NULL);

    fprintf(o->output, "Content-Type: application/xml\n\n");
    fprintf(o->output, "<?xml version='1.0' encoding='UTF-8'?>\n");
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
        fprintf(o->output, "%s\n    %s?service=WFS&amp;version=1.0.0&amp;request=DescribeFeatureType&amp;Typename=",
                namespaces->first->value->buf, o->online_resource->buf);
    else 
        fprintf(o->output, "%s\n    %s?service=WFS&amp;version=1.1.0&amp;request=DescribeFeatureType&amp;Typename=",
                namespaces->first->value->buf, o->online_resource->buf);

    for (ln = wr->typename->first; ln != NULL; ln = ln->next) {
        prefix = ows_layer_prefix(o->layers, ln->value);
        fprintf(o->output, "%s:%s", prefix->buf, ln->value->buf);
        buffer_free(prefix);
        if (ln->next) fprintf(o->output, ",");
    }

    if (ows_version_get(o->request->version) == 100) {
        fprintf(o->output, "   http://www.opengis.net/wfs\n");
        fprintf(o->output, "   http://schemas.opengis.net/wfs/1.0.0/WFS-basic.xsd\n");
    } else {
        fprintf(o->output, "   http://www.opengis.net/wfs\n");
        fprintf(o->output, "   http://schemas.opengis.net/wfs/1.1.0/wfs.xsd\n");
    }

    if (wr->format == WFS_GML2) {
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

    assert(o != NULL);
    assert(wr != NULL);
    assert(request_list != NULL);

    wfs_gml_display_namespaces(o, wr);

    /* just count the number of features */
    for (ln = request_list->first->value->first; ln != NULL; ln = ln->next) {
        res = PQexec(o->pg, ln->value->buf);

        if (PQresultStatus(res) == PGRES_TUPLES_OK)
            hits = hits + PQnfields(res);

        PQclear(res);
    }

    /* render GML hits output */
    res = PQexec(o->pg, "select localtimestamp");
    date = ows_psql_timestamp_to_xml_time(PQgetvalue(res, 0, 0));
    fprintf(o->output, " timeStamp='%s' numberOfFeatures='%d' />\n",
            date->buf, hits);
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
    buffer *layer_name;
    list *fe;
    PGresult *res;
    ows_bbox *outer_b;

    assert(o != NULL);
    assert(wr != NULL);
    assert(request_list != NULL);

    ln = NULL;
    ln_typename = NULL;
    mln_property = NULL;
    mln_fid = NULL;

    /* display the first node and namespaces */
    wfs_gml_display_namespaces(o, wr);

    fprintf(o->output, ">\n");

    /* 
     * Display only if we really asked the bbox of the features retrieved
     * Overhead could be signifiant !!!
     */
    if (o->wfs_display_bbox) {
        /* print the outerboundaries of the requests */
        outer_b = ows_bbox_boundaries(o, request_list->first->next->value,
                                  request_list->last->value);
        wfs_gml_bounded_by(o, wr, outer_b->xmin, outer_b->ymin,
                       outer_b->xmax, outer_b->ymax, outer_b->srs->srid);
        ows_bbox_free(outer_b);
    }

    /* initialize the nodes to run through requests */
    if (wr->typename != NULL)
        ln_typename = wr->typename->first;

    if (wr->featureid != NULL)
        mln_fid = wr->featureid->first;

    if (wr->propertyname != NULL)
        mln_property = wr->propertyname->first;

    for (ln = request_list->first->value->first; ln != NULL; ln = ln->next) {
        /* execute the sql request */
        res = PQexec(o->pg, ln->value->buf);

        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);

            /*increments the nodes */
            if (wr->featureid != NULL)
                mln_fid = mln_fid->next;

            if (wr->propertyname != NULL)
                mln_property = mln_property->next;

            if (wr->typename != NULL)
                ln_typename = ln_typename->next;

            break;
        }

        /* define a layer_name which match typename or featureid */
        layer_name = buffer_init();

        if (wr->typename != NULL)
            buffer_copy(layer_name, ln_typename->value);
        else {
            fe = list_explode('.', mln_fid->value->first->value);
            buffer_copy(layer_name, fe->first->value);
            list_free(fe);
        }

        /* display each feature member */
        if (wr->propertyname != NULL)
            wfs_gml_feature_member(o, wr, layer_name, mln_property->value, res);
        else
            /* Use NULL if propertynames not defined */
            wfs_gml_feature_member(o, wr, layer_name, NULL, res);

        PQclear(res);

        /*increments the nodes */
        if (wr->featureid != NULL)
            mln_fid = mln_fid->next;

        if (wr->propertyname != NULL)
            mln_property = mln_property->next;

        if (wr->typename != NULL)
            ln_typename = ln_typename->next;
    }

    fprintf(o->output, "</wfs:FeatureCollection>\n");
}


/*
 * Transform part of GetFeature request into SELECT statement of a SQL request
 */
static buffer *wfs_retrieve_sql_request_select(ows * o, wfs_request * wr,
        buffer * layer_name)
{
    buffer *select;
    array *prop_table;
    array_node *an;

    assert(o != NULL);
    assert(wr != NULL);

    select = buffer_init();
    buffer_add_str(select, "SELECT ");

    prop_table = ows_psql_describe_table(o, layer_name);

    for (an = prop_table->first; an != NULL; an = an->next) {
        /* geometry columns must be returned in GML */
        if (ows_psql_is_geometry_column(o, layer_name, an->key)) {

            if (wr->format == WFS_GML2) {
                buffer_add_str(select, "ST_AsGml(2, ");

                /* Geometry Reprojection on the fly step if asked */
                if (wr->srs != NULL) {
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

                if ((wr->srs != NULL && !wr->srs->is_degree) || 
                        (wr->srs == NULL && ows_srs_meter_units(o, layer_name)))
                    buffer_add_int(select, o->meter_precision); 
                else 
                    buffer_add_int(select, o->degree_precision);

                buffer_add_str(select, ") AS \"");
                buffer_copy(select, an->key);
                buffer_add_str(select, "\" ");
            }
            /* GML3 */
            else if (wr->format == WFS_GML3) {
                buffer_add_str(select, "ST_AsGml(3, ");

                /* Geometry Reprojection on the fly step if asked */
                if (wr->srs != NULL) {
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

                if ((wr->srs != NULL && !wr->srs->is_degree) || 
                        (wr->srs == NULL && ows_srs_meter_units(o, layer_name))) {
                    buffer_add_int(select, o->meter_precision);
                    buffer_add_str(select, ", 3) AS \"");
                } else {
                    buffer_add_int(select, o->degree_precision);
                    buffer_add_str(select, ", 19) AS \"");
                }

                buffer_copy(select, an->key);
                buffer_add_str(select, "\" ");
            } 
            else if (wr->format == WFS_GEOJSON) {
                buffer_add_str(select, "ST_AsGeoJSON(");

                /* Geometry Reprojection on the fly step if asked */
                if (wr->srs != NULL) {
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

                if ((wr->srs != NULL && !wr->srs->is_degree) || 
                        (wr->srs == NULL && ows_srs_meter_units(o, layer_name)))
                    buffer_add_int(select, o->meter_precision); 
                else 
                    buffer_add_int(select, o->degree_precision);

                buffer_add_str(select, ", 1) AS \"");
                buffer_copy(select, an->key);
                buffer_add_str(select, "\" ");
            }

        }
        /* columns are written in quotation marks */
        else {
            buffer_add_str(select, "\"");
            buffer_copy(select, an->key);
            buffer_add_str(select, "\"");
        }

        if (an->next != NULL)
            buffer_add_str(select, ",");
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
    buffer *geom, *sql, *where, *layer_name;
    int srid, size, cpt, nb;
    filter_encoding *fe;
    ows_bbox *bbox;

    assert(o != NULL);
    assert(wr != NULL);

    mln_fid = NULL;
    ln_filter = NULL;
    ln_typename = NULL;
    where = NULL;
    geom = NULL;
    size = 0;
    nb = 0;

    /* initialize the nodes to run through typename and fid */
    if (wr->typename != NULL) {
        size = wr->typename->size;
        ln_typename = wr->typename->first;
    }

    if (wr->filter != NULL)
        ln_filter = wr->filter->first;

    if (wr->featureid != NULL) {
        size = wr->featureid->size;
        mln_fid = wr->featureid->first;
    }

    /* sql_req will contain list of sql requests */
    sql_req = list_init();
    /* from_list and where_list will contain layer names and
       SQL WHERE statement to calculate then the boundaries */
    from_list = list_init();
    where_list = list_init();

    /* fill a SQL request for each typename */
    for (cpt = 0; cpt < size; cpt++) {
        layer_name = buffer_init();

        /* defines a layer_name which match typename or featureid */
        if (wr->typename != NULL)
            buffer_copy(layer_name, ln_typename->value);
        else {
            fid = list_explode('.', mln_fid->value->first->value);
            buffer_copy(layer_name, fid->first->value);
            list_free(fid);
        }

        /* SELECT */
        sql = wfs_retrieve_sql_request_select(o, wr, layer_name);

        /* FROM : match layer_name (typename or featureid) */
        buffer_add_str(sql, " FROM ");
        buffer_copy(sql, ows_psql_schema_name(o, layer_name));
        buffer_add_str(sql, ".\"");
        buffer_copy(sql, layer_name);
        buffer_add_str(sql, "\"");

        /* WHERE : match featureid, bbox or filter */

        /* FeatureId */
        if (wr->featureid != NULL) {
            where = fe_kvp_featureid(o, wr, layer_name, mln_fid->value);

            if (where->use == 0) {
                buffer_free(where);
                buffer_free(sql);
                list_free(sql_req);
                list_free(from_list);
                list_free(where_list);
                buffer_free(layer_name);
                wfs_error(o, wr, WFS_ERROR_NO_MATCHING,
                          "error : an id_column is required to use featureid",
                          "GetFeature");
            }
        }
        /* BBOX */
        else if (wr->bbox != NULL)
            where = fe_kvp_bbox(o, wr, layer_name, wr->bbox);
        /* Filter */
        else if (wr->filter != NULL) {
            if (ln_filter->value->use != 0) {
                where = buffer_init();
                buffer_add_str(where, " WHERE ");

                fe = filter_encoding_init();
                fe = fe_filter(o, fe, layer_name, ln_filter->value);

                if (fe->error_code != FE_NO_ERROR) {
                    buffer_free(where);
                    buffer_free(sql);
                    list_free(sql_req);
                    list_free(from_list);
                    list_free(where_list);
                    buffer_free(layer_name);
                    fe_error(o, fe);
                }

                buffer_copy(where, fe->sql);
                filter_encoding_free(fe);
            }
        } else
            where = buffer_init();

        if (o->max_geobbox != NULL && where->use != 0)
            buffer_add_str(where, " AND ");
        else if (o->max_geobbox != NULL && where->use == 0)
            buffer_add_str(where, " WHERE ");

        /* geobbox's limits of ows */
        if (o->max_geobbox != NULL) {

            bbox = ows_bbox_init();
	    ows_bbox_set_from_geobbox(o, bbox, o->max_geobbox);

            buffer_add_str(where, "not(disjoint(");
            buffer_copy(where, geom);
            buffer_add_str(where, ",ST_Transform(");
            ows_bbox_to_query(o, bbox, where);
            buffer_add_str(where, ",");
            srid = ows_srs_get_srid_from_layer(o, layer_name);
            buffer_add_int(where, srid);
            buffer_add_str(where, ")))");

            ows_bbox_free(bbox);
        }

        /* sortby parameter */
        if (wr->sortby != NULL) {
            buffer_add_str(where, " ORDER BY ");
            buffer_copy(where, wr->sortby);
        }

        /* maxfeatures parameter, or max_features ows'limits, limits the
           number of results */
        if (wr->maxfeatures > 0) {
            buffer_add_str(where, " LIMIT ");
            nb = ows_psql_number_features(o, from_list, where_list);
            buffer_add_int(where, wr->maxfeatures - nb);
        } else if (o->max_features > 0) {
            buffer_add_str(where, " LIMIT ");
            buffer_add_int(where, o->max_features);
        }

        buffer_copy(sql, where);

        list_add(sql_req, sql);
        list_add(where_list, where);
        list_add(from_list, layer_name);

        /* incrementation of the nodes */
        if (wr->featureid != NULL)
            mln_fid = mln_fid->next;

        if (wr->typename != NULL)
            ln_typename = ln_typename->next;

        if (wr->filter != NULL)
            ln_filter = ln_filter->next;
    }

    /* requests multiple list contains three lists : sql requests, from
       list and where list */
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
    buffer *prop, *value, *value_enc, *geom;
    bool first;
    int i,j;
    int geoms;

    assert(o != NULL);
    assert(wr != NULL);
    assert(request_list != NULL);

    ln = ll= NULL;

    ll = request_list->first->next->value->first;
    geom = buffer_init();
    prop = buffer_init();
    value = buffer_init();

    fprintf(o->output, "Content-Type: application/json\n\n");
    fprintf(o->output, "{\"type\": \"FeatureCollection\", \"features\": [");

    for (ln = request_list->first->value->first; ln != NULL; ln = ln->next) {
        /* execute the sql request */
        res = PQexec(o->pg, ln->value->buf);


        if (PQresultStatus(res) != PGRES_TUPLES_OK) {
            PQclear(res);
            ll = ll->next;
            break;
        }

        prop_table = ows_psql_describe_table(o, ll->value);

        for (i=0 ; i < PQntuples(res) ; i++) {

            first = true;
            geoms = 0;

            for (an = prop_table->first, j=0 ; an != NULL ; an = an->next, j++) {

                if (ows_psql_is_geometry_column(o, ll->value, an->key)) {
                    buffer_add_str(geom, PQgetvalue(res, i, j));
                    geoms++;
                } else {

                    if (first)  first = false;
                    else buffer_add_str(prop, ", \"");

                    buffer_copy(prop, an->key); 
                    buffer_add_str(prop, "\": \"");
                    buffer_add_str(value, PQgetvalue(res, i, j));
                    value_enc = buffer_encode_json(value);
                    buffer_copy(prop, value_enc);
                    buffer_free(value_enc);
                    buffer_empty(value);
                    buffer_add(prop, '"');
                }
            }

            if (geoms == 0) {
                     fprintf(o->output,
                            "{\"type\":\"Feature\", \"properties\":{\"%s}}\n",
                            prop->buf);
            } else if (geoms == 1) {
                fprintf(o->output,
                        "{\"type\":\"Feature\", \"properties\":{\"%s}, \"geometry\":%s}\n",
                        prop->buf, geom->buf);
            } else if (geoms > 1) {
                fprintf(o->output,
                        "{\"type\":\"Feature\", \"properties\":{\"%s}, \"geometry\":%s%s]}}\n",
                        prop->buf, "{ \"type\": \"GeometryCollection\", \"geometries\": [", geom->buf);
            } 
	    if (j) fprintf(o->output, ",");

            buffer_empty(prop);
            buffer_empty(geom);
        }

        PQclear(res);
        ll = ll->next;
    }

    fprintf(o->output, "]}");

    buffer_free(geom);
    buffer_free(prop);
    buffer_free(value);
}



/*
 * Execute the GetFeature request
 */
void wfs_get_feature(ows * o, wfs_request * wr)
{
    mlist *request_list;

    assert(o != NULL);
    assert(wr != NULL);

    /* retrieve a list of SQL requests from the GetFeature parameters */
    request_list = wfs_retrieve_sql_request_list(o, wr);

       
    if (wr->format == WFS_GML2 || wr->format == WFS_GML3) {
        /* display result of the GetFeature request in GML */
        if (buffer_cmp(wr->resulttype, "hits"))
            wfs_gml_display_hits(o, wr, request_list);
        else
            wfs_gml_display_results(o, wr, request_list);

    } else if (wr->format == WFS_GEOJSON) {
                wfs_geojson_display_results(o, wr, request_list);
    }

    /* add here other functions to display GetFeature response in
           other formats */

    mlist_free(request_list);
}


/*
 * vim: expandtab sw=4 ts=4
 */
