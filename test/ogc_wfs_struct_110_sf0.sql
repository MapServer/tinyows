-- PostGIS setup script for wfs 1.1 sf0 cite dataset
-- 
-- From OGC CITE: http://cite.opengeospatial.org/te2/wfs-1.1.0-r0/data/data-wfs-1.1.0.zip
-- With several modifications
--
--SET client_encoding = 'UTF8';

delete from geometry_columns where f_table_name = 'PrimitiveGeoFeature' ;
delete from geometry_columns where f_table_name = 'AggregateGeoFeature' ;
delete from geometry_columns where f_table_name = 'EntitéGénérique' ;

drop table "PrimitiveGeoFeature";
create table "PrimitiveGeoFeature" ( description varchar, name varchar );

select addgeometrycolumn( 'public', 'PrimitiveGeoFeature', 'surfaceProperty', 4326, 'POLYGON', 2 );
select addgeometrycolumn( 'public', 'PrimitiveGeoFeature', 'pointProperty', 4326, 'POINT', 2 );
select addgeometrycolumn( 'public', 'PrimitiveGeoFeature', 'curveProperty', 4326, 'LINESTRING', 2 );

alter table "PrimitiveGeoFeature" add "intProperty" int8 not null;
alter table "PrimitiveGeoFeature" add "uriProperty" varchar;
alter table "PrimitiveGeoFeature" add measurand float not null;
alter table "PrimitiveGeoFeature" add "dateTimeProperty" timestamp with time zone;
alter table "PrimitiveGeoFeature" add "dateProperty" date;
alter table "PrimitiveGeoFeature" add "decimalProperty" float not null;
alter table "PrimitiveGeoFeature" add id varchar; 
alter table "PrimitiveGeoFeature" add primary key ( id );

drop table "AggregateGeoFeature";
create table "AggregateGeoFeature" ( description varchar, name varchar );

select addgeometrycolumn( 'public', 'AggregateGeoFeature', 'multiPointProperty', 4326, 'MULTIPOINT', 2 );
select addgeometrycolumn( 'public', 'AggregateGeoFeature', 'multiCurveProperty', 4326, 'MULTILINESTRING', 2 );
select addgeometrycolumn( 'public', 'AggregateGeoFeature', 'multiSurfaceProperty', 4326, 'MULTIPOLYGON', 2 );

alter table "AggregateGeoFeature" add "doubleProperty" float not null;
alter table "AggregateGeoFeature" add "intRangeProperty" varchar;
alter table "AggregateGeoFeature" add "strProperty" varchar not null;
alter table "AggregateGeoFeature" add "featureCode" varchar not null;
alter table "AggregateGeoFeature" add id varchar; 
alter table "AggregateGeoFeature" add primary key ( id );

drop table "EntitéGénérique";
create table "EntitéGénérique" ( description varchar, name varchar );

select addgeometrycolumn( 'public', 'EntitéGénérique', 'attribut.Géométrie', 4326, 'GEOMETRY', 2 );

alter table "EntitéGénérique" add "boolProperty" boolean not null;
alter table "EntitéGénérique" add "str4Property" varchar not null;
alter table "EntitéGénérique" add "featureRef" varchar;
alter table "EntitéGénérique" add id varchar; 
alter table "EntitéGénérique" add primary key ( id );
