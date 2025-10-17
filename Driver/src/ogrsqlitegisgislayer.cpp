/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR Layer for SqliteGIS format - Implementation
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 ****************************************************************************/

#include "ogrsqlitegisgislayer.h"
#include "ogrsqlitegisgdatasource.h"
#include "cpl_conv.h"
#include "cpl_string.h"

/************************************************************************/
/*                       OGRSqliteGISLayer()                            */
/************************************************************************/

OGRSqliteGISLayer::OGRSqliteGISLayer(
    OGRSqliteGISDataSource *poDSIn,
    const char *pszTableName,
    const char *pszGeomColumn,
    int nSRID,
    OGRwkbGeometryType eGeomType)
    : m_poDS(poDSIn), m_osTableName(pszTableName),
      m_osGeomColumn(pszGeomColumn), m_nSRID(nSRID),
      m_poStmt(nullptr), m_iNextShapeId(0),
      m_bEOF(false)
{
    // Create feature definition
    m_poFeatureDefn = new OGRFeatureDefn(pszTableName);
    m_poFeatureDefn->Reference();
    m_poFeatureDefn->SetGeomType(eGeomType);
    
    // Set spatial reference if SRID is defined
    if (nSRID > 0)
    {
        OGRSpatialReference *poSRS = new OGRSpatialReference();
        if (poSRS->importFromEPSG(nSRID) == OGRERR_NONE)
        {
            m_poFeatureDefn->GetGeomFieldDefn(0)->SetSpatialRef(poSRS);
        }
        poSRS->Release();
    }
    
    // Read schema
    ReadSchema();
}

/************************************************************************/
/*                      ~OGRSqliteGISLayer()                            */
/************************************************************************/

OGRSqliteGISLayer::~OGRSqliteGISLayer()
{
    ResetReading();
    
    if (m_poFeatureDefn)
        m_poFeatureDefn->Release();
}

/************************************************************************/
/*                          ReadSchema()                                */
/************************************************************************/

void OGRSqliteGISLayer::ReadSchema()
{
    CPLString osSQL;
    osSQL.Printf("PRAGMA table_info('%s')", m_osTableName.c_str());
    
    sqlite3_stmt *poStmt = nullptr;
    int rc = sqlite3_prepare_v2(m_poDS->GetDB(), osSQL, -1, &poStmt, nullptr);
    
    if (rc != SQLITE_OK)
        return;
    
    while (sqlite3_step(poStmt) == SQLITE_ROW)
    {
        const char *pszName = (const char *)sqlite3_column_text(poStmt, 1);
        const char *pszType = (const char *)sqlite3_column_text(poStmt, 2);
        
        // Skip FID and geometry columns
        if (EQUAL(pszName, "fid") || EQUAL(pszName, m_osGeomColumn.c_str()))
            continue;
        
        // Add field
        OGRFieldType eType = OFTString;
        if (pszType)
        {
            if (EQUAL(pszType, "INTEGER"))
                eType = OFTInteger;
            else if (EQUAL(pszType, "REAL") || EQUAL(pszType, "DOUBLE"))
                eType = OFTReal;
            else if (EQUAL(pszType, "TEXT"))
                eType = OFTString;
        }
        
        OGRFieldDefn oField(pszName, eType);
        m_poFeatureDefn->AddFieldDefn(&oField);
    }
    
    sqlite3_finalize(poStmt);
}

/************************************************************************/
/*                         ResetReading()                               */
/************************************************************************/

void OGRSqliteGISLayer::ResetReading()
{
    if (m_poStmt)
    {
        sqlite3_finalize(m_poStmt);
        m_poStmt = nullptr;
    }
    
    m_iNextShapeId = 0;
    m_bEOF = false;
}

/************************************************************************/
/*                       GetNextRawFeature()                            */
/************************************************************************/

OGRFeature *OGRSqliteGISLayer::GetNextRawFeature()
{
    if (m_bEOF)
        return nullptr;
    
    // Prepare statement on first call
    if (m_poStmt == nullptr)
    {
        CPLString osSQL;
        osSQL.Printf("SELECT * FROM \"%s\"", m_osTableName.c_str());
        
        int rc = sqlite3_prepare_v2(m_poDS->GetDB(), osSQL, -1, &m_poStmt, nullptr);
        if (rc != SQLITE_OK)
        {
            CPLError(CE_Failure, CPLE_AppDefined,
                     "Failed to prepare statement: %s",
                     sqlite3_errmsg(m_poDS->GetDB()));
            return nullptr;
        }
    }
    
    // Get next row
    int rc = sqlite3_step(m_poStmt);
    if (rc != SQLITE_ROW)
    {
        m_bEOF = true;
        return nullptr;
    }
    
    // Create feature
    OGRFeature *poFeature = new OGRFeature(m_poFeatureDefn);
    
    // Get columns
    int nCols = sqlite3_column_count(m_poStmt);
    int iField = 0;
    
    for (int i = 0; i < nCols; i++)
    {
        const char *pszColName = sqlite3_column_name(m_poStmt, i);
        
        // FID column
        if (EQUAL(pszColName, "fid"))
        {
            poFeature->SetFID(sqlite3_column_int64(m_poStmt, i));
            continue;
        }
        
        // Geometry column
        if (EQUAL(pszColName, m_osGeomColumn.c_str()))
        {
            int nBytes = sqlite3_column_bytes(m_poStmt, i);
            if (nBytes > 0)
            {
                const void *pabyData = sqlite3_column_blob(m_poStmt, i);
                OGRGeometry *poGeom = ParseEWKB(pabyData, nBytes);
                if (poGeom)
                    poFeature->SetGeometryDirectly(poGeom);
            }
            continue;
        }
        
        // Regular field
        if (iField >= m_poFeatureDefn->GetFieldCount())
            continue;
        
        int nColType = sqlite3_column_type(m_poStmt, i);
        
        if (nColType == SQLITE_NULL)
        {
            // Leave as unset
        }
        else if (nColType == SQLITE_INTEGER)
        {
            poFeature->SetField(iField, (GIntBig)sqlite3_column_int64(m_poStmt, i));
        }
        else if (nColType == SQLITE_FLOAT)
        {
            poFeature->SetField(iField, sqlite3_column_double(m_poStmt, i));
        }
        else if (nColType == SQLITE_TEXT)
        {
            poFeature->SetField(iField, (const char *)sqlite3_column_text(m_poStmt, i));
        }
        
        iField++;
    }
    
    m_iNextShapeId++;
    return poFeature;
}

/************************************************************************/
/*                        GetNextFeature()                              */
/************************************************************************/

OGRFeature *OGRSqliteGISLayer::GetNextFeature()
{
    while (true)
    {
        OGRFeature *poFeature = GetNextRawFeature();
        if (poFeature == nullptr)
            return nullptr;
        
        // Apply filters
        if ((m_poFilterGeom == nullptr || 
             FilterGeometry(poFeature->GetGeometryRef())) &&
            (m_poAttrQuery == nullptr ||
             m_poAttrQuery->Evaluate(poFeature)))
        {
            return poFeature;
        }
        
        delete poFeature;
    }
}

/************************************************************************/
/*                          ParseEWKB()                                 */
/************************************************************************/

OGRGeometry *OGRSqliteGISLayer::ParseEWKB(const void *pabyData, int nBytes)
{
    if (nBytes < 5)
        return nullptr;
    
    const unsigned char *pabyWKB = static_cast<const unsigned char *>(pabyData);
    
    // Read byte order
    OGRwkbByteOrder eByteOrder = (pabyWKB[0] == 0) ? wkbXDR : wkbNDR;
    
    // Read geometry type (with EWKB flags)
    uint32_t nType;
    if (eByteOrder == wkbNDR)
    {
        memcpy(&nType, pabyWKB + 1, 4);
    }
    else
    {
        nType = (pabyWKB[1] << 24) | (pabyWKB[2] << 16) | 
                (pabyWKB[3] << 8) | pabyWKB[4];
    }
    
    // Check for SRID flag
    int nOffset = 5;
    if (nType & 0x20000000)  // SRID_FLAG
    {
        nOffset += 4;  // Skip SRID
    }
    
    // Remove EWKB flags to get standard WKB type
    nType &= 0x1FFFFFFF;
    
    // Let OGR parse the geometry
    OGRGeometry *poGeom = nullptr;
    OGRErr eErr = OGRGeometryFactory::createFromWkb(
        pabyData, nullptr, &poGeom, nBytes);
    
    if (eErr != OGRERR_NONE)
    {
        delete poGeom;
        return nullptr;
    }
    
    return poGeom;
}

/************************************************************************/
/*                         ICreateFeature()                             */
/************************************************************************/

OGRErr OGRSqliteGISLayer::ICreateFeature(OGRFeature *poFeature)
{
    if (!m_poDS->GetUpdate())
        return OGRERR_FAILURE;
    
    // Build INSERT statement
    CPLString osSQL;
    osSQL.Printf("INSERT INTO \"%s\" (", m_osTableName.c_str());
    
    // Add geometry column
    osSQL += "\"";
    osSQL += m_osGeomColumn;
    osSQL += "\"";
    
    // Add other fields
    for (int i = 0; i < m_poFeatureDefn->GetFieldCount(); i++)
    {
        osSQL += ", \"";
        osSQL += m_poFeatureDefn->GetFieldDefn(i)->GetNameRef();
        osSQL += "\"";
    }
    
    osSQL += ") VALUES (?";
    for (int i = 0; i < m_poFeatureDefn->GetFieldCount(); i++)
        osSQL += ", ?";
    osSQL += ")";
    
    // Prepare statement
    sqlite3_stmt *poStmt = nullptr;
    int rc = sqlite3_prepare_v2(m_poDS->GetDB(), osSQL, -1, &poStmt, nullptr);
    if (rc != SQLITE_OK)
        return OGRERR_FAILURE;
    
    // Bind geometry
    OGRGeometry *poGeom = poFeature->GetGeometryRef();
    if (poGeom != nullptr)
    {
        // Convert to EWKB
        int nWKBSize = poGeom->WkbSize();
        std::vector<unsigned char> abyWKB(nWKBSize + 4);  // +4 for SRID
        
        // Write SRID flag if needed
        if (m_nSRID != -1)
        {
            abyWKB[0] = 1;  // Little endian
            uint32_t nTypeWithSRID = poGeom->getGeometryType() | 0x20000000;
            memcpy(&abyWKB[1], &nTypeWithSRID, 4);
            memcpy(&abyWKB[5], &m_nSRID, 4);
            poGeom->exportToWkb(wkbNDR, &abyWKB[9]);
            sqlite3_bind_blob(poStmt, 1, abyWKB.data(), nWKBSize + 4, SQLITE_TRANSIENT);
        }
        else
        {
            poGeom->exportToWkb(wkbNDR, abyWKB.data());
            sqlite3_bind_blob(poStmt, 1, abyWKB.data(), nWKBSize, SQLITE_TRANSIENT);
        }
    }
    else
    {
        sqlite3_bind_null(poStmt, 1);
    }
    
    // Bind other fields
    for (int i = 0; i < m_poFeatureDefn->GetFieldCount(); i++)
    {
        if (!poFeature->IsFieldSet(i))
        {
            sqlite3_bind_null(poStmt, i + 2);
            continue;
        }
        
        OGRFieldType eType = m_poFeatureDefn->GetFieldDefn(i)->GetType();
        if (eType == OFTInteger)
        {
            sqlite3_bind_int64(poStmt, i + 2, poFeature->GetFieldAsInteger64(i));
        }
        else if (eType == OFTReal)
        {
            sqlite3_bind_double(poStmt, i + 2, poFeature->GetFieldAsDouble(i));
        }
        else
        {
            sqlite3_bind_text(poStmt, i + 2, poFeature->GetFieldAsString(i), -1, SQLITE_TRANSIENT);
        }
    }
    
    // Execute
    rc = sqlite3_step(poStmt);
    sqlite3_finalize(poStmt);
    
    if (rc != SQLITE_DONE)
        return OGRERR_FAILURE;
    
    // Set FID
    poFeature->SetFID(sqlite3_last_insert_rowid(m_poDS->GetDB()));
    
    return OGRERR_NONE;
}

/************************************************************************/
/*                        TestCapability()                              */
/************************************************************************/

int OGRSqliteGISLayer::TestCapability(const char *pszCap)
{
    if (EQUAL(pszCap, OLCRandomRead))
        return TRUE;
    else if (EQUAL(pszCap, OLCSequentialWrite))
        return m_poDS->GetUpdate();
    else if (EQUAL(pszCap, OLCRandomWrite))
        return m_poDS->GetUpdate();
    else if (EQUAL(pszCap, OLCFastFeatureCount))
        return TRUE;
    else if (EQUAL(pszCap, OLCFastSpatialFilter))
        return FALSE;
    else if (EQUAL(pszCap, OLCFastGetExtent))
        return FALSE;
    else if (EQUAL(pszCap, OLCCreateField))
        return m_poDS->GetUpdate();
    else if (EQUAL(pszCap, OLCDeleteFeature))
        return m_poDS->GetUpdate();
    else if (EQUAL(pszCap, OLCStringsAsUTF8))
        return TRUE;
    else if (EQUAL(pszCap, OLCTransactions))
        return TRUE;
    
    return FALSE;
}

/************************************************************************/
/*                         GetFeatureCount()                            */
/************************************************************************/

GIntBig OGRSqliteGISLayer::GetFeatureCount(int bForce)
{
    if (m_poFilterGeom == nullptr && m_poAttrQuery == nullptr)
    {
        CPLString osSQL;
        osSQL.Printf("SELECT COUNT(*) FROM \"%s\"", m_osTableName.c_str());
        
        sqlite3_stmt *poStmt = nullptr;
        int rc = sqlite3_prepare_v2(m_poDS->GetDB(), osSQL, -1, &poStmt, nullptr);
        
        if (rc == SQLITE_OK && sqlite3_step(poStmt) == SQLITE_ROW)
        {
            GIntBig nCount = sqlite3_column_int64(poStmt, 0);
            sqlite3_finalize(poStmt);
            return nCount;
        }
        
        if (poStmt)
            sqlite3_finalize(poStmt);
    }
    
    return OGRLayer::GetFeatureCount(bForce);
}
