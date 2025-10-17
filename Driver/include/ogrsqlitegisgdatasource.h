/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR DataSource for SqliteGIS format
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 ****************************************************************************/

#ifndef OGR_SQLITEGIS_DATASOURCE_H_INCLUDED
#define OGR_SQLITEGIS_DATASOURCE_H_INCLUDED

#include "ogrsf_frmts.h"
#include <sqlite3.h>
#include <vector>
#include <memory>

class OGRSqliteGISLayer;

/************************************************************************/
/*                    OGRSqliteGISDataSource                            */
/************************************************************************/

class OGRSqliteGISDataSource : public GDALDataset
{
  private:
    sqlite3 *m_poDb = nullptr;
    char *m_pszName = nullptr;
    std::vector<std::unique_ptr<OGRSqliteGISLayer>> m_apoLayers;
    bool m_bUpdate = false;

    bool OpenDatabase(const char *pszFilename, bool bUpdate);
    bool LoadExtension();
    bool DiscoverLayers();
    bool IsGeometryColumn(const char *pszTableName, const char *pszColumnName);

  public:
    OGRSqliteGISDataSource();
    ~OGRSqliteGISDataSource() override;

    // Open/Create methods
    bool Open(const char *pszFilename, bool bUpdate);
    bool Create(const char *pszFilename);

    // GDALDataset interface
    int GetLayerCount() override;
    OGRLayer *GetLayer(int iLayer) override;
    OGRLayer *GetLayerByName(const char *pszName) override;
    
    OGRLayer *ICreateLayer(const char *pszName,
                          const OGRSpatialReference *poSpatialRef = nullptr,
                          OGRwkbGeometryType eGType = wkbUnknown,
                          char **papszOptions = nullptr) override;
    
    int TestCapability(const char *pszCap) override;

    // SqliteGIS specific methods
    sqlite3 *GetDB() { return m_poDb; }
    bool IsUpdateMode() const { return m_bUpdate; }
    
    // Execute SQL
    OGRErr ExecuteSQL(const char *pszStatement);
};

#endif /* OGR_SQLITEGIS_DATASOURCE_H_INCLUDED */
