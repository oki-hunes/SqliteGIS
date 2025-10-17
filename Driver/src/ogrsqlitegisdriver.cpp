/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR Driver for SqliteGIS format - Driver Registration
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 ****************************************************************************/

#include "ogrsqlitegisdriver.h"
#include "ogrsqlitegisgdatasource.h"
#include "cpl_conv.h"
#include "cpl_string.h"

/************************************************************************/
/*                            Identify()                                 */
/************************************************************************/

int OGRSqliteGISDriver::Identify(GDALOpenInfo *poOpenInfo)
{
    // Check file extension
    if (poOpenInfo->pszFilename == nullptr)
        return FALSE;

    // Check for .sqlitegis extension
    const char *pszExt = CPLGetExtension(poOpenInfo->pszFilename);
    if (EQUAL(pszExt, "sqlitegis"))
        return TRUE;

    // Check SQLite magic number if file exists
    if (poOpenInfo->nHeaderBytes >= 16 &&
        STARTS_WITH_CI((const char *)poOpenInfo->pabyHeader, "SQLite format 3"))
    {
        // Could be a SQLite database, but we prefer .sqlitegis extension
        return GDAL_IDENTIFY_UNKNOWN;
    }

    return FALSE;
}

/************************************************************************/
/*                              Open()                                   */
/************************************************************************/

GDALDataset *OGRSqliteGISDriver::Open(GDALOpenInfo *poOpenInfo)
{
    if (Identify(poOpenInfo) == FALSE)
        return nullptr;

    // Determine update mode
    bool bUpdate = (poOpenInfo->eAccess == GA_Update);

    // Create and open datasource
    auto poDS = new OGRSqliteGISDataSource();
    if (!poDS->Open(poOpenInfo->pszFilename, bUpdate))
    {
        delete poDS;
        return nullptr;
    }

    return poDS;
}

/************************************************************************/
/*                             Create()                                  */
/************************************************************************/

GDALDataset *OGRSqliteGISDriver::Create(const char *pszName,
                                        int /* nXSize */,
                                        int /* nYSize */,
                                        int /* nBands */,
                                        GDALDataType /* eType */,
                                        char ** /* papszOptions */)
{
    // Create new SqliteGIS database
    auto poDS = new OGRSqliteGISDataSource();
    if (!poDS->Create(pszName))
    {
        delete poDS;
        return nullptr;
    }

    return poDS;
}

/************************************************************************/
/*                        RegisterOGRSqliteGIS()                        */
/************************************************************************/

void RegisterOGRSqliteGIS()
{
    if (GDALGetDriverByName("SqliteGIS") != nullptr)
        return;

    GDALDriver *poDriver = new OGRSqliteGISDriver();

    poDriver->SetDescription("SqliteGIS");
    poDriver->SetMetadataItem(GDAL_DCAP_VECTOR, "YES");
    poDriver->SetMetadataItem(GDAL_DMD_LONGNAME,
                             "SqliteGIS - PostGIS-compatible SQLite GIS Extension");
    poDriver->SetMetadataItem(GDAL_DMD_EXTENSION, "sqlitegis");
    poDriver->SetMetadataItem(GDAL_DMD_HELPTOPIC, "drivers/vector/sqlitegis.html");
    
    // Supported geometry types
    poDriver->SetMetadataItem(GDAL_DMD_CREATIONOPTIONLIST,
        "<CreationOptionList>"
        "  <Option name='SPATIALITE' type='boolean' description='Create as SpatiaLite compatible' default='NO'/>"
        "</CreationOptionList>");

    // Layer creation options
    poDriver->SetMetadataItem(GDAL_DS_LAYER_CREATIONOPTIONLIST,
        "<LayerCreationOptionList>"
        "  <Option name='GEOMETRY_NAME' type='string' description='Name of geometry column' default='geom'/>"
        "  <Option name='SRID' type='int' description='Spatial Reference System ID' default='-1'/>"
        "  <Option name='SPATIAL_INDEX' type='boolean' description='Create spatial index (R-tree)' default='YES'/>"
        "  <Option name='FID' type='string' description='Name of FID column' default='fid'/>"
        "</LayerCreationOptionList>");

    // Capabilities
    poDriver->SetMetadataItem(GDAL_DCAP_CREATE, "YES");
    poDriver->SetMetadataItem(GDAL_DCAP_CREATECOPY, "YES");

    // Set driver functions
    poDriver->pfnIdentify = OGRSqliteGISDriver::Identify;
    poDriver->pfnOpen = OGRSqliteGISDriver::Open;
    poDriver->pfnCreate = OGRSqliteGISDriver::Create;

    GetGDALDriverManager()->RegisterDriver(poDriver);
}

/************************************************************************/
/*                    GDALRegister_SqliteGIS()                          */
/*  Alternative entry point for explicit registration                   */
/************************************************************************/

extern "C" void CPL_DLL GDALRegister_SqliteGIS()
{
    RegisterOGRSqliteGIS();
}
