/******************************************************************************
 * Project:  SqliteGIS GDAL Driver
 * Purpose:  OGR Driver for SqliteGIS format
 * Author:   SqliteGIS Project
 ******************************************************************************
 * Copyright (c) 2025, SqliteGIS Project
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#ifndef OGR_SQLITEGIS_DRIVER_H_INCLUDED
#define OGR_SQLITEGIS_DRIVER_H_INCLUDED

#include "ogrsf_frmts.h"

/************************************************************************/
/*                      OGRSqliteGISDriver                              */
/************************************************************************/

class OGRSqliteGISDriver : public GDALDriver
{
  public:
    OGRSqliteGISDriver() = default;
    ~OGRSqliteGISDriver() override = default;

    // Driver identification
    static const char *GetName() { return "SqliteGIS"; }
    static const char *GetDescription() { return "SqliteGIS - PostGIS-compatible SQLite GIS Extension"; }
    
    // Driver capabilities
    static int Identify(GDALOpenInfo *poOpenInfo);
    static GDALDataset *Open(GDALOpenInfo *poOpenInfo);
    static GDALDataset *Create(const char *pszName, int nXSize, int nYSize,
                               int nBands, GDALDataType eType,
                               char **papszOptions);
};

// Driver registration function
void RegisterOGRSqliteGIS();

#endif /* OGR_SQLITEGIS_DRIVER_H_INCLUDED */
