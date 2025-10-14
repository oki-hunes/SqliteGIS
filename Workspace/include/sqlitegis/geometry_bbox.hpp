#pragma once

#include <sqlite3ext.h>

namespace sqlitegis {

// バウンディングボックス関連の関数を登録
void register_bbox_functions(sqlite3* db);

} // namespace sqlitegis
