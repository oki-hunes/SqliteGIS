/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR Layer for SqliteGIS format
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 ****************************************************************************/

#ifndef OGR_SQLITEGIS_LAYER_H_INCLUDED
#define OGR_SQLITEGIS_LAYER_H_INCLUDED

#include "ogrsf_frmts.h"
#include <sqlite3.h>
#include <string>

class OGRSqliteGISDataSource;

/************************************************************************/
/*                      OGRSqliteGISLayer                               */
/************************************************************************/

class OGRSqliteGISLayer : public OGRLayer
{
  private:
    OGRSqliteGISDataSource *m_poDS = nullptr;
    OGRFeatureDefn *m_poFeatureDefn = nullptr;
    
    std::string m_osTableName;
    std::string m_osGeomColumn;
    int m_nSRID = -1;
    OGRwkbGeometryType m_eGeomType = wkbUnknown;
    
    sqlite3_stmt *m_poStmt = nullptr;
    bool m_bEOF = false;
    GIntBig m_iNextShapeId = 0;
    
    bool BuildFeatureDefn();
    OGRFeature *GetNextRawFeature();
    OGRGeometry *ParseEWKB(const void *pabyData, int nBytes);

  public:
    OGRSqliteGISLayer(OGRSqliteGISDataSource *poDS,
                      const char *pszTableName,
                      const char *pszGeomColumn,
                      int nSRID,
                      OGRwkbGeometryType eGeomType);
    ~OGRSqliteGISLayer() override;

    // OGRLayer interface
    void ResetReading() override;
    OGRFeature *GetNextFeature() override;
    OGRFeatureDefn *GetLayerDefn() override { return m_poFeatureDefn; }
    
    GIntBig GetFeatureCount(int bForce = TRUE) override;
    OGRErr GetExtent(OGREnvelope *psExtent, int bForce = TRUE) override;
    OGRErr GetExtent(int iGeomField, OGREnvelope *psExtent, int bForce) override;
    
    int TestCapability(const char *pszCap) override;
    
    // Write operations
    OGRErr ICreateFeature(OGRFeature *poFeature) override;
    OGRErr ISetFeature(OGRFeature *poFeature) override;
    OGRErr DeleteFeature(GIntBig nFID) override;
    
    // Spatial filter
    void SetSpatialFilter(OGRGeometry *poGeom) override;
    void SetSpatialFilter(int iGeomField, OGRGeometry *poGeom) override;

    // Attribute filter
    OGRErr SetAttributeFilter(const char *pszQuery) override;

    // Transaction support
    OGRErr StartTransaction() override;
    OGRErr CommitTransaction() override;
    OGRErr RollbackTransaction() override;
};

#endif /* OGR_SQLITEGIS_LAYER_H_INCLUDED */
