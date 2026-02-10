#pragma once

#include <pluss/datalog/handle/feed/feed.h>
#include <pluss/datalog/handle/catalog/catalog.h>
#include <pluss/datalog/handle/archive/archive.h>
#include <pluss/datalog/table/thread/thread.h>
#include <pluss/datalog/table/dataframe/dataframe.h>
#include <pluss/datalog/table/atlas/atlas.h>
#include <string>

namespace pluss::converters {

// ========== TABLIZE: Handle → Table ==========

/**
 * @brief Convert Feed to Thread (trivial, same variant)
 */
table::Thread tablize(const handle::Feed& feed);

/**
 * @brief Convert Catalog to DataFrame
 * Keys become a "_key" column in the DataFrame
 * All Feeds must have the same length
 */
table::DataFrame tablize(const handle::Catalog& catalog, const std::string& key_col_name = "_key");

/**
 * @brief Convert Archive to Atlas
 * Recursively converts each Catalog to DataFrame
 */
table::Atlas tablize(const handle::Archive& archive, const std::string& key_col_name = "_key");

// ========== HANDELIZE: Table → Handle ==========

/**
 * @brief Convert Thread to Feed (trivial, same variant)
 */
handle::Feed handelize(const table::Thread& thread);

/**
 * @brief Convert DataFrame to Catalog
 * Requires a key column (default "_key") to use as catalog keys
 * Key column must be Note (string) type with unique values
 * Each row becomes a catalog entry keyed by the key column value
 */
handle::Catalog handelize(const table::DataFrame& df, const std::string& key_col = "_key");

/**
 * @brief Convert Atlas to Archive
 * Recursively converts each DataFrame to Catalog
 */
handle::Archive handelize(const table::Atlas& atlas, const std::string& key_col = "_key");

// ========== VALIDATION ==========

/**
 * @brief Check if DataFrame can be converted to Catalog
 * Validates:
 * - Key column exists
 * - Key column is Note type
 * - All values in key column are unique
 */
bool can_handelize(const table::DataFrame& df, const std::string& key_col = "_key");

/**
 * @brief Check if Catalog can be converted to DataFrame
 * Validates:
 * - All Feeds have the same length
 * - Catalog is not empty
 */
bool can_tablize(const handle::Catalog& catalog);

/**
 * @brief Check if Archive can be converted to Atlas
 * Validates all contained Catalogs
 */
bool can_tablize(const handle::Archive& archive, const std::string& key_col_name = "_key");

/**
 * @brief Check if Atlas can be converted to Archive
 * Validates all contained DataFrames
 */
bool can_handelize(const table::Atlas& atlas, const std::string& key_col = "_key");

}  // namespace pluss::converters
