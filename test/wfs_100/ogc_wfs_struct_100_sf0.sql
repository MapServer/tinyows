-- This file inserts all the appropriate cite data into postgis to run with
-- WFS 1.0.0 Cite Unit test

-- Inspired From OGC CITE: http://cite.opengeospatial.org/te2/wfs-1.0.0-r0/data/data-wfs-1.0.0.zip
-- With several modifications


drop table "Nulls";
drop table "Points";
drop table "Other";
drop table "Lines";
drop table "Polygons";
drop table "MLines";
drop table "MPolygons";
drop table "MPoints";
drop table "Seven";
drop table "Fifteen";
drop table "Updates";
drop table "Inserts";
drop table "Deletes";
drop table "Locks";

delete from geometry_columns where f_table_name = 'Nulls' ;
delete from geometry_columns where f_table_name = 'Points' ;
delete from geometry_columns where f_table_name = 'Other' ;
delete from geometry_columns where f_table_name = 'Lines' ;
delete from geometry_columns where f_table_name = 'Polygons' ;
delete from geometry_columns where f_table_name = 'MLines' ;
delete from geometry_columns where f_table_name = 'MPolygons' ;
delete from geometry_columns where f_table_name = 'MPoints' ;
delete from geometry_columns where f_table_name = 'Seven' ;
delete from geometry_columns where f_table_name = 'Fifteen' ;
delete from geometry_columns where f_table_name = 'Updates' ;
delete from geometry_columns where f_table_name = 'Inserts' ;
delete from geometry_columns where f_table_name = 'Deletes' ;
delete from geometry_columns where f_table_name = 'Locks' ;


--
-- Create SQL structure
--

CREATE TABLE "Seven" ();
ALTER TABLE "Seven" ADD COLUMN "name" character varying;
SELECT AddGeometryColumn('Seven', 'pointProperty', 32615, 'POINT', 2); 
SELECT AddGeometryColumn('Seven', 'boundedBy', 32615, 'POLYGON', 2); 


CREATE TABLE "Nulls" ();
ALTER TABLE "Nulls" ADD COLUMN "description" character varying;
ALTER TABLE "Nulls" ADD COLUMN "name" character varying;
SELECT AddGeometryColumn('Nulls', 'boundedBy', 32615, 'POLYGON', 2); 
ALTER TABLE "Nulls" ADD COLUMN "integers" integer;
ALTER TABLE "Nulls" ADD COLUMN "dates" date;
SELECT AddGeometryColumn('Nulls', 'pointProperty', 32615, 'POINT', 2); 



CREATE TABLE "Deletes" ();
SELECT AddGeometryColumn('Deletes', 'boundedBy', 32615, 'POLYGON', 2); 
ALTER TABLE "Deletes" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Deletes', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Inserts" ();
SELECT AddGeometryColumn('Inserts', 'boundedBy', 32615, 'POLYGON', 2); 
ALTER TABLE "Inserts" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Inserts', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Updates" ();
SELECT AddGeometryColumn('Updates', 'boundedBy', 32615, 'POLYGON', 2); 
ALTER TABLE "Updates" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Updates', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Points" ();
ALTER TABLE "Points" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Points', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Lines" ();
ALTER TABLE "Lines" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Lines', 'lineStringProperty', 32615, 'LINESTRING', 2); 


CREATE TABLE "Polygons" ();
ALTER TABLE "Polygons" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Polygons', 'polygonProperty', 32615, 'POLYGON', 2); 


CREATE TABLE "MPoints" ();
ALTER TABLE "MPoints" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('MPoints', 'multiPointProperty', 32615, 'MULTIPOINT', 2); 


CREATE TABLE "MLines" ();
ALTER TABLE "MLines" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('MLines', 'multiLineStringProperty', 32615, 'MULTILINESTRING', 2); 


CREATE TABLE "MPolygons" ();
ALTER TABLE "MPolygons" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('MPolygons', 'multiPolygonProperty', 32615, 'MULTIPOLYGON', 2); 


CREATE TABLE "Fifteen" ();
ALTER TABLE "Fifteen" ADD COLUMN "name" character varying;
SELECT AddGeometryColumn('Fifteen', 'boundedBy', 32615, 'POLYGON', 2); 
SELECT AddGeometryColumn('Fifteen', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Locks" ();
SELECT AddGeometryColumn('Locks', 'boundedBy', 32615, 'POLYGON', 2); 
ALTER TABLE "Locks" ADD COLUMN "id" character varying;
SELECT AddGeometryColumn('Locks', 'pointProperty', 32615, 'POINT', 2); 


CREATE TABLE "Other" ();
ALTER TABLE "Other" ADD COLUMN "description" character varying;
ALTER TABLE "Other" ADD COLUMN "name" character varying;
SELECT AddGeometryColumn('Other', 'boundedBy', 32615, 'POLYGON', 2); 
SELECT AddGeometryColumn('Other', 'pointProperty', 32615, 'POINT', 2); 
ALTER TABLE "Other" ADD COLUMN "string1" character varying NOT NULL;
ALTER TABLE "Other" ADD COLUMN "string2" character varying;
ALTER TABLE "Other" ADD COLUMN "integers" integer;
ALTER TABLE "Other" ADD COLUMN "dates" date;

--
-- Add Primary keys
--
alter table "Deletes" add column pkey serial;
alter table "Deletes" add primary key (pkey);

alter table "Fifteen" add column pkey serial;
alter table "Fifteen" add primary key (pkey);

alter table "Inserts" add column pkey serial;
alter table "Inserts" add primary key (pkey);

alter table "Lines" add column pkey serial;
alter table "Lines" add primary key (pkey);

alter table "Locks" add column pkey serial;
alter table "Locks" add primary key (pkey);

alter table "MLines" add column pkey serial;
alter table "MLines" add primary key (pkey);

alter table "MPoints" add column pkey serial;
alter table "MPoints" add primary key (pkey);

alter table "MPolygons" add column pkey serial;
alter table "MPolygons" add primary key (pkey);

alter table "Nulls" add column pkey serial;
alter table "Nulls" add primary key (pkey);

alter table "Other" add column pkey serial;
alter table "Other" add primary key (pkey);

alter table "Points" add column pkey serial;
alter table "Points" add primary key (pkey);

alter table "Polygons" add column pkey serial;
alter table "Polygons" add primary key (pkey);

alter table "Seven" add column pkey serial;
alter table "Seven" add primary key (pkey);

alter table "Updates" add column pkey serial;
alter table "Updates" add primary key (pkey);
