/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR DataSource for SqliteGIS format - Implementation
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 ****************************************************************************/

#include "ogrsqlitegisgdatasource.h"
#include "ogrsqlitegisgislayer.h"
#include "cpl_conv.h"
#include "cpl_string.h"
#include <algorithm>

/************************************************************************/
/*                      OGRSqliteGISDataSource()                        */
/************************************************************************/

OGRSqliteGISDataSource::OGRSqliteGISDataSource()
    : m_poDb(nullptr), m_pszName(nullptr), m_bUpdate(false)
{
}

/************************************************************************/
/*                     ~OGRSqliteGISDataSource()                        */
/************************************************************************/

OGRSqliteGISDataSource::~OGRSqliteGISDataSource()
{
    m_apoLayers.clear();
    
    if (m_poDb != nullptr)
    {
        sqlite3_close(m_poDb);
        m_poDb = nullptr;
    }
    
    CPLFree(m_pszName);
}

/************************************************************************/
/*                          OpenDatabase()                              */
/************************************************************************/

bool OGRSqliteGISDataSource::OpenDatabase(const char *pszFilename, bool bUpdate)
{
    int flags = bUpdate ? (SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE) 
                        : SQLITE_OPEN_READONLY;
    
    int rc = sqlite3_open_v2(pszFilename, &m_poDb, flags, nullptr);
    if (rc != SQLITE_OK)
    {
        CPLError(CE_Failure, CPLE_OpenFailed,
                 "sqlite3_open(%s) failed: %s",
                 pszFilename, sqlite3_errmsg(m_poDb));
        sqlite3_close(m_poDb);
        m_poDb = nullptr;
        return false;
    }
    
    return true;
}

/************************************************************************/
/*                        LoadExtension()                               */
/************************************************************************/

bool OGRSqliteGISDataSource::LoadExtension()
{
    // Enable extension loading
    sqlite3_enable_load_extension(m_poDb, 1);
    
    // Try to load sqlitegis extension
    // First try from same directory as the database
    char *pszErrMsg = nullptr;
    
#ifdef _WIN32
    const char *pszExtName = "sqlitegis.dll";
#else
    const char *pszExtName = "sqlitegis.so";
#endif
    
    int rc = sqlite3_load_extension(m_poDb, pszExtName, nullptr, &pszErrMsg);
    
    if (rc != SQLITE_OK)
    {
        CPLDebug("SqliteGIS", "Failed to load extension: %s", 
                 pszErrMsg ? pszErrMsg : "unknown error");
        sqlite3_free(pszErrMsg);
        
        // Extension not loaded - continue anyway, user might have it preloaded
        // or database might not need it yet
    }
    
    sqlite3_enable_load_extension(m_poDb, 0);
    return true;
}

/************************************************************************/
/*                         DiscoverLayers()                             */
/************************************************************************/

bool OGRSqliteGISDataSource::DiscoverLayers()
{
    // Query sqlite_master for all tables
    const char *pszSQL = 
        "SELECT name FROM sqlite_master "
        "WHERE type='table' AND name NOT LIKE 'sqlite_%'";
    
    sqlite3_stmt *poStmt = nullptr;
    int rc = sqlite3_prepare_v2(m_poDb, pszSQL, -1, &poStmt, nullptr);
    
    if (rc != SQLITE_OK)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "Failed to query tables: %s", sqlite3_errmsg(m_poDb));
        return false;
    }
    
    // For each table, check if it has a geometry column
    while (sqlite3_step(poStmt) == SQLITE_ROW)
    {
        const char *pszTableName = (const char *)sqlite3_column_text(poStmt, 0);
        
        // Get table info
        CPLString osSQL;
        osSQL.Printf("PRAGMA table_info('%s')", pszTableName);
        
        sqlite3_stmt *poInfoStmt = nullptr;
        rc = sqlite3_prepare_v2(m_poDb, osSQL, -1, &poInfoStmt, nullptr);
        
        if (rc == SQLITE_OK)
        {
            std::string osGeomColumn;
            
            // Look for BLOB columns that might contain geometry
            while (sqlite3_step(poInfoStmt) == SQLITE_ROW)
            {
                const char *pszColName = (const char *)sqlite3_column_text(poInfoStmt, 1);
                const char *pszColType = (const char *)sqlite3_column_text(poInfoStmt, 2);
                
                // Check if this looks like a geometry column
                if (pszColType && EQUAL(pszColType, "BLOB") &&
                    (EQUAL(pszColName, "geom") || EQUAL(pszColName, "geometry") ||
                     EQUAL(pszColName, "the_geom") || EQUAL(pszColName, "wkb_geometry")))
                {
                    osGeomColumn = pszColName;
                    break;
                }
            }
            
            sqlite3_finalize(poInfoStmt);
            
            // Create layer if geometry column found
            if (!osGeomColumn.empty())
            {
                auto poLayer = std::make_unique<OGRSqliteGISLayer>(
                    this, pszTableName, osGeomColumn.c_str(), -1, wkbUnknown);
                m_apoLayers.push_back(std::move(poLayer));
            }
        }
    }
    
    sqlite3_finalize(poStmt);
    return true;
}

/************************************************************************/
/*                              Open()                                   */
/************************************************************************/

bool OGRSqliteGISDataSource::Open(const char *pszFilename, bool bUpdate)
{
    m_pszName = CPLStrdup(pszFilename);
    m_bUpdate = bUpdate;
    
    // Open database
    if (!OpenDatabase(pszFilename, bUpdate))
        return false;
    
    // Load SqliteGIS extension
    LoadExtension();
    
    // Discover layers
    if (!DiscoverLayers())
        return false;
    
    return true;
}

/************************************************************************/
/*                             Create()                                  */
/************************************************************************/

bool OGRSqliteGISDataSource::Create(const char *pszFilename)
{
    m_pszName = CPLStrdup(pszFilename);
    m_bUpdate = true;
    
    // Create new database
    if (!OpenDatabase(pszFilename, true))
        return false;
    
    // Load SqliteGIS extension
    LoadExtension();
    
    return true;
}

/************************************************************************/
/*                          GetLayerCount()                             */
/************************************************************************/

int OGRSqliteGISDataSource::GetLayerCount()
{
    return static_cast<int>(m_apoLayers.size());
}

/************************************************************************/
/*                            GetLayer()                                 */
/************************************************************************/

OGRLayer *OGRSqliteGISDataSource::GetLayer(int iLayer)
{
    if (iLayer < 0 || iLayer >= static_cast<int>(m_apoLayers.size()))
        return nullptr;
    
    return m_apoLayers[iLayer].get();
}

/************************************************************************/
/*                         GetLayerByName()                             */
/************************************************************************/

OGRLayer *OGRSqliteGISDataSource::GetLayerByName(const char *pszName)
{
    for (auto &poLayer : m_apoLayers)
    {
        if (EQUAL(poLayer->GetName(), pszName))
            return poLayer.get();
    }
    
    return nullptr;
}

/************************************************************************/
/*                          ICreateLayer()                              */
/************************************************************************/

OGRLayer *OGRSqliteGISDataSource::ICreateLayer(
    const char *pszName,
    const OGRSpatialReference *poSpatialRef,
    OGRwkbGeometryType eGType,
    char **papszOptions)
{
    if (!m_bUpdate)
    {
        CPLError(CE_Failure, CPLE_NoWriteAccess,
                 "Cannot create layer in read-only mode");
        return nullptr;
    }
    
    // Get options
    const char *pszGeomColumn = CSLFetchNameValueDef(papszOptions, "GEOMETRY_NAME", "geom");
    const char *pszFIDColumn = CSLFetchNameValueDef(papszOptions, "FID", "fid");
    int nSRID = atoi(CSLFetchNameValueDef(papszOptions, "SRID", "-1"));
    bool bCreateSpatialIndex = CPLTestBool(CSLFetchNameValueDef(papszOptions, "SPATIAL_INDEX", "YES"));
    
    // Get SRID from spatial reference
    if (nSRID == -1 && poSpatialRef != nullptr)
    {
        const char *pszAuthName = poSpatialRef->GetAuthorityName(nullptr);
        const char *pszAuthCode = poSpatialRef->GetAuthorityCode(nullptr);
        
        if (pszAuthName && EQUAL(pszAuthName, "EPSG") && pszAuthCode)
        {
            nSRID = atoi(pszAuthCode);
        }
    }
    
    // Create table
    CPLString osSQL;
    osSQL.Printf("CREATE TABLE \"%s\" (\"%s\" INTEGER PRIMARY KEY AUTOINCREMENT, \"%s\" BLOB)",
                 pszName, pszFIDColumn, pszGeomColumn);
    
    if (ExecuteSQL(osSQL) != OGRERR_NONE)
        return nullptr;
    
    // Create spatial index if requested
    if (bCreateSpatialIndex)
    {
        osSQL.Printf("CREATE VIRTUAL TABLE \"rtree_%s_%s\" USING rtree(id, minx, maxx, miny, maxy)",
                     pszName, pszGeomColumn);
        ExecuteSQL(osSQL);  // Non-fatal if fails
    }
    
    // Create and add layer
    auto poLayer = std::make_unique<OGRSqliteGISLayer>(
        this, pszName, pszGeomColumn, nSRID, eGType);
    
    OGRLayer *poRetLayer = poLayer.get();
    m_apoLayers.push_back(std::move(poLayer));
    
    return poRetLayer;
}

/************************************************************************/
/*                         TestCapability()                             */
/************************************************************************/

int OGRSqliteGISDataSource::TestCapability(const char *pszCap)
{
    if (EQUAL(pszCap, ODsCCreateLayer))
        return m_bUpdate;
    else if (EQUAL(pszCap, ODsCDeleteLayer))
        return m_bUpdate;
    else if (EQUAL(pszCap, ODsCCreateGeomFieldAfterCreateLayer))
        return m_bUpdate;
    else if (EQUAL(pszCap, ODsCTransactions))
        return TRUE;
    
    return FALSE;
}

/************************************************************************/
/*                          ExecuteSQL()                                */
/************************************************************************/

OGRErr OGRSqliteGISDataSource::ExecuteSQL(const char *pszStatement)
{
    char *pszErrMsg = nullptr;
    int rc = sqlite3_exec(m_poDb, pszStatement, nullptr, nullptr, &pszErrMsg);
    
    if (rc != SQLITE_OK)
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                 "SQL error: %s\nStatement: %s",
                 pszErrMsg ? pszErrMsg : "unknown", pszStatement);
        sqlite3_free(pszErrMsg);
        return OGRERR_FAILURE;
    }
    
    return OGRERR_NONE;
}
