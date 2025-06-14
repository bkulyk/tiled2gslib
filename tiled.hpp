#include <vector>
#include <fstream>
#include <filesystem>
#include "lib/stb_image.h"
#include "lib/tileson.hpp"
#include "doc.hpp"

namespace fs = std::filesystem;

typedef std::array<uint16_t, 4> Metatile; // Each metatile consists of 4 words (2x2 tiles)
typedef std::vector<Metatile> Metatiles;
typedef std::vector<uint8_t> Scrolltable;

struct GsltInfo {
  Metatiles metatiles;
  Scrolltable scrolltable;
  std::string tilesetImagePath;
  int width;
  int height;
};

// --- getTileData Function (Revised to return a single combined word) ---
// Extracts and encodes data for a single 8x8 tile into a single 16-bit combined word.
// This word integrates both the tile ID and its attributes.
uint16_t getTileData(int x, int y, tson::Layer* tileLayer, tson::Layer* priorityLayer, tson::Layer* metaLayer) {
  tson::Tile* tile = tileLayer->getTileData(x, y);
  tson::Tile* priorityTile = priorityLayer != nullptr ? priorityLayer->getTileData(x, y) : nullptr;
  tson::Tile* metaTile = metaLayer != nullptr ? metaLayer->getTileData(x, y) : nullptr;

  uint32_t raw_gid_32bit = 0;

  // --- 1. Extract Base Tile ID (0-based) ---
  uint16_t base_tile_id_0based = 0;
  if (tile != nullptr) {
    raw_gid_32bit = tile->getGid();
    base_tile_id_0based = raw_gid_32bit - 1;
  }

  // --- 2. Extract Flip Flags ---
  int hFlip = 0;
  int vFlip = 0;

  if (tile->hasFlipFlags(tson::TileFlipFlags::Horizontally) && tile->hasFlipFlags(tson::TileFlipFlags::Vertically)) {
    // std::cout << "**** flag is flipped diagonally **** " << x << "," << y << " id: " << raw_gid_32bit << std::endl;
    vFlip = hFlip = 1; // Diagonal flip is a combination of horizontal and vertical flips.
  } else if (tile->hasFlipFlags(tson::TileFlipFlags::Horizontally)) {
    // std::cout << "**** flag is flipped horizontally ****" << x << "," << y << " id: " << raw_gid_32bit << std::endl;
    hFlip = 1;
  } else if (tile->hasFlipFlags(tson::TileFlipFlags::Vertically)) {
    // std::cout << "**** flag is flipped vertically ****" << x << "," << y << " id: " << raw_gid_32bit << std::endl;
    vFlip = 1;
  }

  // --- 3. Determine Priority and Meta ID ---
  int priority_flag = (priorityTile != nullptr && priorityTile->getGid() > 0) ? 1 : 0;
  uint16_t current_meta_id = 0;
  if (metaTile != nullptr && metaTile->getGid() > 0) {
    if (metaTile->getTileset() != nullptr) {
      current_meta_id = static_cast<uint16_t>(metaTile->getGid() - metaTile->getTileset()->getFirstgid() + 1);
      if (current_meta_id > 7) {
        std::cerr << "Warning: Meta ID " << current_meta_id << " for tile (" << x << "," << y << ") exceeds 3-bit capacity (0-7). Truncating." << std::endl;
        current_meta_id = 7;
      }
    } else {
      std::cerr << "Warning: Meta tile at (" << x << "," << y << ") has GID but no associated tileset for meta ID calculation." << std::endl;
    }
  }
  
  int palette = 0; // Assuming default palette 0 for now.

  uint16_t combined_word = 0;
  combined_word = combined_word | (hFlip) ? 512 : 0;
  combined_word |= vFlip ? 1024 : 0;
  combined_word |= base_tile_id_0based;
  combined_word |= palette;
  combined_word |= priority_flag != 0 ? 4096 : 0;
  combined_word |= (current_meta_id & 7) << 13;

  return combined_word;
}

void saveMetatileFile(Metatiles metatiles, std::string filename) {
  // Calculate total file length (8 bytes header + metatiles.size() * 4 words/metatile * 2 bytes/word).
  // If your map is 4x4, extractMetaTiles will produce 4 metatiles.
  // So, metatiles.size() will be 4.
  // Total data bytes = 4 * 4 * 2 = 32 bytes.
  // Total file length = 32 bytes (data) + 8 bytes (header) = 40 bytes.
  uint16_t total_file_length = static_cast<uint16_t>(8 + metatiles.size() * 4 * 2);

  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) {
    std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
    return;
  }

  // --- Write the 8-byte header ---
  ofs.put(static_cast<char>(total_file_length & 0xFF));        // LSB (Lower byte of length)
  ofs.put(static_cast<char>((total_file_length >> 8) & 0xFF)); // MSB (Upper byte of length)
  for (int i = 0; i < 6; ++i) {
    ofs.put(0x00); // Write a null byte
  }

  // --- Write the metatile data ---
  // Each uint16_t word needs to be written as 2 bytes.
  // We write all metatiles generated from the map.
  for (const auto& metatile : metatiles) { // <<< THIS IS THE CORRECT LOOP
    for (uint16_t val : metatile) { // Iterate over its 4 uint16_t words
      ofs.put(static_cast<char>(val & 0xFF));        // Lower byte (LSB)
      ofs.put(static_cast<char>((val >> 8) & 0xFF)); // Upper byte (MSB)
    }
  }
  ofs.close();
}

void saveScrolltable(Scrolltable& scrolltable, const std::string& filename, uint16_t width, uint16_t height) {
  uint16_t tile_size = 8;
  uint16_t width_in_metatiles = width / 2;
  uint16_t height_in_metatiles = width / 2;
  uint16_t data_bytes = width_in_metatiles * height_in_metatiles;
  uint16_t total_bytes = data_bytes;
  uint16_t width_pixels = width_in_metatiles * tile_size * 2;
  uint16_t height_pixels = height_in_metatiles * tile_size * 2;
  uint16_t vertical_addition = width_in_metatiles * 13;
  uint8_t option_byte = 0x01;

  std::ofstream ofs(filename, std::ios::binary);
  if (!ofs) {
    std::cerr << "Error: Could not open file for writing: " << filename << std::endl;
    return;
  }

  // Write header (little-endian)
  ofs.put(static_cast<char>(total_bytes & 0xFF));
  ofs.put(static_cast<char>((total_bytes >> 8) & 0xFF));
  ofs.put(static_cast<char>(width_in_metatiles & 0xFF));
  ofs.put(static_cast<char>((width_in_metatiles >> 8) & 0xFF));
  ofs.put(static_cast<char>(height_in_metatiles & 0xFF));
  ofs.put(static_cast<char>((height_in_metatiles >> 8) & 0xFF));
  ofs.put(static_cast<char>(width_pixels & 0xFF));
  ofs.put(static_cast<char>((width_pixels >> 8) & 0xFF));
  ofs.put(static_cast<char>(height_pixels & 0xFF));
  ofs.put(static_cast<char>((height_pixels >> 8) & 0xFF));
  ofs.put(static_cast<char>(vertical_addition & 0xFF));
  ofs.put(static_cast<char>((vertical_addition >> 8) & 0xFF));
  ofs.put(static_cast<char>(option_byte));

  // Write scrolltable data
  for (uint8_t metatile_id : scrolltable) {
    uint8_t entry = static_cast<uint8_t>(((metatile_id << 3) & 0xF8) + ((metatile_id >> 5) & 0x07));
    ofs.put(static_cast<char>(entry));
  }

  ofs.close();
}

// --- extractMetaTiles Function ---
// Extracts 2x2 metatiles from the map.
// This function remains generic and processes all metatiles in the map.
GsltInfo extractMetaTiles(Options *opts, std::unique_ptr<tson::Map> *map) {
  tson::Map* m = map->get();
  tson::Layer* tileLayer = m->getLayer(opts->tile_layer);
  tson::Layer* priorityLayer = m->getLayer(opts->priority_layer);
  tson::Layer* metaLayer = m->getLayer(opts->meta_layer);
  tson::Vector2i size = m->getSize(); // Map size in tiles (e.g., 4x4)
  Metatiles all_metatiles; // Change to 4 words per metatile

  std::cout << "size: " << size.x << " x " << size.y << std::endl;

  // Iterate through the map in 2x2 blocks to form metatiles (row-major order)
  for (int y = 0; y < size.y; y += 2) {
    for (int x = 0; x < size.x; x += 2) {
      // Skip incomplete metatiles at the edges of the map
      if (x + 1 >= size.x || y + 1 >= size.y) {
        std::cerr << "Warning: Skipping incomplete metatile at (" << x << "," << y << ") due to map edge." << std::endl;
        continue;
      }

      // Get data for the four 8x8 tiles forming the 2x2 metatile
      uint16_t tl_word = getTileData(x, y, tileLayer, priorityLayer, metaLayer);     // Top-Left
      uint16_t tr_word = getTileData(x+1, y, tileLayer, priorityLayer, metaLayer);   // Top-Right
      uint16_t bl_word = getTileData(x, y+1, tileLayer, priorityLayer, metaLayer);   // Bottom-Left
      uint16_t br_word = getTileData(x+1, y+1, tileLayer, priorityLayer, metaLayer); // Bottom-Right

      // Assemble the 8-word metatile array: [TL_ID, TL_ATTRS, TR_ID, TR_ATTRS, BL_ID, BL_ATTRS, BR_ID, BR_ATTRS]
      std::array<uint16_t, 4> metatile{}; // Array of 4 words
      metatile[0] = tl_word;
      metatile[1] = tr_word;
      metatile[2] = bl_word;
      metatile[3] = br_word;

      all_metatiles.push_back(metatile);
    }
  }

  std::vector<std::array<uint16_t, 4>> unique_metatiles;
  Scrolltable scrolltable;

  // Build the scrolltable from the unique metatiles
  scrolltable.reserve(all_metatiles.size()); // Reserve space for scrolltable
  for (const auto& metatile : all_metatiles) {
    // Try to find the metatile in the unique_metatiles vector
    auto it = std::find(unique_metatiles.begin(), unique_metatiles.end(), metatile);
    int metatile_id;
    if (it != unique_metatiles.end()) {
      // Already exists, get its 1-based index
      metatile_id = std::distance(unique_metatiles.begin(), it) + 1;
    } else {
      // New unique metatile, add to vector and assign new ID
      unique_metatiles.push_back(metatile);
      metatile_id = unique_metatiles.size(); // 1-based
    }

    scrolltable.push_back(metatile_id);
  }

  std::cout << "metatile count: " << unique_metatiles.size() << std::endl;

  GsltInfo info = {unique_metatiles, scrolltable, m->getTilesets()[0].getImagePath(), size.x, size.y};
  return info;
}

// use the path from opts->input_file and append the tile_path
std::string getAbsoluteTilePath(Options *opts, std::string tile_path) {
  std::string input_dir = fs::path(opts->input_file).parent_path().string();
  
  // join the input directory with the tile path
  return (fs::path(input_dir) / tile_path).string();
}

// --- processTiledDoc Function ---
// Main function to process the Tiled map and extract/save metatiles and scrolltable.
int processTiledDoc(Options *opts) {
  std::cout << "Processing... " << opts->input_file << std::endl;

  // Parse the Tiled file using Tileson
  tson::Tileson t;
  std::unique_ptr<tson::Map> map = t.parse(opts->input_file);
  if (map->getStatus() != tson::ParseStatus::OK) {
    std::cerr << "Failed to parse Tiled map: " << opts->input_file << std::endl;
    return 1;
  }

  GsltInfo info = extractMetaTiles(opts, &map);
  std::cout << std::endl;

  if (!opts->save_metatiles_file.empty()) {
    saveMetatileFile(info.metatiles, opts->save_metatiles_file);
    std::cout << "Saved metatiles to: " << opts->save_metatiles_file << std::endl;
  }

  if (!opts->save_scrolltable_file.empty()) {
    saveScrolltable(info.scrolltable, opts->save_scrolltable_file, info.width, info.height);
    std::cout << "Saved scrolltable to: " << opts->save_scrolltable_file << std::endl;
  }

  if (!opts->save_metatiles_doc_file.empty()) {
    std::string path = getAbsoluteTilePath(opts, info.tilesetImagePath);
    saveMetatileDocHtml(info.metatiles, path, opts->save_metatiles_doc_file);
    std::cout << "Saved metatile html doc to: " << opts->save_metatiles_doc_file << std::endl;
  }
  
  std::cout << "fin. " << std::endl;

  return 0;
}
